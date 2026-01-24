# Kokoro TTS Resources

This directory contains scripts to download the required model files and generate demo audio.

## Quick Start

Run the setup script to download everything you need:

```bash
cd c/resources
./setup.sh
```

This will:
1. Download the ONNX model file (~300MB)
2. Download the voices file (~80MB)
3. Convert voices to C binary format
4. Build the examples (if not already built)
5. Generate a demo WAV file

## What Gets Downloaded

### Model Files

| File | Size | Description |
|------|------|-------------|
| `kokoro-v1.0.onnx` | ~300MB | ONNX TTS model |
| `voices-v1.0.bin` | ~80MB | Voice embeddings (NumPy format) |
| `voices-v1.0-c.bin` | ~80MB | Voice embeddings (C binary format) |

### Generated Files

| File | Description |
|------|-------------|
| `demo.wav` | Demo audio file with sample text |

## Manual Download

If you prefer to download manually:

### 1. Download Model

```bash
wget https://github.com/thewh1teagle/kokoro-onnx/releases/download/model-files-v1.0/kokoro-v1.0.onnx
```

Or with curl:

```bash
curl -L -O https://github.com/thewh1teagle/kokoro-onnx/releases/download/model-files-v1.0/kokoro-v1.0.onnx
```

### 2. Download Voices

```bash
wget https://github.com/thewh1teagle/kokoro-onnx/releases/download/model-files-v1.0/voices-v1.0.bin
```

Or with curl:

```bash
curl -L -O https://github.com/thewh1teagle/kokoro-onnx/releases/download/model-files-v1.0/voices-v1.0.bin
```

### 3. Convert Voices to C Format

```bash
python3 ../../scripts/convert_voices.py voices-v1.0.bin voices-v1.0-c.bin
```

## Generate Demo Audio

After downloading the files and building the examples:

```bash
../build/kokoro_example kokoro-v1.0.onnx voices-v1.0-c.bin demo.wav af_sarah "Hello world"
```

## Usage Examples

### Basic TTS

```bash
cd c/resources
../build/kokoro_example kokoro-v1.0.onnx voices-v1.0-c.bin output.wav af_sarah "Your text here"
```

### Generate Audiobook

```bash
cd c/resources
../build/kokoro_audiobook kokoro-v1.0.onnx voices-v1.0-c.bin ../examples/sample_story.txt audiobook.wav
```

### Custom Voice and Speed

```bash
cd c/resources
../build/kokoro_example kokoro-v1.0.onnx voices-v1.0-c.bin output.wav af_bella "Fast speech" 1.5
```

## Available Voices

The voices file contains multiple voices for different languages. Common voices include:

- `af_sarah` - Female, clear and natural (default)
- `af_bella` - Female, warm and expressive
- `af_sky` - Female, friendly
- `am_adam` - Male, professional
- `am_michael` - Male, deep

To list all available voices, use the example program without text.

## File Locations

After running the setup script, files will be located at:

```
c/resources/
├── setup.sh                  # This setup script
├── README.md                 # This file
├── kokoro-v1.0.onnx         # ONNX model (downloaded)
├── voices-v1.0.bin          # Voices NumPy format (downloaded)
├── voices-v1.0-c.bin        # Voices C format (generated)
└── demo.wav                  # Demo audio (generated)
```

## Disk Space Requirements

- **Total Download**: ~380MB
- **After Conversion**: ~460MB
- **With Demo**: ~465MB

## Troubleshooting

### Download Failed

- Check your internet connection
- Try the manual download method
- Some networks may block large file downloads

### Conversion Failed

- Ensure Python 3 is installed: `python3 --version`
- Ensure NumPy is installed: `pip3 install numpy`
- Check that `scripts/convert_voices.py` exists

### Demo Generation Failed

- Build the project first: `cd .. && make`
- Verify model files are downloaded
- Check that voice file is converted to C format

### Out of Disk Space

The model files are large. Ensure you have at least 1GB free space.

## Cleanup

To remove downloaded files:

```bash
cd c/resources
rm -f kokoro-v1.0.onnx voices-v1.0.bin voices-v1.0-c.bin demo.wav
```

To re-download everything:

```bash
cd c/resources
rm -f kokoro-v1.0.onnx voices-v1.0.bin voices-v1.0-c.bin demo.wav
./setup.sh
```

## Model Information

### Kokoro TTS Model

- **Version**: 1.0
- **Architecture**: 82M parameters
- **Languages**: Multiple (English, Spanish, French, German, Japanese, Chinese, etc.)
- **Sample Rate**: 24kHz
- **Format**: ONNX (optimized for CPU and GPU)

### License

- **Model**: Apache 2.0 License
- **Code**: MIT License

See the main repository for full license details.

## Support

For issues or questions:
- GitHub Issues: https://github.com/thewh1teagle/kokoro-onnx/issues
- Documentation: See `../README.md`, `../API.md`, `../QUICKSTART.md`

## Advanced Usage

### Use Custom Voice Files

If you have custom voice embeddings:

```bash
# Convert your custom voices
python3 ../../scripts/convert_voices.py your-voices.bin your-voices-c.bin

# Use with examples
../build/kokoro_example kokoro-v1.0.onnx your-voices-c.bin output.wav voice_name "Text"
```

### Batch Processing

Generate multiple audio files:

```bash
#!/bin/bash
for text in "First" "Second" "Third"; do
    ../build/kokoro_example kokoro-v1.0.onnx voices-v1.0-c.bin "${text}.wav" af_sarah "$text"
done
```

### Integration

Copy the model files to your application directory:

```bash
cp kokoro-v1.0.onnx voices-v1.0-c.bin /path/to/your/app/
```

Then use them in your application code.
