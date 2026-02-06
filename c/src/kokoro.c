#include "kokoro.h"
#include "audio_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <espeak-ng/speak_lib.h>
#include <onnxruntime_c_api.h>

/* Maximum number of voices */
#define MAX_VOICES 256

/* Vocabulary size from config.json */
#define VOCAB_SIZE 178

/* Internal structures */
struct kokoro_t {
    const OrtApi* ort;
    OrtEnv* env;
    OrtSession* session;
    OrtAllocator* allocator;
    
    /* Voice embeddings: voice_name -> float array */
    char* voice_names[MAX_VOICES];
    float* voice_embeddings[MAX_VOICES];
    size_t num_voices;
    size_t embedding_size;  /* Total size of embedding (could be flattened 2D) */
    size_t voice_dim;       /* Dimension of a single voice vector (e.g., 256) */
    int is_2d_embedding;    /* 1 if embedding is 2D (flattened), 0 if 1D */
    
    /* Vocabulary mapping: phoneme -> token_id */
    int vocab[256];  /* Simple ASCII/UTF-8 first byte mapping */
    
    /* espeak-ng initialized flag */
    int espeak_initialized;
    
    /* Model input names (detected at initialization) */
    char* tokens_input_name;  /* "input_ids" or "tokens" */
    int use_newer_model;      /* 1 if model uses "input_ids", 0 if "tokens" */
    
    /* Model output name (detected at initialization) */
    char* output_name;  /* e.g., "output" or "audio" */
};

/* Vocabulary mapping: phoneme -> token_id
 * These mappings are defined in src/kokoro_onnx/config.json in the main Python implementation.
 * Token IDs correspond to the model's internal vocabulary. This is a simplified subset
 * containing the most common IPA phonemes for English and other languages.
 * Note: Some token IDs are non-consecutive due to the full vocabulary including
 * additional characters not commonly used in standard phonemization.
 */
static const struct {
    const char* phoneme;
    int token_id;
} VOCAB_MAP[] = {
    /* Punctuation and separators */
    {";", 1}, {":", 2}, {",", 3}, {".", 4}, {"!", 5}, {"?", 6},
    {"—", 9}, {"…", 10}, {"\"", 11}, {"(", 12}, {")", 13},
    {""", 14}, {""", 15}, {" ", 16},
    /* Basic Latin alphabet */
    {"a", 43}, {"b", 44}, {"c", 45}, {"d", 46}, {"e", 47}, {"f", 48},
    {"h", 50}, {"i", 51}, {"j", 52}, {"k", 53}, {"l", 54}, {"m", 55},
    {"n", 56}, {"o", 57}, {"p", 58}, {"q", 59}, {"r", 60}, {"s", 61},
    {"t", 62}, {"u", 63}, {"v", 64}, {"w", 65}, {"x", 66}, {"y", 67},
    {"z", 68},
    /* IPA vowels */
    {"ɑ", 69}, {"ɐ", 70}, {"ɒ", 71}, {"æ", 72}, {"ɔ", 76},
    {"ə", 83}, {"ɛ", 86}, {"ɜ", 87}, {"ɪ", 102}, {"ʊ", 135}, {"ʌ", 138},
    /* IPA consonants */
    {"ð", 81}, {"ɡ", 92}, {"ŋ", 112}, {"ɹ", 123}, {"ʃ", 131},
    {"ʒ", 147}, {"ʔ", 148},
    /* IPA modifiers */
    {"ˈ", 156}, {"ˌ", 157}, {"ː", 158},
    {NULL, 0}
};

/* Helper function to check ONNX Runtime errors */
static int check_ort_status(const OrtApi* ort, OrtStatus* status) {
    if (status != NULL) {
        const char* msg = ort->GetErrorMessage(status);
        fprintf(stderr, "ONNX Runtime error: %s\n", msg);
        ort->ReleaseStatus(status);
        return -1;
    }
    return 0;
}

/* Initialize vocabulary mapping */
static void init_vocab(kokoro_t* kokoro) {
    /* Initialize all to -1 (not found) */
    for (int i = 0; i < 256; i++) {
        kokoro->vocab[i] = -1;
    }
    
    /* Map single-byte characters */
    for (int i = 0; VOCAB_MAP[i].phoneme != NULL; i++) {
        if (strlen(VOCAB_MAP[i].phoneme) == 1) {
            unsigned char c = (unsigned char)VOCAB_MAP[i].phoneme[0];
            kokoro->vocab[c] = VOCAB_MAP[i].token_id;
        }
    }
}

/* Voice file format constants */
#define VOICE_FILE_UINT32_SIZE 4
#define VOICE_FILE_FLOAT32_SIZE 4

/* Load voices from binary file
 * Binary format created by scripts/convert_voices.py:
 * - Header:
 *   - uint32_t (4 bytes): number of voices
 *   - uint32_t (4 bytes): embedding size (floats per voice)
 * - For each voice:
 *   - uint32_t (4 bytes): voice name length in bytes
 *   - char[] (variable): UTF-8 encoded voice name
 *   - float32[] (embedding_size * 4 bytes): voice embedding
 */
static kokoro_error_t load_voices(kokoro_t* kokoro, const char* voices_path) {
    FILE* fp = fopen(voices_path, "rb");
    if (!fp) {
        return KOKORO_ERROR_FILE_NOT_FOUND;
    }
    
    uint32_t num_voices;
    uint32_t embedding_size;
    
    if (fread(&num_voices, VOICE_FILE_UINT32_SIZE, 1, fp) != 1 ||
        fread(&embedding_size, VOICE_FILE_UINT32_SIZE, 1, fp) != 1) {
        fclose(fp);
        return KOKORO_ERROR_INIT_FAILED;
    }
    
    kokoro->num_voices = (size_t)num_voices;
    kokoro->embedding_size = (size_t)embedding_size;
    
    /* Detect if embeddings are 2D (flattened from [max_length, dim])
     * If embedding_size is much larger than 256, it's likely flattened 2D */
    if (kokoro->embedding_size == 256) {
        /* 1D embedding: shape is [256] */
        kokoro->is_2d_embedding = 0;
        kokoro->voice_dim = 256;
    } else if (kokoro->embedding_size == 130560) {
        /* 2D embedding flattened: shape was [510, 256] */
        kokoro->is_2d_embedding = 1;
        kokoro->voice_dim = 256;
    } else {
        /* Try to detect based on divisibility */
        kokoro->is_2d_embedding = (kokoro->embedding_size % 256 == 0 && kokoro->embedding_size > 256);
        kokoro->voice_dim = 256;
    }
    
    if (kokoro->num_voices > MAX_VOICES) {
        fclose(fp);
        return KOKORO_ERROR_INIT_FAILED;
    }
    
    for (size_t i = 0; i < kokoro->num_voices; i++) {
        uint32_t name_len;
        if (fread(&name_len, VOICE_FILE_UINT32_SIZE, 1, fp) != 1) {
            fclose(fp);
            return KOKORO_ERROR_INIT_FAILED;
        }
        
        kokoro->voice_names[i] = (char*)malloc(name_len + 1);
        if (!kokoro->voice_names[i]) {
            fclose(fp);
            return KOKORO_ERROR_OUT_OF_MEMORY;
        }
        
        if (fread(kokoro->voice_names[i], 1, name_len, fp) != name_len) {
            fclose(fp);
            return KOKORO_ERROR_INIT_FAILED;
        }
        kokoro->voice_names[i][name_len] = '\0';
        
        kokoro->voice_embeddings[i] = (float*)malloc(kokoro->embedding_size * VOICE_FILE_FLOAT32_SIZE);
        if (!kokoro->voice_embeddings[i]) {
            fclose(fp);
            return KOKORO_ERROR_OUT_OF_MEMORY;
        }
        
        if (fread(kokoro->voice_embeddings[i], VOICE_FILE_FLOAT32_SIZE, kokoro->embedding_size, fp) != kokoro->embedding_size) {
            fclose(fp);
            return KOKORO_ERROR_INIT_FAILED;
        }
    }
    
    fclose(fp);
    return KOKORO_SUCCESS;
}

kokoro_t* kokoro_init(
    const char* model_path,
    const char* voices_path,
    const char* espeak_lib_path,
    const char* espeak_data_path
) {
    if (!model_path || !voices_path) {
        fprintf(stderr, "Kokoro init error: model_path and voices_path are required\n");
        return NULL;
    }
    
    kokoro_t* kokoro = (kokoro_t*)calloc(1, sizeof(kokoro_t));
    if (!kokoro) {
        fprintf(stderr, "Kokoro init error: Out of memory\n");
        return NULL;
    }
    
    /* Initialize ONNX Runtime */
    kokoro->ort = OrtGetApiBase()->GetApi(ORT_API_VERSION);
    if (!kokoro->ort) {
        fprintf(stderr, "Kokoro init error: Failed to get ONNX Runtime API\n");
        free(kokoro);
        return NULL;
    }
    
    OrtStatus* status = kokoro->ort->CreateEnv(ORT_LOGGING_LEVEL_WARNING, "kokoro", &kokoro->env);
    if (check_ort_status(kokoro->ort, status) != 0) {
        free(kokoro);
        return NULL;
    }
    
    /* Create session options */
    OrtSessionOptions* session_options;
    status = kokoro->ort->CreateSessionOptions(&session_options);
    if (check_ort_status(kokoro->ort, status) != 0) {
        kokoro->ort->ReleaseEnv(kokoro->env);
        free(kokoro);
        return NULL;
    }
    
    /* Set number of threads */
    kokoro->ort->SetIntraOpNumThreads(session_options, 1);
    kokoro->ort->SetSessionGraphOptimizationLevel(session_options, ORT_ENABLE_BASIC);
    
    /* Create session */
    status = kokoro->ort->CreateSession(kokoro->env, model_path, session_options, &kokoro->session);
    kokoro->ort->ReleaseSessionOptions(session_options);
    
    if (check_ort_status(kokoro->ort, status) != 0) {
        kokoro->ort->ReleaseEnv(kokoro->env);
        free(kokoro);
        return NULL;
    }
    
    /* Get allocator */
    status = kokoro->ort->GetAllocatorWithDefaultOptions(&kokoro->allocator);
    if (check_ort_status(kokoro->ort, status) != 0) {
        kokoro->ort->ReleaseSession(kokoro->session);
        kokoro->ort->ReleaseEnv(kokoro->env);
        free(kokoro);
        return NULL;
    }
    
    /* Detect model input names */
    size_t num_inputs;
    status = kokoro->ort->SessionGetInputCount(kokoro->session, &num_inputs);
    if (check_ort_status(kokoro->ort, status) != 0) {
        kokoro->ort->ReleaseSession(kokoro->session);
        kokoro->ort->ReleaseEnv(kokoro->env);
        free(kokoro);
        return NULL;
    }
    
    /* Check if model uses "input_ids" (newer) or "tokens" (older) */
    kokoro->use_newer_model = 0;
    kokoro->tokens_input_name = NULL;
    
    for (size_t i = 0; i < num_inputs; i++) {
        char* input_name;
        status = kokoro->ort->SessionGetInputName(kokoro->session, i, kokoro->allocator, &input_name);
        if (status == NULL && input_name != NULL) {
            if (strcmp(input_name, "input_ids") == 0) {
                kokoro->use_newer_model = 1;
                kokoro->tokens_input_name = strdup("input_ids");
                kokoro->ort->AllocatorFree(kokoro->allocator, input_name);
                break;
            } else if (strcmp(input_name, "tokens") == 0) {
                kokoro->use_newer_model = 0;
                kokoro->tokens_input_name = strdup("tokens");
                kokoro->ort->AllocatorFree(kokoro->allocator, input_name);
                break;
            }
            kokoro->ort->AllocatorFree(kokoro->allocator, input_name);
        }
    }
    
    if (!kokoro->tokens_input_name) {
        fprintf(stderr, "Kokoro init error: Could not detect model input format\n");
        kokoro->ort->ReleaseSession(kokoro->session);
        kokoro->ort->ReleaseEnv(kokoro->env);
        free(kokoro);
        return NULL;
    }
    
    /* Detect model output name */
    size_t num_outputs;
    status = kokoro->ort->SessionGetOutputCount(kokoro->session, &num_outputs);
    if (check_ort_status(kokoro->ort, status) != 0 || num_outputs == 0) {
        fprintf(stderr, "Kokoro init error: Could not get model outputs\n");
        free(kokoro->tokens_input_name);
        kokoro->ort->ReleaseSession(kokoro->session);
        kokoro->ort->ReleaseEnv(kokoro->env);
        free(kokoro);
        return NULL;
    }
    
    /* Get the first output name (models typically have one audio output) */
    char* output_name;
    status = kokoro->ort->SessionGetOutputName(kokoro->session, 0, kokoro->allocator, &output_name);
    if (check_ort_status(kokoro->ort, status) != 0 || output_name == NULL) {
        fprintf(stderr, "Kokoro init error: Could not get model output name\n");
        free(kokoro->tokens_input_name);
        kokoro->ort->ReleaseSession(kokoro->session);
        kokoro->ort->ReleaseEnv(kokoro->env);
        free(kokoro);
        return NULL;
    }
    
    kokoro->output_name = strdup(output_name);
    kokoro->ort->AllocatorFree(kokoro->allocator, output_name);
    
    /* Initialize vocabulary */
    init_vocab(kokoro);
    
    /* Load voices */
    kokoro_error_t err = load_voices(kokoro, voices_path);
    if (err != KOKORO_SUCCESS) {
        kokoro_free(kokoro);
        return NULL;
    }
    
    /* Initialize espeak-ng */
    int espeak_result = espeak_Initialize(AUDIO_OUTPUT_SYNCHRONOUS, 0, espeak_data_path, 0);
    if (espeak_result < 0) {
        fprintf(stderr, "Warning: Failed to initialize espeak-ng\n");
        kokoro->espeak_initialized = 0;
    } else {
        kokoro->espeak_initialized = 1;
    }
    
    return kokoro;
}

void kokoro_free(kokoro_t* kokoro) {
    if (!kokoro) {
        return;
    }
    
    /* Free voices */
    for (size_t i = 0; i < kokoro->num_voices; i++) {
        free(kokoro->voice_names[i]);
        free(kokoro->voice_embeddings[i]);
    }
    
    /* Free model input name */
    if (kokoro->tokens_input_name) {
        free(kokoro->tokens_input_name);
    }
    
    /* Free model output name */
    if (kokoro->output_name) {
        free(kokoro->output_name);
    }
    
    /* Free ONNX Runtime resources */
    if (kokoro->session) {
        kokoro->ort->ReleaseSession(kokoro->session);
    }
    if (kokoro->env) {
        kokoro->ort->ReleaseEnv(kokoro->env);
    }
    
    /* Terminate espeak-ng */
    if (kokoro->espeak_initialized) {
        espeak_Terminate();
    }
    
    free(kokoro);
}

/* Find voice embedding by name */
static float* find_voice_embedding(kokoro_t* kokoro, const char* voice_name) {
    for (size_t i = 0; i < kokoro->num_voices; i++) {
        if (strcmp(kokoro->voice_names[i], voice_name) == 0) {
            return kokoro->voice_embeddings[i];
        }
    }
    return NULL;
}

/* Tokenize phonemes */
static int tokenize_phonemes(kokoro_t* kokoro, const char* phonemes, int64_t* tokens, size_t max_tokens) {
    size_t token_count = 0;
    const unsigned char* p = (const unsigned char*)phonemes;
    
    while (*p && token_count < max_tokens) {
        int token_id = kokoro->vocab[*p];
        if (token_id >= 0) {
            tokens[token_count++] = token_id;
        }
        p++;
    }
    
    return token_count;
}

kokoro_error_t kokoro_text_to_phonemes(
    kokoro_t* kokoro,
    const char* text,
    const char* lang,
    char** phonemes
) {
    if (!kokoro || !text || !phonemes) {
        return KOKORO_ERROR_NULL_POINTER;
    }
    
    if (!kokoro->espeak_initialized) {
        return KOKORO_ERROR_PHONEMIZE_FAILED;
    }
    
    /* Set language */
    if (lang) {
        espeak_VOICE voice = {0};
        voice.languages = lang;
        espeak_SetVoiceByProperties(&voice);
    }
    
    /* Convert text to phonemes */
    const char* ipa = espeak_TextToPhonemes((const void**)&text, espeakCHARS_UTF8, 
                                            espeakPHONEMES_IPA | espeakPHONEMES_SHOW);
    
    if (!ipa) {
        return KOKORO_ERROR_PHONEMIZE_FAILED;
    }
    
    *phonemes = strdup(ipa);
    if (!*phonemes) {
        return KOKORO_ERROR_OUT_OF_MEMORY;
    }
    
    return KOKORO_SUCCESS;
}

kokoro_error_t kokoro_create_from_phonemes(
    kokoro_t* kokoro,
    const char* phonemes,
    const char* voice,
    float speed,
    kokoro_audio_t* audio
) {
    if (!kokoro || !phonemes || !voice || !audio) {
        return KOKORO_ERROR_NULL_POINTER;
    }
    
    if (speed < 0.5f || speed > 2.0f) {
        return KOKORO_ERROR_INVALID_SPEED;
    }
    
    /* Find voice embedding */
    float* voice_embedding = find_voice_embedding(kokoro, voice);
    if (!voice_embedding) {
        return KOKORO_ERROR_INVALID_VOICE;
    }
    
    /* Tokenize phonemes */
    int64_t tokens[KOKORO_MAX_PHONEME_LENGTH + 2];  /* +2 for padding */
    int num_tokens = tokenize_phonemes(kokoro, phonemes, tokens + 1, KOKORO_MAX_PHONEME_LENGTH);
    
    if (num_tokens == 0) {
        return KOKORO_ERROR_TEXT_TOO_LONG;
    }
    
    /* Add padding tokens (0 at start and end) */
    tokens[0] = 0;
    tokens[num_tokens + 1] = 0;
    num_tokens += 2;
    
    /* Index into voice embedding based on sequence length (like Python implementation)
     * Python: voice = voice[len(tokens)]
     * If embedding is 2D (flattened from [max_length, dim]), we need to index by token count */
    float* style;
    if (kokoro->is_2d_embedding) {
        /* 2D embedding: index into flattened array [max_length * dim]
         * Offset = num_tokens * voice_dim (but num_tokens includes padding, need actual phoneme count) */
        int phoneme_count = num_tokens - 2;  /* Remove padding tokens */
        size_t offset = phoneme_count * kokoro->voice_dim;
        style = voice_embedding + offset;
    } else {
        /* 1D embedding: use directly */
        style = voice_embedding;
    }
    
    /* Prepare ONNX Runtime inputs */
    OrtMemoryInfo* memory_info;
    OrtStatus* status = kokoro->ort->CreateCpuMemoryInfo(OrtArenaAllocator, OrtMemTypeDefault, &memory_info);
    if (check_ort_status(kokoro->ort, status) != 0) {
        return KOKORO_ERROR_INFERENCE_FAILED;
    }
    
    /* Create input tensors */
    int64_t input_shape[] = {1, num_tokens};
    OrtValue* input_tensor = NULL;
    status = kokoro->ort->CreateTensorWithDataAsOrtValue(
        memory_info, tokens, num_tokens * sizeof(int64_t),
        input_shape, 2, ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64, &input_tensor
    );
    
    if (check_ort_status(kokoro->ort, status) != 0) {
        kokoro->ort->ReleaseMemoryInfo(memory_info);
        return KOKORO_ERROR_INFERENCE_FAILED;
    }
    
    /* Create style tensor */
    int64_t style_shape[] = {1, kokoro->voice_dim};
    OrtValue* style_tensor = NULL;
    status = kokoro->ort->CreateTensorWithDataAsOrtValue(
        memory_info, style, kokoro->voice_dim * sizeof(float),
        style_shape, 2, ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT, &style_tensor
    );
    
    if (check_ort_status(kokoro->ort, status) != 0) {
        kokoro->ort->ReleaseValue(input_tensor);
        kokoro->ort->ReleaseMemoryInfo(memory_info);
        return KOKORO_ERROR_INFERENCE_FAILED;
    }
    
    /* Create speed tensor - type depends on model version */
    OrtValue* speed_tensor = NULL;
    if (kokoro->use_newer_model) {
        /* Newer models use int32 for speed */
        int32_t speed_val_int = (int32_t)speed;
        int64_t speed_shape[] = {1};
        status = kokoro->ort->CreateTensorWithDataAsOrtValue(
            memory_info, &speed_val_int, sizeof(int32_t),
            speed_shape, 1, ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32, &speed_tensor
        );
    } else {
        /* Older models use float32 for speed */
        float speed_val = speed;
        int64_t speed_shape[] = {1};
        status = kokoro->ort->CreateTensorWithDataAsOrtValue(
            memory_info, &speed_val, sizeof(float),
            speed_shape, 1, ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT, &speed_tensor
        );
    }
    
    if (check_ort_status(kokoro->ort, status) != 0) {
        kokoro->ort->ReleaseValue(input_tensor);
        kokoro->ort->ReleaseValue(style_tensor);
        kokoro->ort->ReleaseMemoryInfo(memory_info);
        return KOKORO_ERROR_INFERENCE_FAILED;
    }
    
    /* Run inference with detected input and output names */
    const char* input_names[] = {kokoro->tokens_input_name, "style", "speed"};
    const char* output_names[] = {kokoro->output_name};
    OrtValue* input_tensors[] = {input_tensor, style_tensor, speed_tensor};
    OrtValue* output_tensor = NULL;
    
    status = kokoro->ort->Run(
        kokoro->session,
        NULL,  /* run options */
        input_names,
        (const OrtValue* const*)input_tensors,
        3,  /* num inputs */
        output_names,
        1,  /* num outputs */
        &output_tensor
    );
    
    int inference_ok = (check_ort_status(kokoro->ort, status) == 0);
    
    /* Cleanup input tensors */
    kokoro->ort->ReleaseValue(input_tensor);
    kokoro->ort->ReleaseValue(style_tensor);
    kokoro->ort->ReleaseValue(speed_tensor);
    kokoro->ort->ReleaseMemoryInfo(memory_info);
    
    if (!inference_ok) {
        return KOKORO_ERROR_INFERENCE_FAILED;
    }
    
    /* Extract output */
    float* output_data;
    status = kokoro->ort->GetTensorMutableData(output_tensor, (void**)&output_data);
    if (check_ort_status(kokoro->ort, status) != 0) {
        kokoro->ort->ReleaseValue(output_tensor);
        return KOKORO_ERROR_INFERENCE_FAILED;
    }
    
    /* Get output size */
    OrtTensorTypeAndShapeInfo* shape_info;
    status = kokoro->ort->GetTensorTypeAndShape(output_tensor, &shape_info);
    if (check_ort_status(kokoro->ort, status) != 0) {
        kokoro->ort->ReleaseValue(output_tensor);
        return KOKORO_ERROR_INFERENCE_FAILED;
    }
    
    size_t num_elements;
    kokoro->ort->GetTensorShapeElementCount(shape_info, &num_elements);
    kokoro->ort->ReleaseTensorTypeAndShapeInfo(shape_info);
    
    /* Allocate output audio */
    audio->samples = (float*)malloc(num_elements * sizeof(float));
    if (!audio->samples) {
        kokoro->ort->ReleaseValue(output_tensor);
        return KOKORO_ERROR_OUT_OF_MEMORY;
    }
    
    memcpy(audio->samples, output_data, num_elements * sizeof(float));
    audio->num_samples = num_elements;
    audio->sample_rate = KOKORO_SAMPLE_RATE;
    
    kokoro->ort->ReleaseValue(output_tensor);
    return KOKORO_SUCCESS;
}

/**
 * Split phonemes into batches at punctuation marks to avoid exceeding MAX_PHONEME_LENGTH
 * Returns array of phoneme strings (caller must free each string and array)
 */
#define BATCH_BUFFER_PADDING 100  /* Extra buffer space for punctuation and safety margin */

static char** split_phonemes_into_batches(const char* phonemes, size_t* num_batches) {
    if (!phonemes || !num_batches) {
        return NULL;
    }
    
    size_t phoneme_len = strlen(phonemes);
    *num_batches = 0;
    
    /* Quick check: if phonemes fit in one batch, return them as-is */
    if (phoneme_len <= KOKORO_MAX_PHONEME_LENGTH) {
        char** batches = (char**)malloc(sizeof(char*));
        if (!batches) return NULL;
        batches[0] = strdup(phonemes);
        if (!batches[0]) {
            free(batches);
            return NULL;
        }
        *num_batches = 1;
        return batches;
    }
    
    /* Allocate array for batches (estimate with +2 for edge cases where punctuation creates extra batches) */
    size_t max_batches = (phoneme_len / KOKORO_MAX_PHONEME_LENGTH) + 2;
    char** batches = (char**)malloc(max_batches * sizeof(char*));
    if (!batches) return NULL;
    
    /* Split by punctuation marks */
    const char* punctuation = ".,!?;";
    char* phonemes_copy = strdup(phonemes);
    if (!phonemes_copy) {
        free(batches);
        return NULL;
    }
    
    char* current_batch = (char*)malloc(KOKORO_MAX_PHONEME_LENGTH + BATCH_BUFFER_PADDING);
    if (!current_batch) {
        free(phonemes_copy);
        free(batches);
        return NULL;
    }
    current_batch[0] = '\0';
    size_t current_len = 0;
    
    /* Parse phonemes character by character */
    const char* p = phonemes_copy;
    const char* word_start = p;
    
    while (*p) {
        /* Check if we're at punctuation or end of string */
        int is_punct = (strchr(punctuation, *p) != NULL);
        int is_space = (*p == ' ');
        
        if (is_punct || is_space || *(p + 1) == '\0') {
            /* Extract word/segment */
            size_t segment_len = p - word_start + (is_punct || *(p + 1) == '\0' ? 1 : 0);
            
            /* Check if adding this segment would exceed limit */
            if (current_len + segment_len + 1 >= KOKORO_MAX_PHONEME_LENGTH && current_len > 0) {
                /* Save current batch */
                batches[*num_batches] = strdup(current_batch);
                if (!batches[*num_batches]) {
                    /* Cleanup on error */
                    for (size_t i = 0; i < *num_batches; i++) {
                        free(batches[i]);
                    }
                    free(batches);
                    free(current_batch);
                    free(phonemes_copy);
                    return NULL;
                }
                (*num_batches)++;
                current_batch[0] = '\0';
                current_len = 0;
            }
            
            /* Add segment to current batch */
            if (current_len > 0 && !is_punct) {
                /* Add space separator using direct indexing for efficiency */
                current_batch[current_len] = ' ';
                current_batch[current_len + 1] = '\0';
                current_len++;
            }
            /* Use memcpy for efficiency instead of strncat */
            memcpy(current_batch + current_len, word_start, segment_len);
            current_len += segment_len;
            current_batch[current_len] = '\0';
            
            /* Move to next segment */
            word_start = p + 1;
        }
        
        p++;
    }
    
    /* Add last batch if not empty */
    if (current_len > 0) {
        batches[*num_batches] = strdup(current_batch);
        if (!batches[*num_batches]) {
            for (size_t i = 0; i < *num_batches; i++) {
                free(batches[i]);
            }
            free(batches);
            free(current_batch);
            free(phonemes_copy);
            return NULL;
        }
        (*num_batches)++;
    }
    
    free(current_batch);
    free(phonemes_copy);
    return batches;
}

kokoro_error_t kokoro_create(
    kokoro_t* kokoro,
    const char* text,
    const char* voice,
    float speed,
    const char* lang,
    kokoro_audio_t* audio
) {
    if (!kokoro || !text || !voice || !audio) {
        return KOKORO_ERROR_NULL_POINTER;
    }
    
    /* Convert text to phonemes */
    char* phonemes = NULL;
    kokoro_error_t err = kokoro_text_to_phonemes(kokoro, text, lang, &phonemes);
    if (err != KOKORO_SUCCESS) {
        return err;
    }
    
    /* Split phonemes into batches if needed */
    size_t num_batches = 0;
    char** phoneme_batches = split_phonemes_into_batches(phonemes, &num_batches);
    free(phonemes);
    
    if (!phoneme_batches || num_batches == 0) {
        return KOKORO_ERROR_OUT_OF_MEMORY;
    }
    
    /* Handle single batch case (most common) */
    if (num_batches == 1) {
        err = kokoro_create_from_phonemes(kokoro, phoneme_batches[0], voice, speed, audio);
        free(phoneme_batches[0]);
        free(phoneme_batches);
        
        /* Apply trimming if audio was generated successfully */
        if (err == KOKORO_SUCCESS && audio->num_samples > 0) {
            float* trimmed_samples = (float*)malloc(audio->num_samples * sizeof(float));
            if (trimmed_samples) {
                size_t trimmed_size = 0;
                int trim_result = audio_trim_silence(
                    audio->samples, audio->num_samples,
                    trimmed_samples, &trimmed_size,
                    60.0f, 2048, 512
                );
                if (trim_result == 0 && trimmed_size > 0) {
                    free(audio->samples);
                    audio->samples = trimmed_samples;
                    audio->num_samples = trimmed_size;
                } else {
                    free(trimmed_samples);
                }
            }
        }
        
        return err;
    }
    
    /* Multiple batches: generate and concatenate audio */
    kokoro_audio_t* batch_audios = (kokoro_audio_t*)calloc(num_batches, sizeof(kokoro_audio_t));
    if (!batch_audios) {
        for (size_t i = 0; i < num_batches; i++) {
            free(phoneme_batches[i]);
        }
        free(phoneme_batches);
        return KOKORO_ERROR_OUT_OF_MEMORY;
    }
    
    /* Generate audio for each batch */
    size_t total_samples = 0;
    for (size_t i = 0; i < num_batches; i++) {
        err = kokoro_create_from_phonemes(kokoro, phoneme_batches[i], voice, speed, &batch_audios[i]);
        free(phoneme_batches[i]);
        
        if (err != KOKORO_SUCCESS) {
            /* Cleanup on error */
            for (size_t j = 0; j < i; j++) {
                kokoro_audio_free(&batch_audios[j]);
            }
            free(batch_audios);
            free(phoneme_batches);
            return err;
        }
        
        /* Trim each batch for better concatenation */
        if (batch_audios[i].num_samples > 0) {
            float* trimmed = (float*)malloc(batch_audios[i].num_samples * sizeof(float));
            if (trimmed) {
                size_t trimmed_size = 0;
                int trim_result = audio_trim_silence(
                    batch_audios[i].samples, batch_audios[i].num_samples,
                    trimmed, &trimmed_size,
                    60.0f, 2048, 512
                );
                if (trim_result == 0 && trimmed_size > 0) {
                    free(batch_audios[i].samples);
                    batch_audios[i].samples = trimmed;
                    batch_audios[i].num_samples = trimmed_size;
                    total_samples += trimmed_size;
                } else {
                    free(trimmed);
                    total_samples += batch_audios[i].num_samples;
                }
            } else {
                total_samples += batch_audios[i].num_samples;
            }
        }
    }
    free(phoneme_batches);
    
    /* Concatenate all batch audios */
    audio->samples = (float*)malloc(total_samples * sizeof(float));
    if (!audio->samples) {
        for (size_t i = 0; i < num_batches; i++) {
            kokoro_audio_free(&batch_audios[i]);
        }
        free(batch_audios);
        return KOKORO_ERROR_OUT_OF_MEMORY;
    }
    
    audio->num_samples = 0;
    audio->sample_rate = KOKORO_SAMPLE_RATE;
    
    for (size_t i = 0; i < num_batches; i++) {
        memcpy(audio->samples + audio->num_samples, batch_audios[i].samples, 
               batch_audios[i].num_samples * sizeof(float));
        audio->num_samples += batch_audios[i].num_samples;
        kokoro_audio_free(&batch_audios[i]);
    }
    free(batch_audios);
    
    return KOKORO_SUCCESS;
}

kokoro_error_t kokoro_get_voices(
    kokoro_t* kokoro,
    char*** voices,
    size_t* num_voices
) {
    if (!kokoro || !voices || !num_voices) {
        return KOKORO_ERROR_NULL_POINTER;
    }
    
    *voices = (char**)malloc(kokoro->num_voices * sizeof(char*));
    if (!*voices) {
        return KOKORO_ERROR_OUT_OF_MEMORY;
    }
    
    for (size_t i = 0; i < kokoro->num_voices; i++) {
        (*voices)[i] = strdup(kokoro->voice_names[i]);
        if (!(*voices)[i]) {
            /* Free already allocated strings */
            for (size_t j = 0; j < i; j++) {
                free((*voices)[j]);
            }
            free(*voices);
            return KOKORO_ERROR_OUT_OF_MEMORY;
        }
    }
    
    *num_voices = kokoro->num_voices;
    return KOKORO_SUCCESS;
}

void kokoro_audio_free(kokoro_audio_t* audio) {
    if (audio && audio->samples) {
        free(audio->samples);
        audio->samples = NULL;
        audio->num_samples = 0;
    }
}

const char* kokoro_error_string(kokoro_error_t error) {
    switch (error) {
        case KOKORO_SUCCESS:
            return "Success";
        case KOKORO_ERROR_NULL_POINTER:
            return "Null pointer argument";
        case KOKORO_ERROR_FILE_NOT_FOUND:
            return "File not found";
        case KOKORO_ERROR_INIT_FAILED:
            return "Initialization failed";
        case KOKORO_ERROR_INFERENCE_FAILED:
            return "Inference failed";
        case KOKORO_ERROR_INVALID_VOICE:
            return "Invalid voice";
        case KOKORO_ERROR_TEXT_TOO_LONG:
            return "Text too long";
        case KOKORO_ERROR_INVALID_SPEED:
            return "Invalid speed (must be 0.5-2.0)";
        case KOKORO_ERROR_PHONEMIZE_FAILED:
            return "Failed to convert text to phonemes";
        case KOKORO_ERROR_OUT_OF_MEMORY:
            return "Out of memory";
        default:
            return "Unknown error";
    }
}
