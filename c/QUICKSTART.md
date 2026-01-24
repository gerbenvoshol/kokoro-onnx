# Quick Start Guide - Kokoro C Implementation

This guide will help you get started with the Kokoro C TTS library quickly.

## Fastest Way to Get Started

**Use the automated setup script:**

```bash
cd c/resources
./setup.sh
```

This downloads everything you need and generates a demo. Skip to [Run Example](#run-example) after this completes.

For manual setup, continue with the prerequisites below.

## Prerequisites

### Ubuntu/Debian

```bash
# Install build tools
sudo apt-get update
sudo apt-get install -y build-essential cmake

# Install espeak-ng
sudo apt-get install -y espeak-ng libespeak-ng-dev

# Download and install ONNX Runtime
wget https://github.com/microsoft/onnxruntime/releases/download/v1.20.1/onnxruntime-linux-x64-1.20.1.tgz
tar -xzf onnxruntime-linux-x64-1.20.1.tgz
sudo cp onnxruntime-linux-x64-1.20.1/lib/* /usr/local/lib/
sudo cp -r onnxruntime-linux-x64-1.20.1/include/* /usr/local/include/
sudo ldconfig
```

### macOS

```bash
# Install build tools
xcode-select --install
brew install cmake

# Install espeak-ng
brew install espeak-ng

# Download and install ONNX Runtime
wget https://github.com/microsoft/onnxruntime/releases/download/v1.20.1/onnxruntime-osx-universal2-1.20.1.tgz
tar -xzf onnxruntime-osx-universal2-1.20.1.tgz
sudo cp onnxruntime-osx-universal2-1.20.1/lib/* /usr/local/lib/
sudo cp -r onnxruntime-osx-universal2-1.20.1/include/* /usr/local/include/
```

## Building

```bash
cd c
make
```

Or using CMake directly:

```bash
cd c
mkdir build
cd build
cmake ..
make
```

## Download Model Files

**Option 1: Automated (Recommended)**

```bash
cd c/resources
./setup.sh
```

**Option 2: Manual Download**

```bash
cd c/resources

# Download ONNX model
wget https://github.com/thewh1teagle/kokoro-onnx/releases/download/model-files-v1.0/kokoro-v1.0.onnx

# Download voices file
wget https://github.com/thewh1teagle/kokoro-onnx/releases/download/model-files-v1.0/voices-v1.0.bin

# Convert voices to C format
python3 ../../scripts/convert_voices.py voices-v1.0.bin voices-v1.0-c.bin
```

## Run Example

```bash
cd c/resources
../build/kokoro_example kokoro-v1.0.onnx voices-v1.0-c.bin output.wav af_sarah "Hello from Kokoro C!"
```

This will generate `output.wav` with synthesized speech.

## Writing Your Own Program

Create a file `my_tts.c`:

```c
#include <stdio.h>
#include <stdlib.h>
#include <kokoro.h>

int main() {
    // Initialize
    kokoro_t* kokoro = kokoro_init(
        "kokoro-v1.0.onnx",
        "voices-v1.0-c.bin",
        NULL, NULL  // Auto-detect espeak
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
        1.0f,
        "en-us",
        &audio
    );
    
    if (err == KOKORO_SUCCESS) {
        printf("Generated %zu samples\n", audio.num_samples);
        
        // TODO: Save audio to file or play it
        
        kokoro_audio_free(&audio);
    } else {
        fprintf(stderr, "Error: %s\n", kokoro_error_string(err));
    }
    
    kokoro_free(kokoro);
    return 0;
}
```

Compile and link:

```bash
gcc my_tts.c -I/usr/local/include -L/usr/local/lib -lkokoro -o my_tts
./my_tts
```

## Troubleshooting

### "libonnxruntime.so: cannot open shared object file"

Add the library path:

```bash
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
```

Or add it permanently to `/etc/ld.so.conf.d/` and run `sudo ldconfig`.

### "espeak-ng initialization failed"

Make sure espeak-ng is installed and data files are in the standard location:
- Linux: `/usr/share/espeak-ng-data/`
- macOS: `/usr/local/share/espeak-ng-data/`

### CMake can't find ONNX Runtime

Specify the path explicitly:

```bash
cmake -DONNXRUNTIME_DIR=/path/to/onnxruntime ..
```

## Next Steps

- Read the full [README.md](README.md) for detailed API documentation
- Check out [example.c](examples/example.c) for a complete working example
- Explore different voices and languages

## Getting Help

If you encounter issues:
1. Check the troubleshooting section in [README.md](README.md)
2. Make sure all dependencies are properly installed
3. Verify model files are downloaded and converted correctly
4. Open an issue on GitHub with details about your error
