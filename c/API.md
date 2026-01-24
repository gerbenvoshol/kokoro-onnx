# Kokoro C API Documentation

Complete API reference for the Kokoro TTS C library.

## Table of Contents

- [Data Types](#data-types)
- [Error Handling](#error-handling)
- [Initialization](#initialization)
- [Audio Generation](#audio-generation)
- [Voice Management](#voice-management)
- [Cleanup](#cleanup)
- [Usage Examples](#usage-examples)

## Data Types

### `kokoro_t`

Opaque structure representing a Kokoro TTS instance. All functions operate on this type.

### `kokoro_audio_t`

Structure containing generated audio data:

```c
typedef struct {
    float* samples;       // Audio samples (mono, float32, range -1.0 to 1.0)
    size_t num_samples;   // Number of samples
    int sample_rate;      // Sample rate in Hz (typically 24000)
} kokoro_audio_t;
```

### `kokoro_error_t`

Error codes returned by API functions:

```c
typedef enum {
    KOKORO_SUCCESS = 0,              // Operation succeeded
    KOKORO_ERROR_NULL_POINTER = -1,  // NULL pointer argument
    KOKORO_ERROR_FILE_NOT_FOUND = -2, // File not found
    KOKORO_ERROR_INIT_FAILED = -3,   // Initialization failed
    KOKORO_ERROR_INFERENCE_FAILED = -4, // ONNX inference failed
    KOKORO_ERROR_INVALID_VOICE = -5, // Voice not found
    KOKORO_ERROR_TEXT_TOO_LONG = -6, // Text exceeds maximum length
    KOKORO_ERROR_INVALID_SPEED = -7, // Speed out of range
    KOKORO_ERROR_PHONEMIZE_FAILED = -8, // Phonemization failed
    KOKORO_ERROR_OUT_OF_MEMORY = -9  // Memory allocation failed
} kokoro_error_t;
```

## Error Handling

### `kokoro_error_string`

Get human-readable error message.

```c
const char* kokoro_error_string(kokoro_error_t error);
```

**Parameters:**
- `error`: Error code

**Returns:** Static string describing the error

**Example:**
```c
kokoro_error_t err = kokoro_create(...);
if (err != KOKORO_SUCCESS) {
    fprintf(stderr, "Error: %s\n", kokoro_error_string(err));
}
```

## Initialization

### `kokoro_init`

Initialize a new Kokoro TTS instance.

```c
kokoro_t* kokoro_init(
    const char* model_path,
    const char* voices_path,
    const char* espeak_lib_path,
    const char* espeak_data_path
);
```

**Parameters:**
- `model_path`: Path to ONNX model file (required)
- `voices_path`: Path to voices binary file (required)
- `espeak_lib_path`: Path to espeak-ng shared library (NULL for auto-detect)
- `espeak_data_path`: Path to espeak-ng data directory (NULL for auto-detect)

**Returns:** Pointer to initialized instance, or NULL on failure

**Example:**
```c
// Auto-detect espeak paths
kokoro_t* kokoro = kokoro_init(
    "kokoro-v1.0.onnx",
    "voices-v1.0-c.bin",
    NULL,
    NULL
);

// Or specify explicit paths
kokoro_t* kokoro = kokoro_init(
    "kokoro-v1.0.onnx",
    "voices-v1.0-c.bin",
    "/usr/lib/libespeak-ng.so",
    "/usr/share/espeak-ng-data"
);
```

**Notes:**
- This function loads the ONNX model and voice embeddings
- Initializes espeak-ng for phonemization
- May take a few seconds to complete
- Always check for NULL return value

## Audio Generation

### `kokoro_create`

Generate audio from text.

```c
kokoro_error_t kokoro_create(
    kokoro_t* kokoro,
    const char* text,
    const char* voice,
    float speed,
    const char* lang,
    kokoro_audio_t* audio
);
```

**Parameters:**
- `kokoro`: Kokoro instance
- `text`: Input text to synthesize
- `voice`: Voice name (e.g., "af_sarah")
- `speed`: Speech speed (0.5 to 2.0, where 1.0 is normal)
- `lang`: Language code (e.g., "en-us", "en-gb", "fr-fr")
- `audio`: Output audio structure (must be freed with `kokoro_audio_free`)

**Returns:** Error code (KOKORO_SUCCESS on success)

**Example:**
```c
kokoro_audio_t audio;
kokoro_error_t err = kokoro_create(
    kokoro,
    "Hello, world!",
    "af_sarah",
    1.0f,
    "en-us",
    &audio
);

if (err == KOKORO_SUCCESS) {
    // Use audio.samples, audio.num_samples, audio.sample_rate
    kokoro_audio_free(&audio);
}
```

**Notes:**
- Text is automatically converted to phonemes using espeak-ng
- Audio samples are in float32 format, range -1.0 to 1.0
- Sample rate is typically 24000 Hz
- Maximum text length is approximately 510 phonemes

### `kokoro_create_from_phonemes`

Generate audio directly from IPA phonemes.

```c
kokoro_error_t kokoro_create_from_phonemes(
    kokoro_t* kokoro,
    const char* phonemes,
    const char* voice,
    float speed,
    kokoro_audio_t* audio
);
```

**Parameters:**
- `kokoro`: Kokoro instance
- `phonemes`: IPA phonemes string
- `voice`: Voice name
- `speed`: Speech speed (0.5 to 2.0)
- `audio`: Output audio structure

**Returns:** Error code

**Example:**
```c
kokoro_audio_t audio;
kokoro_error_t err = kokoro_create_from_phonemes(
    kokoro,
    "həˈloʊ wˈɜːld",
    "af_sarah",
    1.0f,
    &audio
);
```

**Notes:**
- Bypasses text-to-phoneme conversion
- Useful for precise control or custom phonemization
- Phonemes must be in IPA format

### `kokoro_text_to_phonemes`

Convert text to IPA phonemes without generating audio.

```c
kokoro_error_t kokoro_text_to_phonemes(
    kokoro_t* kokoro,
    const char* text,
    const char* lang,
    char** phonemes
);
```

**Parameters:**
- `kokoro`: Kokoro instance
- `text`: Input text
- `lang`: Language code
- `phonemes`: Output phonemes string (caller must free)

**Returns:** Error code

**Example:**
```c
char* phonemes = NULL;
kokoro_error_t err = kokoro_text_to_phonemes(
    kokoro,
    "Hello",
    "en-us",
    &phonemes
);

if (err == KOKORO_SUCCESS) {
    printf("Phonemes: %s\n", phonemes);
    free(phonemes);
}
```

## Voice Management

### `kokoro_get_voices`

Get list of available voices.

```c
kokoro_error_t kokoro_get_voices(
    kokoro_t* kokoro,
    char*** voices,
    size_t* num_voices
);
```

**Parameters:**
- `kokoro`: Kokoro instance
- `voices`: Output array of voice name strings (caller must free)
- `num_voices`: Output number of voices

**Returns:** Error code

**Example:**
```c
char** voices;
size_t num_voices;

kokoro_error_t err = kokoro_get_voices(kokoro, &voices, &num_voices);
if (err == KOKORO_SUCCESS) {
    printf("Available voices:\n");
    for (size_t i = 0; i < num_voices; i++) {
        printf("  %s\n", voices[i]);
        free(voices[i]);
    }
    free(voices);
}
```

## Cleanup

### `kokoro_audio_free`

Free audio data.

```c
void kokoro_audio_free(kokoro_audio_t* audio);
```

**Parameters:**
- `audio`: Audio structure to free

**Example:**
```c
kokoro_audio_t audio;
kokoro_create(kokoro, "Hello", "af_sarah", 1.0f, "en-us", &audio);
// Use audio...
kokoro_audio_free(&audio);
```

**Notes:**
- Always call this after you're done with audio data
- Safe to call with zeroed/uninitialized structure

### `kokoro_free`

Free Kokoro instance and all resources.

```c
void kokoro_free(kokoro_t* kokoro);
```

**Parameters:**
- `kokoro`: Kokoro instance to free

**Example:**
```c
kokoro_t* kokoro = kokoro_init(...);
// Use kokoro...
kokoro_free(kokoro);
```

**Notes:**
- Releases ONNX Runtime resources
- Terminates espeak-ng
- Always call this when done with Kokoro
- Safe to call with NULL pointer

## Usage Examples

### Complete Example

```c
#include <stdio.h>
#include <kokoro.h>

int main() {
    // Initialize
    kokoro_t* kokoro = kokoro_init(
        "kokoro-v1.0.onnx",
        "voices-v1.0-c.bin",
        NULL, NULL
    );
    if (!kokoro) {
        fprintf(stderr, "Failed to initialize\n");
        return 1;
    }

    // Generate audio
    kokoro_audio_t audio;
    kokoro_error_t err = kokoro_create(
        kokoro,
        "Hello from Kokoro!",
        "af_sarah",
        1.0f,
        "en-us",
        &audio
    );

    if (err != KOKORO_SUCCESS) {
        fprintf(stderr, "Error: %s\n", kokoro_error_string(err));
        kokoro_free(kokoro);
        return 1;
    }

    // Process audio
    printf("Generated %zu samples at %d Hz\n",
           audio.num_samples, audio.sample_rate);
    
    // TODO: Save to file or play audio
    
    // Cleanup
    kokoro_audio_free(&audio);
    kokoro_free(kokoro);
    return 0;
}
```

### Multiple Syntheses

```c
kokoro_t* kokoro = kokoro_init(...);

const char* texts[] = {
    "First sentence.",
    "Second sentence.",
    "Third sentence."
};

for (int i = 0; i < 3; i++) {
    kokoro_audio_t audio;
    kokoro_error_t err = kokoro_create(
        kokoro, texts[i], "af_sarah", 1.0f, "en-us", &audio
    );
    
    if (err == KOKORO_SUCCESS) {
        // Process audio...
        kokoro_audio_free(&audio);
    }
}

kokoro_free(kokoro);
```

### Different Voices and Speeds

```c
kokoro_t* kokoro = kokoro_init(...);

// Slow speech
kokoro_audio_t slow;
kokoro_create(kokoro, "Speaking slowly", "af_sarah", 0.7f, "en-us", &slow);

// Normal speech
kokoro_audio_t normal;
kokoro_create(kokoro, "Speaking normally", "af_sarah", 1.0f, "en-us", &normal);

// Fast speech
kokoro_audio_t fast;
kokoro_create(kokoro, "Speaking quickly", "af_sarah", 1.5f, "en-us", &fast);

// Different voice
kokoro_audio_t other_voice;
kokoro_create(kokoro, "Different voice", "af_bella", 1.0f, "en-us", &other_voice);

// Cleanup
kokoro_audio_free(&slow);
kokoro_audio_free(&normal);
kokoro_audio_free(&fast);
kokoro_audio_free(&other_voice);
kokoro_free(kokoro);
```

## Thread Safety

- `kokoro_init` and `kokoro_free` are NOT thread-safe
- Multiple threads can call synthesis functions on the same instance
- ONNX Runtime handles thread safety internally
- espeak-ng phonemization may require synchronization

## Memory Management

- Always free returned strings with `free()`
- Always free audio with `kokoro_audio_free()`
- Always free Kokoro instance with `kokoro_free()`
- Check return values for NULL/errors before accessing data

## Performance Notes

- Initialization is slow (1-3 seconds) - do it once
- Synthesis is fast (near real-time on modern CPUs)
- GPU acceleration available if ONNX Runtime supports it
- Consider using `kokoro_create_from_phonemes` for batch processing

## Language Support

Supported language codes (partial list):
- `en-us` - English (US)
- `en-gb` - English (UK)
- `fr-fr` - French
- `es-es` - Spanish
- `de-de` - German
- `it-it` - Italian
- `ja` - Japanese
- `zh` - Chinese
- `ko` - Korean
- `hi` - Hindi

See espeak-ng documentation for complete list.
