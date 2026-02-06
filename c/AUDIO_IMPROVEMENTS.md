# Audio Quality Improvements

## Summary

This document describes the improvements made to the Kokoro C implementation to fix audio truncation and quality issues.

## Problems Addressed

1. **Audio Truncation**: Long texts were being truncated because the C implementation didn't handle phoneme sequences longer than MAX_PHONEME_LENGTH (510 phonemes).

2. **Audio Quality**: Generated audio contained leading and trailing silence artifacts that affected the perceived quality.

## Solutions Implemented

### 1. Audio Trimming (`audio_utils.c`)

Implemented RMS-based silence detection and trimming, based on librosa's trim function:

- **RMS Calculation**: Analyzes audio in frames to compute Root Mean Square energy
- **Silence Detection**: Identifies frames below a threshold (60 dB below peak)
- **Trimming**: Removes leading and trailing silent portions

Key functions:
- `audio_calculate_rms()`: Computes RMS energy for audio frames
- `audio_trim_silence()`: Trims silence from audio based on RMS analysis

### 2. Phoneme Batching (`kokoro.c`)

Implemented intelligent splitting of long phoneme sequences:

- **Smart Splitting**: Splits at punctuation marks (.,!?;) when possible
- **Batch Processing**: Generates audio for each batch independently
- **Trimming**: Each batch is trimmed before concatenation
- **Concatenation**: All batches are combined into final output

Key functions:
- `split_phonemes_into_batches()`: Splits phonemes at punctuation marks
- `kokoro_create()`: Enhanced to handle multiple batches

## Technical Details

### Audio Trimming Parameters

- **top_db**: 60 dB (threshold below peak considered silence)
- **frame_length**: 2048 samples (analysis window)
- **hop_length**: 512 samples (step between frames)

### Batching Strategy

1. If phonemes ≤ 510 characters: Process as single batch
2. If phonemes > 510 characters:
   - Split at punctuation marks
   - Ensure each batch ≤ 510 characters
   - Process and trim each batch
   - Concatenate results

## Code Quality

- Uses named constants instead of magic numbers
- Proper error handling and memory management
- Efficient string operations (memcpy instead of strcat/strncat)
- Comprehensive comments

## Comparison with Python Implementation

The C implementation now matches the Python version's behavior:

- ✅ Phoneme batching with punctuation-aware splitting
- ✅ Per-batch silence trimming
- ✅ Audio concatenation
- ✅ Same trimming parameters (60 dB, 2048/512 frame settings)

## Impact on Examples

### example.c (Basic TTS)
- Now handles texts of any length automatically
- No more truncation for long inputs
- Better audio quality with silence trimming

### audiobook.c (Long-form Content)
- Automatically benefits from batching improvements
- Each sentence call is batched if needed
- No changes required - works transparently
- Long sentences are now fully supported

## Testing Recommendations

1. **Short text**: "This is a test" - should work as before
2. **Long text**: Multiple sentences exceeding 510 characters - should no longer truncate
3. **Quality check**: Compare output with Python implementation - should sound similar

## Performance Considerations

- Single batch case has minimal overhead (one extra check)
- Multi-batch case adds:
  - Phoneme splitting: O(n) where n is phoneme length
  - Per-batch trimming: O(m) where m is audio samples per batch
  - Total overhead is linear and acceptable for typical use cases
