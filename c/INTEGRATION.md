# Integration Guide

How to integrate Kokoro C library into your project.

## Using CMake

### Method 1: System Installation

If you've installed Kokoro system-wide:

```cmake
cmake_minimum_required(VERSION 3.15)
project(my_app)

# Find Kokoro
find_package(kokoro REQUIRED)

add_executable(my_app main.c)
target_link_libraries(my_app PRIVATE kokoro::kokoro)
```

### Method 2: Add as Subdirectory

```cmake
cmake_minimum_required(VERSION 3.15)
project(my_app)

# Add Kokoro as subdirectory
add_subdirectory(path/to/kokoro-onnx/c)

add_executable(my_app main.c)
target_link_libraries(my_app PRIVATE kokoro)
target_include_directories(my_app PRIVATE path/to/kokoro-onnx/c/include)
```

### Method 3: FetchContent

```cmake
cmake_minimum_required(VERSION 3.15)
project(my_app)

include(FetchContent)

FetchContent_Declare(
    kokoro
    GIT_REPOSITORY https://github.com/gerbenvoshol/kokoro-onnx.git
    GIT_TAG main
    SOURCE_SUBDIR c
)

FetchContent_MakeAvailable(kokoro)

add_executable(my_app main.c)
target_link_libraries(my_app PRIVATE kokoro)
```

## Using pkg-config

After system installation, you can use pkg-config:

```bash
gcc main.c $(pkg-config --cflags --libs kokoro) -o my_app
```

## Manual Compilation

### Linux/macOS

```bash
# Compile
gcc -c main.c -I/usr/local/include -o main.o

# Link
gcc main.o -L/usr/local/lib -lkokoro -lonnxruntime -lespeak-ng -lm -o my_app

# Run (may need to set library path)
LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH ./my_app
```

### Windows (MSVC)

```batch
REM Compile
cl /c main.c /I"C:\Program Files\kokoro\include"

REM Link
link main.obj /LIBPATH:"C:\Program Files\kokoro\lib" kokoro.lib onnxruntime.lib

REM Run
set PATH=%PATH%;C:\Program Files\kokoro\bin
my_app.exe
```

## Using as Header-Only (Not Recommended)

If you can't or don't want to build a library:

1. Add `kokoro.c` directly to your project sources
2. Include necessary headers
3. Link against ONNX Runtime and espeak-ng

```cmake
add_executable(my_app
    main.c
    path/to/kokoro.c
)

target_include_directories(my_app PRIVATE
    path/to/kokoro/include
    ${ONNXRUNTIME_INCLUDE_DIR}
    ${ESPEAK_INCLUDE_DIR}
)

target_link_libraries(my_app PRIVATE
    ${ONNXRUNTIME_LIB}
    ${ESPEAK_LIB}
    m
)
```

## Example Integration Projects

### Simple CLI Tool

```c
// tts_cli.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kokoro.h>

void save_wav(const char* filename, const kokoro_audio_t* audio);

int main(int argc, char** argv) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <model> <voices> <text> <output.wav>\n", argv[0]);
        return 1;
    }

    kokoro_t* kokoro = kokoro_init(argv[1], argv[2], NULL, NULL);
    if (!kokoro) {
        fprintf(stderr, "Failed to initialize\n");
        return 1;
    }

    kokoro_audio_t audio;
    if (kokoro_create(kokoro, argv[3], "af_sarah", 1.0f, "en-us", &audio) == KOKORO_SUCCESS) {
        save_wav(argv[4], &audio);
        kokoro_audio_free(&audio);
    }

    kokoro_free(kokoro);
    return 0;
}
```

### C++ Wrapper

```cpp
// kokoro_wrapper.hpp
#pragma once
#include <kokoro.h>
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>

class Kokoro {
public:
    Kokoro(const std::string& model_path, const std::string& voices_path) {
        handle_ = kokoro_init(model_path.c_str(), voices_path.c_str(), nullptr, nullptr);
        if (!handle_) {
            throw std::runtime_error("Failed to initialize Kokoro");
        }
    }

    ~Kokoro() {
        if (handle_) {
            kokoro_free(handle_);
        }
    }

    // Prevent copying
    Kokoro(const Kokoro&) = delete;
    Kokoro& operator=(const Kokoro&) = delete;

    std::vector<float> synthesize(
        const std::string& text,
        const std::string& voice = "af_sarah",
        float speed = 1.0f,
        const std::string& lang = "en-us"
    ) {
        kokoro_audio_t audio;
        auto err = kokoro_create(handle_, text.c_str(), voice.c_str(), 
                                  speed, lang.c_str(), &audio);
        
        if (err != KOKORO_SUCCESS) {
            throw std::runtime_error(kokoro_error_string(err));
        }

        std::vector<float> result(audio.samples, audio.samples + audio.num_samples);
        kokoro_audio_free(&audio);
        return result;
    }

    std::vector<std::string> getVoices() {
        char** voices;
        size_t num_voices;
        
        auto err = kokoro_get_voices(handle_, &voices, &num_voices);
        if (err != KOKORO_SUCCESS) {
            throw std::runtime_error(kokoro_error_string(err));
        }

        std::vector<std::string> result;
        for (size_t i = 0; i < num_voices; i++) {
            result.emplace_back(voices[i]);
            free(voices[i]);
        }
        free(voices);
        return result;
    }

private:
    kokoro_t* handle_;
};
```

### Multi-threaded Server

```c
// tts_server.c
#include <pthread.h>
#include <kokoro.h>

typedef struct {
    kokoro_t* kokoro;
    pthread_mutex_t lock;
} tts_server_t;

tts_server_t* server_init(const char* model, const char* voices) {
    tts_server_t* server = malloc(sizeof(tts_server_t));
    server->kokoro = kokoro_init(model, voices, NULL, NULL);
    pthread_mutex_init(&server->lock, NULL);
    return server;
}

kokoro_error_t server_synthesize(tts_server_t* server, const char* text,
                                   const char* voice, float speed,
                                   kokoro_audio_t* audio) {
    pthread_mutex_lock(&server->lock);
    kokoro_error_t err = kokoro_create(server->kokoro, text, voice, speed, "en-us", audio);
    pthread_mutex_unlock(&server->lock);
    return err;
}

void server_free(tts_server_t* server) {
    kokoro_free(server->kokoro);
    pthread_mutex_destroy(&server->lock);
    free(server);
}
```

## Deployment Considerations

### Bundling Dependencies

When distributing your application:

1. **ONNX Runtime**: Either bundle the shared library or require installation
2. **espeak-ng**: Either bundle or require system installation
3. **Model files**: Include in your distribution or download on first run

### Directory Structure

```
my_app/
├── bin/
│   └── my_app
├── lib/
│   ├── libkokoro.so
│   ├── libonnxruntime.so
│   └── libespeak-ng.so
├── share/
│   ├── models/
│   │   ├── kokoro-v1.0.onnx
│   │   └── voices-v1.0-c.bin
│   └── espeak-ng-data/
└── README.md
```

### Runtime Library Path

Set the library path at runtime:

```c
#include <stdlib.h>

int main() {
    // Set library path (Linux)
    setenv("LD_LIBRARY_PATH", "./lib", 1);
    
    // Initialize with bundled files
    kokoro_t* kokoro = kokoro_init(
        "./share/models/kokoro-v1.0.onnx",
        "./share/models/voices-v1.0-c.bin",
        "./lib/libespeak-ng.so",
        "./share/espeak-ng-data"
    );
    
    // ...
}
```

Or use rpath (recommended):

```cmake
set_target_properties(my_app PROPERTIES
    INSTALL_RPATH "$ORIGIN/../lib"
    BUILD_WITH_INSTALL_RPATH TRUE
)
```

## Docker Integration

Example Dockerfile:

```dockerfile
FROM ubuntu:22.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    espeak-ng \
    libespeak-ng-dev \
    wget

# Install ONNX Runtime
RUN wget https://github.com/microsoft/onnxruntime/releases/download/v1.20.1/onnxruntime-linux-x64-1.20.1.tgz && \
    tar -xzf onnxruntime-linux-x64-1.20.1.tgz && \
    cp onnxruntime-linux-x64-1.20.1/lib/* /usr/local/lib/ && \
    cp -r onnxruntime-linux-x64-1.20.1/include/* /usr/local/include/ && \
    ldconfig

# Copy and build Kokoro
COPY . /kokoro
WORKDIR /kokoro/c
RUN mkdir build && cd build && cmake .. && make && make install

# Copy your application
COPY my_app.c /app/
WORKDIR /app

# Build your app
RUN gcc my_app.c -lkokoro -o my_app

CMD ["./my_app"]
```

## Static Linking

For fully static binaries (Linux):

```cmake
set(CMAKE_EXE_LINKER_FLAGS "-static")
target_link_libraries(my_app PRIVATE
    kokoro
    -static-libgcc
    -static-libstdc++
)
```

Note: This requires static versions of all dependencies.

## Troubleshooting Integration

### "undefined reference to `kokoro_init`"

- Not linking against libkokoro
- Add `-lkokoro` to link flags

### "cannot open shared object file"

- Library not in standard path
- Set `LD_LIBRARY_PATH` or use rpath

### "espeak initialization failed"

- espeak-ng not found or data missing
- Specify explicit paths in `kokoro_init`

### Header not found

- Include directory not specified
- Add `-I/path/to/include` to compiler flags

## Best Practices

1. **Initialize once**: Kokoro initialization is slow, do it once per application
2. **Thread safety**: Protect concurrent access with mutexes if needed
3. **Error checking**: Always check return values
4. **Memory management**: Always free allocated resources
5. **Model files**: Don't hardcode paths, make them configurable
6. **Logging**: Implement proper error logging for production

## Support

For integration issues:
- Check the [API documentation](API.md)
- Review [examples](examples/)
- Open an issue on GitHub with:
  - Your build system (CMake/Make/manual)
  - Platform (Linux/macOS/Windows)
  - Error messages
  - Minimal reproduction code
