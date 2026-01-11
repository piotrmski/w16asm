#include "preprocessor.h"
#include "../tokenizer/tokenizer.h"
#include <stdlib.h>

#define ARRAY_SIZE_INCREMENT 0x1000

static struct Token* result;
static int tokensCount = 0;

static void pushToken(struct Token token) {
    if ((tokensCount % ARRAY_SIZE_INCREMENT) == ARRAY_SIZE_INCREMENT - 1) {
        result = realloc(result, sizeof (struct Token) * (tokensCount + 1 + ARRAY_SIZE_INCREMENT));
    }
    result[tokensCount++] = token;
}

struct Token* preprocess(char* assemblySource) {
    result = calloc(ARRAY_SIZE_INCREMENT, sizeof (struct Token));

    do {
        pushToken(getToken(&assemblySource));
    } while (result[tokensCount - 1].value != NULL);

    return result;
}