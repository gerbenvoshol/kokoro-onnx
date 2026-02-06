/**
 * Audiobook Generator using Kokoro TTS C API
 * 
 * This example reads a text file and generates an audiobook with natural pauses
 * at punctuation marks. It also supports custom pause directives in the text.
 * 
 * Features:
 * - Automatic pauses at punctuation (., , ; : ! ?) - can be disabled
 * - Custom pause directives: [PAUSE:500] for 500ms pause
 * - Inline voice switching: [af_sarah] to change voice
 * - Sentence-by-sentence processing for better memory usage
 * - Automatic handling of long sentences via phoneme batching
 * - Progress display during generation
 * 
 * Note: The underlying library automatically batches long sentences (>510 phonemes)
 * and trims silence, so individual sentences of any length are supported.
 * 
 * Usage:
 *   ./audiobook -m <model.onnx> -v <voices.bin> -i <input.txt> -o <output.wav>
 *   ./audiobook <model.onnx> <voices.bin> <input.txt> <output.wav>  (backward compatible)
 * 
 * Example:
 *   ./audiobook -m kokoro-v1.0.onnx -v voices-v1.0-c.bin -i story.txt -o audiobook.wav
 *   ./audiobook -m kokoro-v1.0.onnx -v voices-v1.0-c.bin -i story.txt -o audiobook.wav --no-auto-pause
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include "kokoro.h"
#include "argparse.h"

/* Default pause durations in milliseconds */
#define PAUSE_PERIOD 500      /* Period/exclamation/question mark */
#define PAUSE_COMMA 250       /* Comma */
#define PAUSE_SEMICOLON 350   /* Semicolon/colon */
#define PAUSE_PARAGRAPH 800   /* Double newline (paragraph break) */

/* Maximum line length */
#define MAX_LINE_LENGTH 2048

/* WAV file header structure */
typedef struct {
    char riff[4];
    uint32_t file_size;
    char wave[4];
    char fmt[4];
    uint32_t fmt_size;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char data[4];
    uint32_t data_size;
} wav_header_t;

/**
 * Dynamic audio buffer for accumulating samples
 */
typedef struct {
    float* samples;
    size_t num_samples;
    size_t capacity;
    int sample_rate;
} audio_buffer_t;

/**
 * Initialize audio buffer
 */
void audio_buffer_init(audio_buffer_t* buffer, int sample_rate) {
    buffer->samples = NULL;
    buffer->num_samples = 0;
    buffer->capacity = 0;
    buffer->sample_rate = sample_rate;
}

/**
 * Append audio samples to buffer
 */
int audio_buffer_append(audio_buffer_t* buffer, const float* samples, size_t num_samples) {
    size_t new_size = buffer->num_samples + num_samples;
    
    if (new_size > buffer->capacity) {
        size_t new_capacity = (buffer->capacity == 0) ? 48000 : buffer->capacity * 2;
        while (new_capacity < new_size) {
            new_capacity *= 2;
        }
        
        float* new_samples = (float*)realloc(buffer->samples, new_capacity * sizeof(float));
        if (!new_samples) {
            return -1;
        }
        
        buffer->samples = new_samples;
        buffer->capacity = new_capacity;
    }
    
    memcpy(buffer->samples + buffer->num_samples, samples, num_samples * sizeof(float));
    buffer->num_samples = new_size;
    return 0;
}

/**
 * Append silence (pause) to buffer
 */
int audio_buffer_append_silence(audio_buffer_t* buffer, int duration_ms) {
    size_t num_silence_samples = (buffer->sample_rate * duration_ms) / 1000;
    
    if (num_silence_samples == 0) {
        return 0;
    }
    
    float* silence = (float*)calloc(num_silence_samples, sizeof(float));
    if (!silence) {
        return -1;
    }
    
    int result = audio_buffer_append(buffer, silence, num_silence_samples);
    free(silence);
    return result;
}

/**
 * Free audio buffer
 */
void audio_buffer_free(audio_buffer_t* buffer) {
    if (buffer->samples) {
        free(buffer->samples);
        buffer->samples = NULL;
    }
    buffer->num_samples = 0;
    buffer->capacity = 0;
}

/**
 * Write audio buffer to WAV file
 */
int write_wav(const char* filename, const audio_buffer_t* buffer) {
    FILE* fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "Error: Could not open output file: %s\n", filename);
        return -1;
    }
    
    /* Convert float samples to int16 */
    int16_t* pcm_samples = (int16_t*)malloc(buffer->num_samples * sizeof(int16_t));
    if (!pcm_samples) {
        fclose(fp);
        return -1;
    }
    
    for (size_t i = 0; i < buffer->num_samples; i++) {
        float sample = buffer->samples[i];
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;
        pcm_samples[i] = (int16_t)(sample * 32767.0f);
    }
    
    /* Create WAV header */
    wav_header_t header = {0};
    memcpy(header.riff, "RIFF", 4);
    memcpy(header.wave, "WAVE", 4);
    memcpy(header.fmt, "fmt ", 4);
    memcpy(header.data, "data", 4);
    
    header.fmt_size = 16;
    header.audio_format = 1;
    header.num_channels = 1;
    header.sample_rate = buffer->sample_rate;
    header.bits_per_sample = 16;
    header.block_align = header.num_channels * header.bits_per_sample / 8;
    header.byte_rate = header.sample_rate * header.block_align;
    header.data_size = buffer->num_samples * sizeof(int16_t);
    header.file_size = 36 + header.data_size;
    
    fwrite(&header, sizeof(wav_header_t), 1, fp);
    fwrite(pcm_samples, sizeof(int16_t), buffer->num_samples, fp);
    
    free(pcm_samples);
    fclose(fp);
    return 0;
}

/**
 * Parse custom pause directive [PAUSE:duration_ms]
 * Returns pause duration in ms, or -1 if not a pause directive
 */
int parse_pause_directive(const char* text, size_t* chars_consumed) {
    if (strncmp(text, "[PAUSE:", 7) != 0) {
        return -1;
    }
    
    const char* p = text + 7;
    int duration = 0;
    
    while (isdigit(*p)) {
        duration = duration * 10 + (*p - '0');
        p++;
    }
    
    if (*p != ']') {
        return -1;
    }
    
    *chars_consumed = (p + 1) - text;
    return duration;
}

/**
 * Parse voice directive [voice_name]
 * Returns 1 if voice directive found, 0 otherwise
 * Stores voice name in buffer (caller must provide buffer of sufficient size)
 */
int parse_voice_directive(const char* text, size_t* chars_consumed, char* voice_buffer, size_t buffer_size) {
    if (*text != '[') {
        return 0;
    }
    
    /* Check if it's a PAUSE directive first */
    if (strncmp(text, "[PAUSE:", 7) == 0) {
        return 0;
    }
    
    const char* p = text + 1;
    size_t voice_len = 0;
    
    /* Read until ] or end of valid voice name characters */
    while (*p && *p != ']' && voice_len < buffer_size - 1) {
        /* Voice names typically contain letters, numbers, and underscores */
        if (isalnum(*p) || *p == '_') {
            voice_buffer[voice_len++] = *p;
            p++;
        } else {
            /* Invalid character for voice name */
            return 0;
        }
    }
    
    if (*p != ']' || voice_len == 0) {
        return 0;
    }
    
    voice_buffer[voice_len] = '\0';
    *chars_consumed = (p + 1) - text;
    return 1;
}

/**
 * Get pause duration for punctuation character
 */
int get_punctuation_pause(char c) {
    switch (c) {
        case '.':
        case '!':
        case '?':
            return PAUSE_PERIOD;
        case ',':
            return PAUSE_COMMA;
        case ';':
        case ':':
            return PAUSE_SEMICOLON;
        default:
            return 0;
    }
}

/**
 * Trim whitespace from string
 */
void trim_whitespace(char* str) {
    char* start = str;
    while (isspace(*start)) start++;
    
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
    
    char* end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) {
        *end = '\0';
        end--;
    }
}

/**
 * Process text file and generate audiobook
 * 
 * Note: kokoro_create() automatically handles long sentences by batching phonemes
 * at punctuation marks when they exceed 510 characters, so no special handling
 * is needed here for long sentences.
 */
int generate_audiobook(kokoro_t* kokoro, const char* input_file, 
                       audio_buffer_t* output, const char* initial_voice,
                       const char* lang, float speed, int auto_pause) {
    FILE* fp = fopen(input_file, "r");
    if (!fp) {
        fprintf(stderr, "Error: Could not open input file: %s\n", input_file);
        return -1;
    }
    
    char line[MAX_LINE_LENGTH];
    char sentence[MAX_LINE_LENGTH * 2] = {0};
    char current_voice[64];
    strncpy(current_voice, initial_voice, sizeof(current_voice) - 1);
    current_voice[sizeof(current_voice) - 1] = '\0';
    
    int line_count = 0;
    int sentence_count = 0;
    int last_was_empty = 0;
    
    printf("Processing text file...\n");
    printf("Auto-pause: %s\n", auto_pause ? "enabled" : "disabled");
    
    while (fgets(line, sizeof(line), fp)) {
        line_count++;
        trim_whitespace(line);
        
        /* Handle paragraph breaks (empty lines) */
        if (strlen(line) == 0) {
            if (!last_was_empty && strlen(sentence) > 0) {
                /* Generate audio for current sentence */
                kokoro_audio_t audio;
                kokoro_error_t err = kokoro_create(kokoro, sentence, current_voice, speed, lang, &audio);
                
                if (err == KOKORO_SUCCESS) {
                    audio_buffer_append(output, audio.samples, audio.num_samples);
                    kokoro_audio_free(&audio);
                    sentence_count++;
                    printf("  Sentence %d: %.2fs (voice: %s)\r", sentence_count, 
                           (double)output->num_samples / output->sample_rate, current_voice);
                    fflush(stdout);
                }
                
                /* Add paragraph pause only if auto_pause is enabled */
                if (auto_pause) {
                    audio_buffer_append_silence(output, PAUSE_PARAGRAPH);
                }
                sentence[0] = '\0';
            }
            last_was_empty = 1;
            continue;
        }
        
        last_was_empty = 0;
        
        /* Process line character by character */
        const char* p = line;
        while (*p) {
            size_t chars_consumed = 0;
            
            /* Check for custom pause directive (always works) */
            int pause_duration = parse_pause_directive(p, &chars_consumed);
            if (pause_duration >= 0) {
                /* Generate audio for accumulated sentence */
                if (strlen(sentence) > 0) {
                    kokoro_audio_t audio;
                    kokoro_error_t err = kokoro_create(kokoro, sentence, current_voice, speed, lang, &audio);
                    
                    if (err == KOKORO_SUCCESS) {
                        audio_buffer_append(output, audio.samples, audio.num_samples);
                        kokoro_audio_free(&audio);
                        sentence_count++;
                        printf("  Sentence %d: %.2fs (voice: %s)\r", sentence_count,
                               (double)output->num_samples / output->sample_rate, current_voice);
                        fflush(stdout);
                    }
                    sentence[0] = '\0';
                }
                
                /* Add custom pause */
                audio_buffer_append_silence(output, pause_duration);
                p += chars_consumed;
                continue;
            }
            
            /* Check for voice directive */
            char voice_buffer[64];
            if (parse_voice_directive(p, &chars_consumed, voice_buffer, sizeof(voice_buffer))) {
                /* Generate audio for accumulated sentence with current voice */
                if (strlen(sentence) > 0) {
                    kokoro_audio_t audio;
                    kokoro_error_t err = kokoro_create(kokoro, sentence, current_voice, speed, lang, &audio);
                    
                    if (err == KOKORO_SUCCESS) {
                        audio_buffer_append(output, audio.samples, audio.num_samples);
                        kokoro_audio_free(&audio);
                        sentence_count++;
                        printf("  Sentence %d: %.2fs (voice: %s)\r", sentence_count,
                               (double)output->num_samples / output->sample_rate, current_voice);
                        fflush(stdout);
                    }
                    sentence[0] = '\0';
                }
                
                /* Switch to new voice */
                strncpy(current_voice, voice_buffer, sizeof(current_voice) - 1);
                current_voice[sizeof(current_voice) - 1] = '\0';
                printf("\n  Voice changed to: %s\n", current_voice);
                
                p += chars_consumed;
                continue;
            }
            
            /* Accumulate characters */
            size_t len = strlen(sentence);
            if (len < sizeof(sentence) - 2) {
                sentence[len] = *p;
                sentence[len + 1] = '\0';
            }
            
            /* Check for sentence-ending punctuation (only if auto_pause enabled) */
            if (auto_pause) {
                int pause = get_punctuation_pause(*p);
                if (pause > 0 && (p[1] == '\0' || isspace(p[1]))) {
                    /* Generate audio for sentence */
                    if (strlen(sentence) > 0) {
                        kokoro_audio_t audio;
                        kokoro_error_t err = kokoro_create(kokoro, sentence, current_voice, speed, lang, &audio);
                        
                        if (err == KOKORO_SUCCESS) {
                            audio_buffer_append(output, audio.samples, audio.num_samples);
                            kokoro_audio_free(&audio);
                            sentence_count++;
                            printf("  Sentence %d: %.2fs (voice: %s)\r", sentence_count,
                                   (double)output->num_samples / output->sample_rate, current_voice);
                            fflush(stdout);
                        }
                        
                        /* Add punctuation pause */
                        audio_buffer_append_silence(output, pause);
                        sentence[0] = '\0';
                    }
                }
            }
            
            p++;
        }
        
        /* Add space between lines if sentence continues */
        if (strlen(sentence) > 0) {
            size_t len = strlen(sentence);
            if (len < sizeof(sentence) - 2) {
                sentence[len] = ' ';
                sentence[len + 1] = '\0';
            }
        }
    }
    
    /* Process any remaining sentence */
    if (strlen(sentence) > 0) {
        kokoro_audio_t audio;
        kokoro_error_t err = kokoro_create(kokoro, sentence, current_voice, speed, lang, &audio);
        
        if (err == KOKORO_SUCCESS) {
            audio_buffer_append(output, audio.samples, audio.num_samples);
            kokoro_audio_free(&audio);
            sentence_count++;
        }
    }
    
    printf("\n");
    fclose(fp);
    
    printf("Processed %d lines, %d sentences\n", line_count, sentence_count);
    return 0;
}

int main(int argc, char** argv) {
    /* Parse command-line arguments */
    const char* model_path;
    const char* voices_path;
    const char* input_path;
    const char* output_path;
    const char* voice_name;
    const char* lang;
    float speed;
    int auto_pause;
    
    int parse_result = parse_audiobook_args(
        argc, argv,
        &model_path, &voices_path, &input_path, &output_path,
        &voice_name, &lang, &speed, &auto_pause
    );
    
    if (parse_result != 0) {
        return (parse_result < 0) ? 1 : 0;  /* -1 = error, 1 = help shown */
    }
    
    printf("Kokoro Audiobook Generator\n");
    printf("==========================\n\n");
    
    /* Initialize Kokoro */
    printf("Initializing Kokoro TTS...\n");
    kokoro_t* kokoro = kokoro_init(model_path, voices_path, NULL, NULL);
    if (!kokoro) {
        fprintf(stderr, "Error: Failed to initialize Kokoro\n");
        return 1;
    }
    printf("  ✓ Initialized\n\n");
    
    printf("Configuration:\n");
    printf("  Input:      %s\n", input_path);
    printf("  Output:     %s\n", output_path);
    printf("  Voice:      %s\n", voice_name);
    printf("  Language:   %s\n", lang);
    printf("  Speed:      %.2f\n", speed);
    printf("  Auto-pause: %s\n\n", auto_pause ? "enabled" : "disabled");
    
    /* Generate audiobook */
    audio_buffer_t output;
    audio_buffer_init(&output, KOKORO_SAMPLE_RATE);
    
    if (generate_audiobook(kokoro, input_path, &output, voice_name, lang, speed, auto_pause) != 0) {
        audio_buffer_free(&output);
        kokoro_free(kokoro);
        return 1;
    }
    
    /* Save to WAV file */
    printf("\nSaving audiobook...\n");
    printf("  Duration: %.2f seconds (%.2f minutes)\n",
           (double)output.num_samples / output.sample_rate,
           (double)output.num_samples / output.sample_rate / 60.0);
    printf("  Size:     %.2f MB\n",
           (double)(output.num_samples * sizeof(int16_t)) / (1024.0 * 1024.0));
    
    if (write_wav(output_path, &output) != 0) {
        fprintf(stderr, "Error: Failed to write WAV file\n");
        audio_buffer_free(&output);
        kokoro_free(kokoro);
        return 1;
    }
    
    printf("  ✓ Saved to: %s\n\n", output_path);
    printf("Done!\n");
    
    /* Cleanup */
    audio_buffer_free(&output);
    kokoro_free(kokoro);
    
    return 0;
}
