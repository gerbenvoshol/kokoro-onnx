# Troubleshooting Guide

Common issues and solutions for Kokoro C implementation.

## Compilation Issues

### Issue: "espeak-ng/speak_lib.h: No such file or directory"

**Cause:** espeak-ng development headers not installed.

**Solution:**
```bash
# Ubuntu/Debian
sudo apt-get install espeak-ng libespeak-ng-dev

# macOS
brew install espeak-ng
```

### Issue: "ONNX Runtime library not found"

**Cause:** ONNX Runtime not installed or not in library path.

**Solution:**
See [QUICKSTART.md](QUICKSTART.md) for installation instructions, or:
```bash
# Set ONNXRUNTIME_DIR environment variable
export ONNXRUNTIME_DIR=/path/to/onnxruntime
cmake -DONNXRUNTIME_DIR=/path/to/onnxruntime ..
```

## Runtime Issues

### Issue: "Error: Not enough arguments"

**Cause:** Missing required command-line arguments.

**Solution:** The example requires **3 file arguments**:
```bash
./kokoro_example <model.onnx> <voices.bin> <output.wav> [voice] [text]
```

Example:
```bash
./kokoro_example resources/kokoro-v1.0.onnx resources/voices-v1.0-c.bin output.wav af_sarah "Hello"
```

**Common mistake:** Forgetting the voices file argument:
```bash
# WRONG - missing voices file
./kokoro_example resources/kokoro-v1.0.onnx output.wav af_sarah "Hello"

# CORRECT - all 3 files specified
./kokoro_example resources/kokoro-v1.0.onnx resources/voices-v1.0-c.bin output.wav af_sarah "Hello"
```

### Issue: "Cannot open voices file"

**Cause:** Voices file doesn't exist or wrong format.

**Solution:** 
1. Make sure you have the voices file
2. **Important:** Use the C binary format, not the Python NumPy format

```bash
# Download Python format
wget https://github.com/thewh1teagle/kokoro-onnx/releases/download/model-files-v1.0/voices-v1.0.bin

# Convert to C format
python3 ../scripts/convert_voices.py voices-v1.0.bin voices-v1.0-c.bin

# Use the C format file
./kokoro_example model.onnx voices-v1.0-c.bin output.wav
```

### Issue: "Failed to initialize Kokoro"

**Possible causes:**

1. **Wrong voices file format**
   - Solution: Use `voices-v1.0-c.bin` (C format), not `voices-v1.0.bin` (Python format)
   - Convert with: `python3 ../scripts/convert_voices.py voices-v1.0.bin voices-v1.0-c.bin`

2. **Corrupted model or voices file**
   - Solution: Re-download the files
   - Verify file sizes: model ~300MB, voices ~80MB

3. **espeak-ng not installed**
   - Solution: Install espeak-ng (see compilation issues above)

4. **Insufficient memory**
   - Solution: Close other applications or use a machine with more RAM

### Issue: "Voice not found in available voices"

**Cause:** Typo in voice name or voice doesn't exist.

**Solution:** 
1. Check available voices: Run the example without voice argument to see list
2. Common voices: `af_sarah`, `af_bella`, `af_sky`, `am_adam`, `am_michael`
3. Voice names are case-sensitive

### Issue: "Text too long" or "Phonemes too long"

**Cause:** Input text exceeds maximum length (510 phonemes after conversion).

**Solution:**
1. Split text into smaller chunks
2. Use the audiobook example for long texts: `kokoro_audiobook`
3. The audiobook example automatically handles text splitting

## File Path Issues

### Issue: "Cannot open model file" even though file exists

**Causes:**
1. Incorrect relative path
2. Running from wrong directory
3. File permissions

**Solutions:**
```bash
# Use absolute paths
./kokoro_example /full/path/to/kokoro-v1.0.onnx /full/path/to/voices-v1.0-c.bin output.wav

# Or run from correct directory
cd /path/to/kokoro-onnx/c
./build/kokoro_example resources/kokoro-v1.0.onnx resources/voices-v1.0-c.bin output.wav

# Check file permissions
ls -l resources/kokoro-v1.0.onnx
chmod 644 resources/kokoro-v1.0.onnx  # if needed
```

## Resource Setup Issues

### Issue: Resources directory is empty or files missing

**Solution:** Use the setup script:
```bash
cd resources
./setup.sh
```

This will:
- Download model files (~300MB)
- Download voices files (~80MB)
- Convert voices to C format
- Generate demo audio

### Issue: "setup.sh: Permission denied"

**Solution:**
```bash
chmod +x resources/setup.sh
./resources/setup.sh
```

## Audio Output Issues

### Issue: Output WAV file is empty or corrupted

**Possible causes:**
1. Wrong voice name
2. Text too long
3. Disk full
4. File permissions

**Solution:**
1. Check voice name is correct
2. Try shorter text first
3. Check disk space: `df -h`
4. Check write permissions in output directory

### Issue: No audio or garbled audio when playing WAV file

**Possible causes:**
1. Audio player doesn't support 24kHz mono WAV
2. File not fully written

**Solution:**
1. Try different audio player:
   ```bash
   # Linux
   aplay output.wav
   
   # macOS
   afplay output.wav
   
   # Convert to common format
   ffmpeg -i output.wav -ar 44100 output_converted.wav
   ```

2. Check file size: `ls -lh output.wav` (should not be 0 bytes)

## Performance Issues

### Issue: Synthesis is very slow

**Possible causes:**
1. Running on low-powered hardware
2. Model not optimized for your CPU
3. Using debug build

**Solutions:**
1. Use release build:
   ```bash
   cd build
   cmake -DCMAKE_BUILD_TYPE=Release ..
   make
   ```

2. Close other applications

3. For long texts, use audiobook example which shows progress

## Getting Help

If you encounter an issue not listed here:

1. Check error messages carefully
2. Verify all prerequisites are installed
3. Try the examples in order: compile_test → example → audiobook
4. Review documentation:
   - [README.md](README.md) - Overview
   - [QUICKSTART.md](QUICKSTART.md) - Setup guide
   - [API.md](API.md) - API reference
   - [AUDIOBOOK.md](AUDIOBOOK.md) - Audiobook guide

5. Report issues with:
   - Full error message
   - Command you ran
   - OS and version
   - Output of `./kokoro_compile_test`

## Quick Reference

### Correct Example Usage

```bash
# New flag-based usage (recommended)
./kokoro_example -m MODEL -v VOICES -o OUTPUT [OPTIONS]

# With actual files using flags
./kokoro_example \
    -m resources/kokoro-v1.0.onnx \
    -v resources/voices-v1.0-c.bin \
    -o output.wav \
    -V af_sarah \
    -t "Hello world"

# With custom speed
./kokoro_example \
    -m resources/kokoro-v1.0.onnx \
    -v resources/voices-v1.0-c.bin \
    -o output.wav \
    -s 1.5

# Get help
./kokoro_example --help

# Backward compatible positional arguments (still works)
./kokoro_example MODEL VOICES OUTPUT [VOICE] [TEXT] [SPEED]
./kokoro_example resources/kokoro-v1.0.onnx resources/voices-v1.0-c.bin output.wav
./kokoro_example resources/kokoro-v1.0.onnx resources/voices-v1.0-c.bin output.wav af_sarah "Hello world"
```

### Available Options

**Using flags (recommended):**
- `-m, --model MODEL` - ONNX model file (required)
- `-v, --voices VOICES` - Voices in C format (required)
- `-o, --output OUTPUT` - Output file path (required)
- `-V, --voice VOICE` - Voice name like "af_sarah" (optional, default: af_sarah)
- `-t, --text TEXT` - Text to synthesize (optional, default message)
- `-s, --speed SPEED` - Speech speed 0.5-2.0 (optional, default: 1.0)
- `-l, --lang LANG` - Language code (optional, default: en-us)
- `-h, --help` - Show help message

**Using positional arguments (backward compatible):**
1. **model.onnx** - ONNX model file (required)
2. **voices.bin** - Voices in C format (required)
3. **output.wav** - Output file path (required)
4. **voice** - Voice name (optional, default: af_sarah)
5. **text** - Text to synthesize (optional)
6. **speed** - Speech speed 0.5-2.0 (optional, default: 1.0)

### File Format Requirements

- **Model file**: `.onnx` format (~300MB)
- **Voices file**: C binary format (`.bin`, ~80MB)
  - Must be converted from Python format: `voices-v1.0-c.bin`
  - **Not** the Python format: `voices-v1.0.bin`
- **Output file**: Any path, will be created as WAV format

### Resource Locations

After setup, files should be in:
```
c/resources/
├── kokoro-v1.0.onnx         # Model file
├── voices-v1.0.bin          # Python format (for conversion)
└── voices-v1.0-c.bin        # C format (use this one!)
```
