# Audiobook Generator - New Features Testing Guide

## Overview

This document provides testing instructions for the new audiobook generator features:
1. `--no-auto-pause` flag for manual pause control
2. Inline voice switching using `[voice_name]` syntax

## Features Implemented

### 1. Auto-Pause Control

**Default Behavior (auto-pause enabled):**
- Period/Exclamation/Question (`.!?`): 500ms pause
- Comma (`,`): 250ms pause
- Semicolon/Colon (`;:`): 350ms pause
- Paragraph breaks (empty line): 800ms pause
- Custom directives (`[PAUSE:ms]`): Always work

**With `--no-auto-pause` flag:**
- No automatic pauses at punctuation
- No automatic paragraph break pauses
- Custom `[PAUSE:ms]` directives still work
- Gives complete manual control

### 2. Inline Voice Switching

**Syntax:** `[voice_name]text...`

**Supported voices:**
- Female: af_sarah, af_bella, af_sky, af_jessica
- Male: am_adam, am_michael
- And more available in the voices file

**Features:**
- Change voice anywhere in text
- Works with both auto-pause modes
- Progress display shows current voice
- Perfect for dialogue and multi-character content

## Test Cases

### Test 1: Default Auto-Pause Mode

**Test File:** `test_auto_pause.txt`
```text
This is a sentence. This is another sentence.

New paragraph here.

Short pause, followed by a longer pause; and then a colon: like this.

Final sentence!
```

**Command:**
```bash
./kokoro_audiobook -m model.onnx -v voices.bin -i test_auto_pause.txt -o test1.wav
```

**Expected Result:**
- 500ms pause after "sentence."
- 500ms pause after "sentence."
- 800ms pause before "New paragraph"
- 250ms pause after "pause,"
- 500ms pause after "pause;"
- 350ms pause after "colon:"
- 500ms pause after "this."
- 500ms pause after "sentence!"

### Test 2: Manual Pause Control (--no-auto-pause)

**Test File:** `test_no_auto_pause.txt`
```text
This flows continuously without pauses at punctuation marks. Notice how the comma, period, and other punctuation don't create breaks. [PAUSE:1000] But explicit pauses still work perfectly! [PAUSE:500] This gives precise control.
```

**Command:**
```bash
./kokoro_audiobook -m model.onnx -v voices.bin -i test_no_auto_pause.txt -o test2.wav --no-auto-pause
```

**Expected Result:**
- No pauses at commas or periods
- 1000ms pause after "marks."
- No pause after "perfectly!"
- 500ms pause after "work perfectly!"
- No pause after "control."

### Test 3: Inline Voice Switching

**Test File:** `test_voice_switching.txt`
```text
[af_sarah]Hello, I'm Sarah speaking with my voice.

[af_bella]And now Bella is speaking!

[am_adam]Adam here with a male voice.

[af_sarah]Sarah again for the conclusion.
```

**Command:**
```bash
./kokoro_audiobook -m model.onnx -v voices.bin -i test_voice_switching.txt -o test3.wav
```

**Expected Result:**
- First line spoken by af_sarah
- Second line spoken by af_bella
- Third line spoken by am_adam
- Fourth line spoken by af_sarah
- Progress output shows: "(voice: af_sarah)", "(voice: af_bella)", etc.

### Test 4: Combined Features (Voice Switching + Custom Pauses)

**Test File:** `test_combined.txt`
```text
[af_sarah]The narrator begins the story. [PAUSE:1000] A long time ago...

[af_bella]"Who goes there?" the guard demanded.

[PAUSE:500]

[am_adam]"Just a traveler," came the reply.

[af_sarah]And so the adventure began.
```

**Command:**
```bash
./kokoro_audiobook -m model.onnx -v voices.bin -i test_combined.txt -o test4.wav
```

**Expected Result:**
- af_sarah narrates opening
- 1000ms pause after "story."
- af_bella speaks guard's line with auto-pause after "demanded."
- 500ms explicit pause
- am_adam speaks traveler's line with auto-pause after "reply."
- af_sarah narrates conclusion

### Test 5: Voice Switching Without Auto-Pause

**Test File:** `test_voice_no_auto.txt`
```text
[af_sarah]Rapid narration without pauses. This flows quickly. [PAUSE:800] Except here.

[af_bella]Same for this section. No punctuation pauses. [PAUSE:600] Only explicit ones.
```

**Command:**
```bash
./kokoro_audiobook -m model.onnx -v voices.bin -i test_voice_no_auto.txt -o test5.wav --no-auto-pause
```

**Expected Result:**
- af_sarah speaks continuously
- Only 800ms pause where specified
- af_bella speaks continuously
- Only 600ms pause where specified
- No pauses at periods

## Validation Checklist

### Command-Line Interface
- [ ] `--no-auto-pause` flag is accepted
- [ ] `-n` short form works
- [ ] Help message shows new flag
- [ ] Help message mentions voice switching
- [ ] Configuration output shows "Auto-pause: enabled/disabled"

### Auto-Pause Functionality
- [ ] Default mode adds pauses at punctuation
- [ ] Periods add 500ms pause
- [ ] Commas add 250ms pause
- [ ] Semicolons/colons add 350ms pause
- [ ] Paragraph breaks add 800ms pause
- [ ] `--no-auto-pause` disables punctuation pauses
- [ ] `--no-auto-pause` disables paragraph pauses
- [ ] Custom `[PAUSE:ms]` works in both modes

### Voice Switching
- [ ] `[voice_name]` changes voice
- [ ] Invalid voice names are handled gracefully
- [ ] Voice changes are shown in progress output
- [ ] Multiple voice changes in same file work
- [ ] Voice switching works with auto-pause enabled
- [ ] Voice switching works with auto-pause disabled
- [ ] Voice switching works with custom pauses

### Edge Cases
- [ ] Empty `[PAUSE:]` doesn't crash
- [ ] Empty `[]` doesn't crash
- [ ] Unknown `[xyz]` is treated as text
- [ ] Multiple voice changes on same line work
- [ ] Very long voice names are handled
- [ ] Voice names with underscores work (e.g., af_sarah)
- [ ] Case sensitivity in voice names

## Output Verification

### Audio Quality
- Listen to generated files for:
  - Clear voice changes
  - Appropriate pause durations
  - No audio artifacts at transitions
  - Smooth voice-to-voice transitions

### Progress Display
- Check console output shows:
  - "Auto-pause: enabled" or "Auto-pause: disabled"
  - Current voice in progress: "(voice: af_sarah)"
  - "Voice changed to: af_bella" messages

### File Output
- Verify WAV files are created
- Check file sizes are reasonable
- Verify sample rate is 24kHz
- Verify mono channel output

## Performance Testing

### Benchmarks
1. Generate 1000-word story with auto-pause
2. Generate same story with --no-auto-pause
3. Generate multi-voice dialogue (10 voice changes)
4. Compare generation times

### Expected Performance
- `--no-auto-pause` should be slightly faster (no punctuation detection)
- Voice switching should have negligible overhead
- Memory usage should remain constant
- Large files should process without issues

## Regression Testing

### Backward Compatibility
- [ ] Existing audiobook files still work
- [ ] Positional arguments still work
- [ ] Old text files (no voice directives) work
- [ ] Default behavior matches previous version (with auto-pause)

### Integration
- [ ] Example programs compile successfully
- [ ] CMake build works
- [ ] All dependencies resolve correctly

## Known Limitations

1. **Voice Validation**: Voice names are not validated against available voices. Invalid names will fail during audio generation.
2. **Case Sensitivity**: Voice names are case-sensitive (af_sarah vs AF_SARAH)
3. **Nested Directives**: `[voice[pause]]` style nesting is not supported
4. **Memory**: Entire audiobook is kept in memory before writing

## Troubleshooting

### Common Issues

**Voice change not working:**
- Check voice name spelling (case-sensitive)
- Ensure voices.bin file contains the voice
- Verify voice directive format: `[voice_name]` not `[voice name]`

**Pauses not working:**
- With auto-pause: check punctuation is present
- Without auto-pause: ensure explicit `[PAUSE:ms]` directives
- Verify pause duration is reasonable (0-5000ms typical)

**Audio artifacts:**
- Check if silence trimming affects transitions
- Try adjusting pause durations
- Verify voice compatibility

## Conclusion

These features provide complete control over audiobook generation:
- Automatic pauses for natural speech (default)
- Manual control when precision is needed (--no-auto-pause)
- Dynamic voice switching for dialogue and characters
- Flexible combination of all features

The implementation maintains backward compatibility while adding powerful new capabilities for audiobook creators.
