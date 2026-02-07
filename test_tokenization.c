#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Simplified vocabulary map - just test critical phonemes */
static const struct {
    const char* phoneme;
    int token_id;
} TEST_VOCAB[] = {
    {" ", 16}, {".", 4}, {"!", 5}, {"h", 50}, {"ə", 83}, {"l", 54},
    {"ˈ", 156}, {"o", 57}, {"ʊ", 135}, {"ð", 81}, {"ɪ", 102}, {"s", 61},
    {"ɔ", 76}, {"ː", 158}, {"d", 46}, {"ˌ", 157}, {"ʒ", 147}, {"ɛ", 86},
    {"n", 56}, {"ɚ", 85}, {"ɹ", 123}, {"e", 47}, {"ɾ", 125}, {"ᵻ", 177},
    {"b", 44}, {"a", 43}, {"k", 53},
    {NULL, 0}
};

static int match_phoneme(const char* phonemes, size_t* bytes_consumed) {
    for (int i = 0; TEST_VOCAB[i].phoneme != NULL; i++) {
        const char* vocab_phoneme = TEST_VOCAB[i].phoneme;
        size_t len = strlen(vocab_phoneme);
        
        if (strncmp(phonemes, vocab_phoneme, len) == 0) {
            *bytes_consumed = len;
            return TEST_VOCAB[i].token_id;
        }
    }
    
    *bytes_consumed = 1;
    return -1;
}

static int tokenize_phonemes(const char* phonemes, int64_t* tokens, size_t max_tokens) {
    size_t token_count = 0;
    const char* p = phonemes;
    
    while (*p && token_count < max_tokens) {
        size_t bytes_consumed = 0;
        int token_id = match_phoneme(p, &bytes_consumed);
        
        if (token_id >= 0) {
            tokens[token_count++] = token_id;
        }
        
        p += bytes_consumed;
    }
    
    return token_count;
}

int main() {
    const char* phonemes = "həlˈoʊ. ðɪs ˈɔːdɪˌoʊ dʒˈɛnɚɹˌeɪɾᵻd baɪ kəkˈɔːɹoʊ!";
    int64_t tokens[512];
    
    int num_tokens = tokenize_phonemes(phonemes, tokens, 512);
    
    printf("Phonemes: %s\n", phonemes);
    printf("Token count: %d\n", num_tokens);
    printf("Expected: 49\n");
    printf("Tokens: ");
    for (int i = 0; i < num_tokens; i++) {
        printf("%ld ", tokens[i]);
    }
    printf("\n");
    
    return 0;
}
