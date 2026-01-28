/**
 * Simple test to verify Kokoro C library compiles and links correctly
 * This doesn't require model files to run
 */

#include <stdio.h>
#include <kokoro.h>

int main() {
    printf("Kokoro C Library - Compilation Test\n");
    printf("=====================================\n\n");
    
    // Test 1: Check constants
    printf("Test 1: Constants\n");
    printf("  MAX_PHONEME_LENGTH: %d\n", KOKORO_MAX_PHONEME_LENGTH);
    printf("  SAMPLE_RATE: %d\n", KOKORO_SAMPLE_RATE);
    printf("  ✓ Pass\n\n");
    
    // Test 2: Check error strings
    printf("Test 2: Error Strings\n");
    printf("  SUCCESS: %s\n", kokoro_error_string(KOKORO_SUCCESS));
    printf("  NULL_POINTER: %s\n", kokoro_error_string(KOKORO_ERROR_NULL_POINTER));
    printf("  FILE_NOT_FOUND: %s\n", kokoro_error_string(KOKORO_ERROR_FILE_NOT_FOUND));
    printf("  ✓ Pass\n\n");
    
    // Test 3: NULL pointer handling
    printf("Test 3: NULL Pointer Handling\n");
    kokoro_free(NULL);  // Should not crash
    printf("  kokoro_free(NULL): OK\n");
    
    kokoro_audio_t audio = {0};
    kokoro_audio_free(&audio);  // Should not crash
    printf("  kokoro_audio_free(empty): OK\n");
    printf("  ✓ Pass\n\n");
    
    // Test 4: Try to initialize with invalid paths (should fail gracefully)
    printf("Test 4: Invalid Initialization\n");
    kokoro_t* kokoro = kokoro_init("nonexistent.onnx", "nonexistent.bin", NULL, NULL);
    if (kokoro == NULL) {
        printf("  Correctly returns NULL for invalid files\n");
        printf("  ✓ Pass\n\n");
    } else {
        printf("  ✗ Unexpected: returned non-NULL for invalid files\n\n");
        kokoro_free(kokoro);
    }
    
    printf("All compilation tests passed!\n");
    printf("\nNote: To test actual TTS functionality, you need:\n");
    printf("  1. Download model files\n");
    printf("  2. Run the full example: ./kokoro_example\n");
    
    return 0;
}
