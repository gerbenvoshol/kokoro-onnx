# Audiobook Generation Guide

This guide explains how to use the audiobook generator example to create audiobooks from text files.

## Overview

The audiobook generator (`kokoro_audiobook`) reads a text file and converts it to speech with natural pauses at punctuation marks. It's designed for generating long-form audio content like audiobooks, articles, or stories.

## Features

- **Automatic Punctuation Pauses**: Natural pauses at periods, commas, and other punctuation
- **Paragraph Breaks**: Longer pauses between paragraphs
- **Custom Pause Directives**: Insert custom-length pauses anywhere in text
- **Memory Efficient**: Processes text sentence-by-sentence
- **Progress Display**: Real-time progress during generation
- **Configurable**: Adjust voice, language, and speed

## Building

The audiobook example is built automatically when you build the library:

```bash
cd c
make
```

Or using CMake directly:

```bash
mkdir build && cd build
cmake ..
make
```

The executable will be at `build/kokoro_audiobook`.

## Usage

### Basic Usage

```bash
./kokoro_audiobook <model.onnx> <voices.bin> <input.txt> <output.wav>
```

### With Options

```bash
./kokoro_audiobook model.onnx voices.bin story.txt audiobook.wav af_sarah en-us 1.0
```

**Arguments:**
- `model.onnx` - Path to ONNX model file
- `voices.bin` - Path to voices binary file (C format)
- `input.txt` - Input text file
- `output.wav` - Output WAV file
- `voice` - Voice name (optional, default: af_sarah)
- `lang` - Language code (optional, default: en-us)
- `speed` - Speech speed 0.5-2.0 (optional, default: 1.0)

## Text File Format

### Basic Text

Simply write your text naturally. The generator handles sentence boundaries automatically:

```text
This is the first sentence. This is the second sentence.
The third sentence continues the paragraph.
```

### Paragraph Breaks

Use empty lines to create paragraph breaks (800ms pause):

```text
First paragraph text here.

Second paragraph after a longer pause.

Third paragraph.
```

### Custom Pauses

Insert custom pauses anywhere using the `[PAUSE:duration]` directive (duration in milliseconds):

```text
This sentence will be followed by a 2 second pause. [PAUSE:2000]
Now the text continues.
```

### Example Text File

```text
The Adventure Begins

Once upon a time, in a land far away, there lived a brave knight.

[PAUSE:1000]

The knight's name was Sir Geoffrey, and he was known throughout
the kingdom for his courage and wisdom. He had defeated many foes,
rescued countless villagers, and brought peace to troubled lands.

But his greatest adventure was yet to come.

[PAUSE:1500]

One day, a messenger arrived at the castle...
```

## Automatic Pause Durations

The generator adds pauses automatically based on punctuation:

| Punctuation | Pause Duration | Use Case |
|-------------|---------------|-----------|
| `.` `!` `?` | 500ms | End of sentence |
| `,` | 250ms | Clause separator |
| `;` `:` | 350ms | List items, explanations |
| Empty line | 800ms | Paragraph break |

## Customizing Pause Durations

You can modify the pause durations by editing the constants in `audiobook.c`:

```c
#define PAUSE_PERIOD 500      /* Period/exclamation/question mark */
#define PAUSE_COMMA 250       /* Comma */
#define PAUSE_SEMICOLON 350   /* Semicolon/colon */
#define PAUSE_PARAGRAPH 800   /* Double newline (paragraph break) */
```

After modifying, rebuild the example:

```bash
make
```

## Complete Example

### 1. Prepare your text file

Create `story.txt`:

```text
The Tale of Two Cities

It was the best of times, it was the worst of times.

[PAUSE:1000]

The year was 1775, and London was bustling with activity.
The streets were filled with merchants, nobles, and common folk.

Everyone had a story to tell.
```

### 2. Download model files

```bash
wget https://github.com/thewh1teagle/kokoro-onnx/releases/download/model-files-v1.0/kokoro-v1.0.onnx
wget https://github.com/thewh1teagle/kokoro-onnx/releases/download/model-files-v1.0/voices-v1.0.bin

# Convert to C format
python3 ../scripts/convert_voices.py voices-v1.0.bin voices-v1.0-c.bin
```

### 3. Generate audiobook

```bash
./build/kokoro_audiobook kokoro-v1.0.onnx voices-v1.0-c.bin story.txt audiobook.wav
```

### 4. Play the result

```bash
# On Linux
aplay audiobook.wav

# On macOS
afplay audiobook.wav

# Or use any audio player
```

## Tips and Best Practices

### For Best Results

1. **Use proper punctuation**: The generator relies on punctuation for natural pauses
2. **Keep paragraphs reasonable**: Very long paragraphs without breaks can sound monotonous
3. **Test with short samples first**: Try a few paragraphs before generating a full book
4. **Choose appropriate voice**: Different voices work better for different content
5. **Adjust speed if needed**: Slower (0.8-0.9) for complex content, faster (1.1-1.2) for lighter material

### For Long Texts

1. **Split into chapters**: Generate each chapter separately, then concatenate
2. **Monitor memory**: The generator accumulates all audio in memory
3. **Check progress**: The generator shows progress during generation
4. **Be patient**: Long texts can take several minutes to process

### Voice Selection

Try different voices for different content types:

```bash
# Narrative fiction - warm, expressive voice
./kokoro_audiobook model.onnx voices.bin novel.txt output.wav af_bella en-us 1.0

# Technical content - clear, precise voice
./kokoro_audiobook model.onnx voices.bin manual.txt output.wav af_sarah en-us 0.9

# Children's story - friendly, engaging voice
./kokoro_audiobook model.onnx voices.bin kids_story.txt output.wav af_sky en-us 1.0
```

## Troubleshooting

### "Text too long" error

Split your text into smaller files or modify `MAX_LINE_LENGTH` in the source.

### Choppy audio

- Ensure punctuation is used appropriately
- Check that pause durations are not too short
- Try adjusting speed (slower can sound smoother)

### Memory issues with very long texts

Generate in smaller chunks:

```bash
# Split text file
split -l 1000 long_book.txt chapter_

# Generate each part
for f in chapter_*; do
    ./kokoro_audiobook model.onnx voices.bin "$f" "${f}.wav"
done

# Concatenate (using tools like ffmpeg or sox)
```

### Audio sounds robotic

- Use more natural punctuation
- Add paragraph breaks for variety
- Adjust speed (try 0.95 or 1.05)
- Try different voices

## Performance

Typical generation speeds on modern hardware:

- **Speed**: ~2-5x real-time (generates 1 minute of audio in 12-30 seconds)
- **Memory**: ~50-100MB per minute of audio
- **File size**: ~2.8MB per minute of audio (16-bit WAV, 24kHz)

Example: A 10,000-word text (~1 hour of audio) takes approximately 15-20 minutes to generate.

## Advanced Usage

### Batch Processing

Generate multiple audiobooks:

```bash
#!/bin/bash
for book in books/*.txt; do
    output="${book%.txt}.wav"
    echo "Processing $book..."
    ./kokoro_audiobook model.onnx voices.bin "$book" "$output"
done
```

### Integration with Other Tools

Convert to MP3 for smaller file size:

```bash
# Generate WAV
./kokoro_audiobook model.onnx voices.bin story.txt temp.wav

# Convert to MP3
ffmpeg -i temp.wav -codec:a libmp3lame -qscale:a 2 audiobook.mp3

# Clean up
rm temp.wav
```

## Sample Files

The distribution includes `examples/sample_story.txt` demonstrating:
- Basic text with punctuation
- Paragraph breaks
- Custom pause directives
- Natural formatting

Try it:

```bash
./build/kokoro_audiobook kokoro-v1.0.onnx voices-v1.0-c.bin \
    examples/sample_story.txt sample_audio.wav
```

## Further Reading

- [API Documentation](API.md) - Complete API reference
- [README](README.md) - General library overview
- [Integration Guide](INTEGRATION.md) - Using in your projects
