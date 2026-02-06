/**
 * Audio Utilities for Kokoro TTS
 * 
 * Implementation of audio processing functions including silence trimming
 * based on RMS energy analysis (inspired by librosa's trim function).
 */

#include "audio_utils.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/**
 * Calculate RMS (Root Mean Square) energy for audio frames
 */
int audio_calculate_rms(
    const float* audio,
    size_t num_samples,
    int frame_length,
    int hop_length,
    float* rms_values,
    size_t* num_frames
) {
    if (!audio || !rms_values || !num_frames) {
        return -1;
    }
    
    if (num_samples < (size_t)frame_length) {
        return -1;
    }
    
    /* Calculate number of frames */
    *num_frames = 0;
    for (size_t i = 0; i + frame_length <= num_samples; i += hop_length) {
        (*num_frames)++;
    }
    
    /* Calculate RMS for each frame */
    size_t frame_idx = 0;
    for (size_t i = 0; i + frame_length <= num_samples; i += hop_length) {
        double sum = 0.0;
        for (int j = 0; j < frame_length; j++) {
            float sample = audio[i + j];
            sum += sample * sample;
        }
        float mean = (float)(sum / frame_length);
        rms_values[frame_idx] = sqrtf(mean);
        frame_idx++;
    }
    
    return 0;
}

/**
 * Convert amplitude to decibels
 */
static float amplitude_to_db(float amplitude, float ref) {
    const float amin = 1e-5f;
    amplitude = fmaxf(amplitude, amin);
    ref = fmaxf(ref, amin);
    return 20.0f * log10f(amplitude / ref);
}

/**
 * Find maximum value in an array
 */
static float find_max(const float* values, size_t count) {
    float max_val = values[0];
    for (size_t i = 1; i < count; i++) {
        if (values[i] > max_val) {
            max_val = values[i];
        }
    }
    return max_val;
}

/**
 * Trim leading and trailing silence from an audio signal
 */
int audio_trim_silence(
    const float* audio,
    size_t num_samples,
    float* trimmed_audio,
    size_t* trimmed_size,
    float top_db,
    int frame_length,
    int hop_length
) {
    if (!audio || !trimmed_audio || !trimmed_size) {
        return -1;
    }
    
    if (num_samples == 0) {
        *trimmed_size = 0;
        return 0;
    }
    
    /* Handle short audio (less than one frame) */
    if (num_samples < (size_t)frame_length) {
        if (trimmed_audio != audio) {
            memcpy(trimmed_audio, audio, num_samples * sizeof(float));
        }
        *trimmed_size = num_samples;
        return 0;
    }
    
    /* Calculate number of frames needed */
    size_t max_frames = (num_samples - frame_length) / hop_length + 1;
    
    /* Allocate buffer for RMS values */
    float* rms_values = (float*)malloc(max_frames * sizeof(float));
    if (!rms_values) {
        return -1;
    }
    
    /* Calculate RMS for each frame */
    size_t num_frames;
    int result = audio_calculate_rms(audio, num_samples, frame_length, hop_length, 
                                     rms_values, &num_frames);
    if (result != 0) {
        free(rms_values);
        return -1;
    }
    
    /* Find reference (maximum RMS) */
    float ref = find_max(rms_values, num_frames);
    
    /* Convert RMS to dB and find non-silent frames */
    int first_nonsilent = -1;
    int last_nonsilent = -1;
    
    for (size_t i = 0; i < num_frames; i++) {
        float db = amplitude_to_db(rms_values[i], ref);
        if (db > -top_db) {
            if (first_nonsilent == -1) {
                first_nonsilent = (int)i;
            }
            last_nonsilent = (int)i;
        }
    }
    
    free(rms_values);
    
    /* If all frames are silent, return empty audio */
    if (first_nonsilent == -1) {
        *trimmed_size = 0;
        return 0;
    }
    
    /* Calculate sample positions */
    size_t start_sample = (size_t)first_nonsilent * hop_length;
    size_t end_sample = (size_t)(last_nonsilent + 1) * hop_length;
    
    /* Clamp end_sample to array bounds */
    if (end_sample > num_samples) {
        end_sample = num_samples;
    }
    
    /* Copy trimmed audio */
    size_t trimmed_length = end_sample - start_sample;
    if (trimmed_audio != audio) {
        memcpy(trimmed_audio, audio + start_sample, trimmed_length * sizeof(float));
    } else if (start_sample != 0) {
        /* In-place trim: move data to start of buffer */
        memmove(trimmed_audio, audio + start_sample, trimmed_length * sizeof(float));
    }
    
    *trimmed_size = trimmed_length;
    return 0;
}
