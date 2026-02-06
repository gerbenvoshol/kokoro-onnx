/**
 * Example: Using Kokoro TTS C API
 * 
 * This example demonstrates how to use the Kokoro TTS library to generate
 * speech from text and save it to a WAV file.
 * 
 * Usage:
 *   ./kokoro_example -m <model.onnx> -v <voices.bin> -o <output.wav>
 *   ./kokoro_example <model.onnx> <voices.bin> <output.wav>  (backward compatible)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "kokoro.h"
#include "argparse.h"
#include "audio_utils.h"

/* WAV file header structure */
typedef struct {
    char riff[4];           // "RIFF"
    uint32_t file_size;     // File size - 8
    char wave[4];           // "WAVE"
    char fmt[4];            // "fmt "
    uint32_t fmt_size;      // Format chunk size (16 for PCM)
    uint16_t audio_format;  // Audio format (1 for PCM)
    uint16_t num_channels;  // Number of channels
    uint32_t sample_rate;   // Sample rate
    uint32_t byte_rate;     // Bytes per second
    uint16_t block_align;   // Bytes per sample
    uint16_t bits_per_sample; // Bits per sample
    char data[4];           // "data"
    uint32_t data_size;     // Data size
} wav_header_t;

/**
 * Write audio samples to a WAV file
 */
int write_wav(const char* filename, const float* samples, size_t num_samples, int sample_rate) {
    FILE* fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "Error: Could not open output file: %s\n", filename);
        return -1;
    }
    
    /* Convert float samples to int16 */
    int16_t* pcm_samples = (int16_t*)malloc(num_samples * sizeof(int16_t));
    if (!pcm_samples) {
        fclose(fp);
        return -1;
    }
    
    for (size_t i = 0; i < num_samples; i++) {
        float sample = samples[i];
        /* Clamp to [-1.0, 1.0] */
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;
        /* Convert to int16 */
        pcm_samples[i] = (int16_t)(sample * 32767.0f);
    }
    
    /* Create WAV header */
    wav_header_t header = {0};
    memcpy(header.riff, "RIFF", 4);
    memcpy(header.wave, "WAVE", 4);
    memcpy(header.fmt, "fmt ", 4);
    memcpy(header.data, "data", 4);
    
    header.fmt_size = 16;
    header.audio_format = 1;  // PCM
    header.num_channels = 1;  // Mono
    header.sample_rate = sample_rate;
    header.bits_per_sample = 16;
    header.block_align = header.num_channels * header.bits_per_sample / 8;
    header.byte_rate = header.sample_rate * header.block_align;
    header.data_size = num_samples * sizeof(int16_t);
    header.file_size = 36 + header.data_size;
    
    /* Write header */
    fwrite(&header, sizeof(wav_header_t), 1, fp);
    
    /* Write data */
    fwrite(pcm_samples, sizeof(int16_t), num_samples, fp);
    
    free(pcm_samples);
    fclose(fp);
    return 0;
}

int main(int argc, char** argv) {
    /* Parse command-line arguments */
    const char* model_path;
    const char* voices_path;
    const char* output_path;
    const char* voice_name;
    const char* text;
    float speed;
    const char* lang;
    
    int parse_result = parse_example_args(
        argc, argv,
        &model_path, &voices_path, &output_path,
        &voice_name, &text, &speed, &lang
    );
    
    if (parse_result != 0) {
        return (parse_result < 0) ? 1 : 0;  /* -1 = error, 1 = help shown */
    }
    
    printf("Kokoro TTS - Pure C Implementation\n");
    printf("===================================\n\n");
    
    /* Validate file paths */
    FILE* test_file = fopen(model_path, "rb");
    if (!test_file) {
        fprintf(stderr, "Error: Cannot open model file: %s\n", model_path);
        fprintf(stderr, "Please check that the file exists and path is correct.\n");
        return 1;
    }
    fclose(test_file);
    
    test_file = fopen(voices_path, "rb");
    if (!test_file) {
        fprintf(stderr, "Error: Cannot open voices file: %s\n", voices_path);
        fprintf(stderr, "Please check that the file exists and path is correct.\n");
        fprintf(stderr, "\nNote: You need the C-format voices file (voices-v1.0-c.bin),\n");
        fprintf(stderr, "not the Python format (voices-v1.0.bin).\n");
        return 1;
    }
    fclose(test_file);
    
    /* Initialize Kokoro */
    printf("Initializing Kokoro TTS...\n");
    printf("  Model: %s\n", model_path);
    printf("  Voices: %s\n", voices_path);
    printf("  Output: %s\n", output_path);
    printf("  Voice: %s\n", voice_name);
    
    kokoro_t* kokoro = kokoro_init(model_path, voices_path, NULL, NULL);
    if (!kokoro) {
        fprintf(stderr, "\nError: Failed to initialize Kokoro\n");
        fprintf(stderr, "\nPossible causes:\n");
        fprintf(stderr, "  1. Model file is invalid or corrupted\n");
        fprintf(stderr, "  2. Voices file is in wrong format (need C binary format)\n");
        fprintf(stderr, "  3. Insufficient memory\n");
        fprintf(stderr, "  4. espeak-ng not properly installed\n");
        fprintf(stderr, "\nTry running: python3 scripts/convert_voices.py voices-v1.0.bin voices-v1.0-c.bin\n");
        return 1;
    }
    
    printf("  ✓ Initialized successfully\n\n");
    
    /* List available voices */
    char** voices;
    size_t num_voices;
    kokoro_error_t err = kokoro_get_voices(kokoro, &voices, &num_voices);
    if (err == KOKORO_SUCCESS) {
        printf("Available voices (%zu):\n", num_voices);
        for (size_t i = 0; i < num_voices && i < 10; i++) {
            printf("  - %s\n", voices[i]);
            free(voices[i]);
        }
        if (num_voices > 10) {
            printf("  ... and %zu more\n", num_voices - 10);
            for (size_t i = 10; i < num_voices; i++) {
                free(voices[i]);
            }
        }
        free(voices);
        printf("\n");
    }
    
    /* Generate audio */
    printf("Generating audio...\n");
    printf("  Voice: %s\n", voice_name);
    printf("  Text: \"%s\"\n", text);
    printf("  Language: %s\n", lang);
    printf("  Speed: %.2f\n\n", speed);
    
    kokoro_audio_t audio = {0};
    err = kokoro_create(kokoro, text, voice_name, speed, lang, &audio);
    
    if (err != KOKORO_SUCCESS) {
        fprintf(stderr, "Error: %s\n", kokoro_error_string(err));
        kokoro_free(kokoro);
        return 1;
    }
    
    printf("  ✓ Generated %zu samples at %d Hz\n", audio.num_samples, audio.sample_rate);
    printf("  Duration: %.2f seconds\n", (double)audio.num_samples / audio.sample_rate);
    
    /* Trim silence for better audio quality */
    printf("  Trimming silence...\n");
    float* trimmed_samples = (float*)malloc(audio.num_samples * sizeof(float));
    if (!trimmed_samples) {
        fprintf(stderr, "Error: Out of memory\n");
        kokoro_audio_free(&audio);
        kokoro_free(kokoro);
        return 1;
    }
    
    size_t trimmed_size = 0;
    int trim_result = audio_trim_silence(
        audio.samples, audio.num_samples,
        trimmed_samples, &trimmed_size,
        60.0f,  /* top_db: 60 dB below peak is considered silence */
        2048,   /* frame_length: analysis window size */
        512     /* hop_length: step between frames */
    );
    
    if (trim_result != 0 || trimmed_size == 0) {
        fprintf(stderr, "Warning: Failed to trim silence, using original audio\n");
        free(trimmed_samples);
        trimmed_samples = audio.samples;
        trimmed_size = audio.num_samples;
    } else {
        printf("  ✓ Trimmed to %zu samples (%.2f seconds)\n", 
               trimmed_size, (double)trimmed_size / audio.sample_rate);
        /* Free original samples and use trimmed ones */
        free(audio.samples);
        audio.samples = trimmed_samples;
        audio.num_samples = trimmed_size;
    }
    printf("\n");
    
    /* Save to WAV file */
    printf("Saving audio to: %s\n", output_path);
    if (write_wav(output_path, audio.samples, audio.num_samples, audio.sample_rate) != 0) {
        fprintf(stderr, "Error: Failed to write WAV file\n");
        kokoro_audio_free(&audio);
        kokoro_free(kokoro);
        return 1;
    }
    
    printf("  ✓ Saved successfully\n\n");
    printf("Done!\n");
    
    /* Cleanup */
    kokoro_audio_free(&audio);
    kokoro_free(kokoro);
    
    return 0;
}
