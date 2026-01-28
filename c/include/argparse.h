/**
 * Argument Parsing Utilities for Kokoro TTS C Programs
 * 
 * This module provides utilities for parsing command-line arguments
 * with support for both positional and named (flag-based) arguments.
 */

#ifndef KOKORO_ARGPARSE_H
#define KOKORO_ARGPARSE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Print help message for example program
 */
void print_example_help(const char* program_name);

/**
 * Print help message for audiobook program
 */
void print_audiobook_help(const char* program_name);

/**
 * Parse arguments for example program
 * Returns 0 on success, -1 on error, 1 if help was shown
 */
int parse_example_args(
    int argc,
    char** argv,
    const char** model_path,
    const char** voices_path,
    const char** output_path,
    const char** voice_name,
    const char** text,
    float* speed,
    const char** lang
);

/**
 * Parse arguments for audiobook program
 * Returns 0 on success, -1 on error, 1 if help was shown
 */
int parse_audiobook_args(
    int argc,
    char** argv,
    const char** model_path,
    const char** voices_path,
    const char** input_path,
    const char** output_path,
    const char** voice_name,
    const char** lang,
    float* speed
);

#ifdef __cplusplus
}
#endif

#endif /* KOKORO_ARGPARSE_H */
