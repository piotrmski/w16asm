#include <stdio.h>
#include <string.h>
#include "program-input/program-input.h"
#include "tokenizer/tokenizer.h"

int main(int argc, const char * argv[]) {
    struct ProgramInput input = getProgramInput(argc, argv);

    if (input.error) { return 1; }

    if (input.asmFilePath == NULL || input.binaryFilePath == NULL) { return 0; }

    FILE* asmFile = fopen(input.asmFilePath, "r");

    if (asmFile == NULL) {
        printf("Error: could not read file \"%s\"\n.", input.binaryFilePath);
        return 1;
    }

    struct TokenizerState tokenizerState = getInitialTokenizerState();

    int ch;
    while ((ch = getc(asmFile)) != EOF && !tokenizerState.error) {
        parseChar(&tokenizerState, ch);
    }

    fclose(asmFile);

    validateEof(&tokenizerState);

    if (tokenizerState.error) { return 1; }

    for (int i = 0; tokenizerState.tokens[i].tokenType != TokenTypeNone; ++i) {
        printf("%s\n", tokenizerState.tokens[i].stringValue);
    }


    return 0;
}