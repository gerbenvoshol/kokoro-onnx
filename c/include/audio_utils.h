/**
 * Audio Utilities for Kokoro TTS
 * 
 * Provides audio processing functions including silence trimming
 * based on RMS energy analysis.
 */

#ifndef AUDIO_UTILS_H
#define AUDIO_UTILS_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Trim leading and trailing silence from an audio signal
 * 
 * This function detects and removes silent portions at the beginning
 * and end of an audio signal based on RMS energy analysis.
 * 
 * @param audio Input audio samples (float32, mono)
 * @param num_samples Number of input samples
 * @param trimmed_audio Output buffer for trimmed audio (allocated by caller, can be same as input)
 * @param trimmed_size Output: number of samples in trimmed audio
 * @param top_db Threshold in decibels below peak to consider as silence (default: 60)
 * @param frame_length Number of samples per analysis frame (default: 2048)
 * @param hop_length Number of samples between frames (default: 512)
 * @return 0 on success, -1 on error
 */
int audio_trim_silence(
    const float* audio,
    size_t num_samples,
    float* trimmed_audio,
    size_t* trimmed_size,
    float top_db,
    int frame_length,
    int hop_length
);

/**
 * Calculate RMS (Root Mean Square) energy for audio frames
 * 
 * @param audio Input audio samples
 * @param num_samples Number of samples
 * @param frame_length Number of samples per frame
 * @param hop_length Number of samples between frames
 * @param rms_values Output buffer for RMS values (allocated by caller)
 * @param num_frames Output: number of frames analyzed
 * @return 0 on success, -1 on error
 */
int audio_calculate_rms(
    const float* audio,
    size_t num_samples,
    int frame_length,
    int hop_length,
    float* rms_values,
    size_t* num_frames
);

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_UTILS_H */
