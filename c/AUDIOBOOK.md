# Audiobook Generation Guide

This guide explains how to use the audiobook generator example to create audiobooks from text files.

## Overview

The audiobook generator (`kokoro_audiobook`) reads a text file and converts it to speech with natural pauses at punctuation marks. It's designed for generating long-form audio content like audiobooks, articles, or stories.

## Features

- **Automatic Punctuation Pauses**: Natural pauses at periods, commas, and other punctuation (can be disabled)
- **Paragraph Breaks**: Longer pauses between paragraphs
- **Custom Pause Directives**: Insert custom-length pauses anywhere in text
- **Inline Voice Switching**: Change voices dynamically within the text using `[voice_name]`
- **Manual Pause Control**: `--no-auto-pause` flag for complete manual control
- **Long Text Support**: Automatically handles sentences of any length with intelligent batching
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
./kokoro_audiobook -m model.onnx -v voices.bin -i input.txt -o output.wav
```

### With Options

```bash
./kokoro_audiobook -m model.onnx -v voices.bin -i story.txt -o audiobook.wav -V af_sarah -l en-us -s 1.0
```

### Disable Automatic Pauses

```bash
./kokoro_audiobook -m model.onnx -v voices.bin -i story.txt -o audiobook.wav --no-auto-pause
```

**Command-Line Options:**
- `-m, --model` - Path to ONNX model file (required)
- `-v, --voices` - Path to voices binary file, C format (required)
- `-i, --input` - Input text file (required)
- `-o, --output` - Output WAV file (required)
- `-V, --voice` - Initial voice name (optional, default: af_sarah)
- `-l, --lang` - Language code (optional, default: en-us)
- `-s, --speed` - Speech speed 0.5-2.0 (optional, default: 1.0)
- `-n, --no-auto-pause` - Disable automatic pauses at punctuation (optional)
- `-h, --help` - Show help message

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

Insert custom pauses anywhere using the `[PAUSE:duration]` directive (duration in milliseconds). 

**Note:** Custom pauses work regardless of the `--no-auto-pause` setting.

```text
This sentence will be followed by a 2 second pause. [PAUSE:2000]
Now the text continues.
```

### Inline Voice Switching

Change voices dynamically within your text using `[voice_name]` directives:

```text
[af_sarah]Sarah is narrating this part of the story.

[af_bella]Now Bella takes over with her voice.

[am_adam]And Adam speaks with a deep male voice.
```

This is perfect for:
- **Dialogue**: Different characters with different voices
- **Narration styles**: Change tone for different sections
- **Multi-speaker content**: Podcasts, plays, or interviews

### Combining Features

You can combine voice switching, custom pauses, and automatic punctuation:

```text
[af_sarah]The narrator began, "Hello there!"

[PAUSE:1000]

[af_bella]"Who's there?" Bella asked nervously.

[af_sarah]There was a long silence. [PAUSE:2000] Then footsteps.
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

When automatic pauses are **enabled** (default), the generator adds pauses based on punctuation:

| Punctuation | Pause Duration | Use Case |
|-------------|---------------|-----------|
| `.` `!` `?` | 500ms | End of sentence |
| `,` | 250ms | Clause separator |
| `;` `:` | 350ms | List items, explanations |
| Empty line | 800ms | Paragraph break |

When automatic pauses are **disabled** (`--no-auto-pause`):
- No automatic pauses at punctuation
- No automatic paragraph break pauses  
- Custom `[PAUSE:ms]` directives still work
- Inline voice changes `[voice_name]` still work
- Gives complete manual control over timing

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

### Example 1: Basic Audiobook

Create `story.txt`:

```text
The Tale of Two Cities

It was the best of times, it was the worst of times.

[PAUSE:1000]

The year was 1775, and London was bustling with activity.
The streets were filled with merchants, nobles, and common folk.

Everyone had a story to tell.
```

Generate audiobook:

```bash
./kokoro_audiobook -m model.onnx -v voices.bin -i story.txt -o audiobook.wav
```

### Example 2: Multi-Voice Dialogue

Create `dialogue.txt`:

```text
A Conversation

[af_sarah]"Good morning!" Sarah greeted cheerfully.

[am_adam]"Good morning to you too," Adam replied with a smile.

[af_bella]"Did someone say morning? I could use some coffee!" Bella laughed.

[PAUSE:1500]

[af_sarah]Sarah turned to her friends. "Shall we get breakfast together?"

[am_adam]"That sounds wonderful," Adam agreed.
```

Generate with multiple voices:

```bash
./kokoro_audiobook -m model.onnx -v voices.bin -i dialogue.txt -o dialogue.wav
```

### Example 3: Manual Pause Control

Create `manual_timing.txt`:

```text
This text has no automatic pauses at punctuation. Notice how it flows continuously without breaks. [PAUSE:1000] But I can still add explicit pauses wherever I want them. [PAUSE:500] This gives me complete control over the timing and pacing of the narration.
```

Generate with manual pause control:

```bash
./kokoro_audiobook -m model.onnx -v voices.bin -i manual_timing.txt -o manual.wav --no-auto-pause
```

## Setup Instructions

### 1. Download model files

```bash
wget https://github.com/thewh1teagle/kokoro-onnx/releases/download/model-files-v1.0/kokoro-v1.0.onnx
wget https://github.com/thewh1teagle/kokoro-onnx/releases/download/model-files-v1.0/voices-v1.0.bin

# Convert to C format
python3 ../scripts/convert_voices.py voices-v1.0.bin voices-v1.0-c.bin
```

### 2. Build the audiobook generator

```bash
cd c
./build.sh
# or
mkdir build && cd build && cmake .. && make
```

### 3. Run with your text file

```bash
./build/kokoro_audiobook -m kokoro-v1.0.onnx -v voices-v1.0-c.bin -i your_story.txt -o audiobook.wav
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

1. **Use proper punctuation**: The generator relies on punctuation for natural pauses (unless disabled with `--no-auto-pause`)
2. **Keep paragraphs reasonable**: Very long paragraphs without breaks can sound monotonous
3. **Test with short samples first**: Try a few paragraphs before generating a full book
4. **Choose appropriate voice**: Different voices work better for different content
5. **Adjust speed if needed**: Slower (0.8-0.9) for complex content, faster (1.1-1.2) for lighter material
6. **Use inline voice switching**: Perfect for dialogue and multi-character content
7. **Consider manual pause control**: Use `--no-auto-pause` when you want precise control over timing

### For Voice Switching in Dialogue

1. **Place voice directives at the start of dialogue**: `[af_sarah]"Hello there!"`
2. **Be consistent**: Use the same voice for the same character throughout
3. **Test voice combinations**: Some voices pair better for conversations
4. **Use narrator voice**: Consider one voice for narration, others for dialogue
5. **Available voices**: af_sarah, af_bella, af_sky, af_jessica, am_adam, am_michael, and more

### For Manual Pause Control

When using `--no-auto-pause`:
1. **Add explicit pauses**: Use `[PAUSE:ms]` where you want breaks
2. **Control pacing**: Great for poetry, scripts, or precise timing needs
3. **Faster generation**: Slightly faster processing without pause detection
4. **Full control**: You decide exactly where every pause goes

### For Long Texts

1. **No length limits**: The library now automatically handles sentences of any length by batching phonemes intelligently
2. **Split into chapters** (optional): Generate each chapter separately for easier management
3. **Monitor memory**: The generator accumulates all audio in memory
4. **Check progress**: The generator shows progress during generation
5. **Be patient**: Long texts can take several minutes to process

### Voice Selection Examples

Try different voices for different content types:

```bash
# Single voice audiobook - narrative fiction
./kokoro_audiobook -m model.onnx -v voices.bin -i novel.txt -o output.wav -V af_bella

# Technical content - clear, precise voice
./kokoro_audiobook -m model.onnx -v voices.bin -i manual.txt -o output.wav -V af_sarah -s 0.9

# Children's story - friendly, engaging voice
./kokoro_audiobook -m model.onnx -v voices.bin -i kids_story.txt -o output.wav -V af_sky
```

**For multi-voice content**, use inline voice switching in your text file:

```text
[af_sarah]The narrator sets the scene...

[am_adam]"I have something to say," the man announced.

[af_bella]"What is it?" the woman asked curiously.
```

## Troubleshooting

### "Text too long" error

This error is now rare as the library automatically handles long texts by batching. If you still encounter it, the text file itself may have extremely long lines. Consider adding line breaks for readability.

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

The distribution includes sample text files demonstrating different features:

**`examples/sample_story.txt`** - Basic usage:
- Basic text with punctuation
- Paragraph breaks
- Custom pause directives
- Natural formatting

**`examples/sample_voice_switching.txt`** - Advanced features (NEW):
- Inline voice switching with `[voice_name]`
- Combining voice changes with custom pauses
- Multi-character dialogue examples

Try them:

```bash
# Basic sample
./build/kokoro_audiobook -m kokoro-v1.0.onnx -v voices-v1.0-c.bin \
    -i examples/sample_story.txt -o sample_audio.wav

# Voice switching sample
./build/kokoro_audiobook -m kokoro-v1.0.onnx -v voices-v1.0-c.bin \
    -i examples/sample_voice_switching.txt -o multi_voice.wav

# Same with manual pause control
./build/kokoro_audiobook -m kokoro-v1.0.onnx -v voices-v1.0-c.bin \
    -i examples/sample_voice_switching.txt -o manual.wav --no-auto-pause
```

## Further Reading

- [API Documentation](API.md) - Complete API reference
- [README](README.md) - General library overview
- [Integration Guide](INTEGRATION.md) - Using in your projects
