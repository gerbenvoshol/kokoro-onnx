/**
 * Kokoro TTS - Pure C Implementation
 * 
 * A C interface for Kokoro TTS using ONNX Runtime
 */

#ifndef KOKORO_H
#define KOKORO_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Constants */
/* Maximum phoneme length (510) is determined by the model's context window size.
 * The model can process up to 512 tokens, but we reserve 2 for padding tokens
 * (one at the start and one at the end), leaving 510 for actual phonemes. */
#define KOKORO_MAX_PHONEME_LENGTH 510
#define KOKORO_SAMPLE_RATE 24000

/* Error codes */
typedef enum {
    KOKORO_SUCCESS = 0,
    KOKORO_ERROR_NULL_POINTER = -1,
    KOKORO_ERROR_FILE_NOT_FOUND = -2,
    KOKORO_ERROR_INIT_FAILED = -3,
    KOKORO_ERROR_INFERENCE_FAILED = -4,
    KOKORO_ERROR_INVALID_VOICE = -5,
    KOKORO_ERROR_TEXT_TOO_LONG = -6,
    KOKORO_ERROR_INVALID_SPEED = -7,
    KOKORO_ERROR_PHONEMIZE_FAILED = -8,
    KOKORO_ERROR_OUT_OF_MEMORY = -9
} kokoro_error_t;

/* Opaque structure for Kokoro instance */
typedef struct kokoro_t kokoro_t;

/* Audio output structure */
typedef struct {
    float* samples;       /* Audio samples (mono, float32) */
    size_t num_samples;   /* Number of samples */
    int sample_rate;      /* Sample rate (typically 24000) */
} kokoro_audio_t;

/**
 * Initialize Kokoro TTS engine
 * 
 * @param model_path Path to the ONNX model file
 * @param voices_path Path to the voices binary file
 * @param espeak_lib_path Path to espeak-ng shared library (NULL for auto-detect)
 * @param espeak_data_path Path to espeak-ng data directory (NULL for auto-detect)
 * @return Pointer to kokoro instance or NULL on failure
 */
kokoro_t* kokoro_init(
    const char* model_path,
    const char* voices_path,
    const char* espeak_lib_path,
    const char* espeak_data_path
);

/**
 * Free Kokoro TTS engine and all associated resources
 * 
 * @param kokoro Kokoro instance to free
 */
void kokoro_free(kokoro_t* kokoro);

/**
 * Generate audio from text
 * 
 * @param kokoro Kokoro instance
 * @param text Input text to convert to speech
 * @param voice Voice name (e.g., "af_sarah")
 * @param speed Speech speed (0.5 to 2.0, default 1.0)
 * @param lang Language code (e.g., "en-us")
 * @param audio Output audio structure (caller must free with kokoro_audio_free)
 * @return Error code (KOKORO_SUCCESS on success)
 */
kokoro_error_t kokoro_create(
    kokoro_t* kokoro,
    const char* text,
    const char* voice,
    float speed,
    const char* lang,
    kokoro_audio_t* audio
);

/**
 * Generate audio from phonemes (IPA)
 * 
 * @param kokoro Kokoro instance
 * @param phonemes Input phonemes (IPA format)
 * @param voice Voice name (e.g., "af_sarah")
 * @param speed Speech speed (0.5 to 2.0, default 1.0)
 * @param audio Output audio structure (caller must free with kokoro_audio_free)
 * @return Error code (KOKORO_SUCCESS on success)
 */
kokoro_error_t kokoro_create_from_phonemes(
    kokoro_t* kokoro,
    const char* phonemes,
    const char* voice,
    float speed,
    kokoro_audio_t* audio
);

/**
 * Convert text to phonemes using espeak-ng
 * 
 * @param kokoro Kokoro instance
 * @param text Input text
 * @param lang Language code (e.g., "en-us")
 * @param phonemes Output buffer for phonemes (caller must free)
 * @return Error code (KOKORO_SUCCESS on success)
 */
kokoro_error_t kokoro_text_to_phonemes(
    kokoro_t* kokoro,
    const char* text,
    const char* lang,
    char** phonemes
);

/**
 * Get list of available voices
 * 
 * @param kokoro Kokoro instance
 * @param voices Output array of voice names (caller must free each string and array)
 * @param num_voices Output number of voices
 * @return Error code (KOKORO_SUCCESS on success)
 */
kokoro_error_t kokoro_get_voices(
    kokoro_t* kokoro,
    char*** voices,
    size_t* num_voices
);

/**
 * Free audio structure
 * 
 * @param audio Audio structure to free
 */
void kokoro_audio_free(kokoro_audio_t* audio);

/**
 * Get error message for error code
 * 
 * @param error Error code
 * @return Error message string
 */
const char* kokoro_error_string(kokoro_error_t error);

#ifdef __cplusplus
}
#endif

#endif /* KOKORO_H */
