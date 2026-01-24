# Kokoro TTS - Pure C Implementation

A pure C implementation of Kokoro TTS using ONNX Runtime C API and espeak-ng.

## Features

- Pure C implementation with no Python dependencies
- Uses ONNX Runtime C API for inference
- Uses espeak-ng for text-to-phoneme conversion
- Simple and clean API
- Cross-platform support (Linux, macOS, Windows)
- Low memory footprint
- Thread-safe design

## Dependencies

### Required

- **ONNX Runtime** (>= 1.20.1): For running the TTS model
  - Download from: https://github.com/microsoft/onnxruntime/releases
  - Or install via package manager

- **espeak-ng**: For text-to-phoneme conversion
  - Ubuntu/Debian: `sudo apt-get install espeak-ng libespeak-ng-dev`
  - macOS: `brew install espeak-ng`
  - Windows: Download from https://github.com/espeak-ng/espeak-ng/releases

### Build Tools

- CMake >= 3.15
- C compiler (GCC, Clang, or MSVC)

## Building

### Linux/macOS

```bash
cd c
mkdir build
cd build
cmake ..
make
```

### With Custom ONNX Runtime Path

If ONNX Runtime is not in a standard location:

```bash
cmake -DONNXRUNTIME_DIR=/path/to/onnxruntime ..
make
```

### Installation

```bash
sudo make install
```

This will install:
- Library: `/usr/local/lib/libkokoro.so` (or `.dylib` on macOS)
- Headers: `/usr/local/include/kokoro.h`
- CMake config: `/usr/local/lib/cmake/kokoro/`

### Verification

After building, you can run a simple compilation test:

```bash
make test
# or
./build/kokoro_compile_test
```

This verifies that the library compiles and links correctly without requiring model files.

## Usage

### Basic Example

```c
#include <kokoro.h>
#include <stdio.h>

int main() {
    // Initialize Kokoro
    kokoro_t* kokoro = kokoro_init(
        "kokoro-v1.0.onnx",
        "voices-v1.0.bin",
        NULL,  // Auto-detect espeak library
        NULL   // Auto-detect espeak data
    );
    
    if (!kokoro) {
        fprintf(stderr, "Failed to initialize\n");
        return 1;
    }
    
    // Generate audio
    kokoro_audio_t audio;
    kokoro_error_t err = kokoro_create(
        kokoro,
        "Hello, world!",
        "af_sarah",
        1.0f,      // speed
        "en-us",   // language
        &audio
    );
    
    if (err != KOKORO_SUCCESS) {
        fprintf(stderr, "Error: %s\n", kokoro_error_string(err));
        kokoro_free(kokoro);
        return 1;
    }
    
    // Use audio.samples, audio.num_samples, audio.sample_rate
    printf("Generated %zu samples at %d Hz\n", 
           audio.num_samples, audio.sample_rate);
    
    // Cleanup
    kokoro_audio_free(&audio);
    kokoro_free(kokoro);
    
    return 0;
}
```

### Running the Example

First, download the model files:

```bash
wget https://github.com/thewh1teagle/kokoro-onnx/releases/download/model-files-v1.0/kokoro-v1.0.onnx
wget https://github.com/thewh1teagle/kokoro-onnx/releases/download/model-files-v1.0/voices-v1.0.bin
```

Then run the example:

```bash
./build/kokoro_example kokoro-v1.0.onnx voices-v1.0.bin output.wav af_sarah "Hello from C!"
```

This will generate `output.wav` containing the synthesized speech.

## API Reference

### Initialization

```c
kokoro_t* kokoro_init(
    const char* model_path,
    const char* voices_path,
    const char* espeak_lib_path,
    const char* espeak_data_path
);
```

Initialize the Kokoro TTS engine. Pass `NULL` for espeak paths to auto-detect.

### Text-to-Speech

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

Generate speech from text. Speed should be between 0.5 and 2.0.

### Phoneme-to-Speech

```c
kokoro_error_t kokoro_create_from_phonemes(
    kokoro_t* kokoro,
    const char* phonemes,
    const char* voice,
    float speed,
    kokoro_audio_t* audio
);
```

Generate speech directly from IPA phonemes.

### Text-to-Phonemes

```c
kokoro_error_t kokoro_text_to_phonemes(
    kokoro_t* kokoro,
    const char* text,
    const char* lang,
    char** phonemes
);
```

Convert text to IPA phonemes using espeak-ng.

### Get Available Voices

```c
kokoro_error_t kokoro_get_voices(
    kokoro_t* kokoro,
    char*** voices,
    size_t* num_voices
);
```

Get list of available voices. Remember to free each string and the array.

### Cleanup

```c
void kokoro_audio_free(kokoro_audio_t* audio);
void kokoro_free(kokoro_t* kokoro);
```

Free audio and Kokoro resources.

## Voice File Format

The C implementation expects a custom binary format for the voice embeddings:

```
[4 bytes] number of voices (uint32_t)
[4 bytes] embedding size (uint32_t)
For each voice:
    [4 bytes] name length (uint32_t)
    [name_length bytes] voice name (UTF-8)
    [embedding_size * 4 bytes] embeddings (float32)
```

A conversion script is provided to convert from the Python NumPy format:

```bash
python scripts/convert_voices.py voices-v1.0.bin voices-v1.0-c.bin
```

## Notes

- The voice file format used by the C implementation is different from the Python version
- The C implementation uses a simplified vocabulary mapping optimized for common phonemes
- For best results, ensure espeak-ng is properly installed with language data
- The library is thread-safe after initialization

## Differences from Python Version

1. **Voice File Format**: Uses a custom binary format instead of NumPy's .npz
2. **Streaming**: Currently does not support async streaming (planned for future)
3. **Trimming**: Audio trimming is not yet implemented (planned for future)
4. **Vocabulary**: Uses simplified phoneme mapping for common IPA symbols

## Troubleshooting

### espeak-ng not found

If you get errors about espeak-ng:

1. Make sure espeak-ng is installed
2. Set `PHONEMIZER_ESPEAK_LIBRARY` environment variable to the library path
3. Pass explicit paths to `kokoro_init()`

### ONNX Runtime errors

1. Verify ONNX Runtime is installed correctly
2. Check that the model file is compatible with your ONNX Runtime version
3. Try setting `ORT_LOGGING_LEVEL=1` for more debug info

### Build errors

1. Ensure all dependencies are installed
2. Check CMake can find ONNX Runtime: `cmake -DONNXRUNTIME_DIR=/path/to/onnxruntime ..`
3. Verify compiler supports C11

## Contributing

Contributions are welcome! Please feel free to submit issues or pull requests.

## License

This C implementation follows the same license as the main kokoro-onnx project:
- Library code: MIT
- Model files: Apache 2.0
