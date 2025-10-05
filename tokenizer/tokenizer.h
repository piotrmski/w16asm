#ifndef tokenizer
#define tokenizer

#include <stdbool.h>

#define MAX_TOKENS 0x8000
#define MAX_NAME_LEN_INCL_0 0x20
#define MAX_STR_VAL_LEN_INCL_0 0x2000

enum TokenType {
    TokenTypeNone,
    TokenTypeComment,
    TokenTypeLabelDefinition,
    TokenTypeLabelUseOrInstruction,
    TokenTypeDirective,
    TokenTypeDecimalNumber,
    TokenTypeMinus,
    TokenTypeZero,
    TokenTypeHexNumber,
    TokenTypeOctalNumber,
    TokenTypeBinaryNumber,
    TokenTypeZTString,
    TokenTypeNZTString
};

struct Token {
    enum TokenType tokenType;
    int numberValue;
    char* stringValue;
};

struct TokenizerState {
    bool error;
    struct Token tokens[MAX_TOKENS];
    int currentTokenIndex;
    char currentTokenStringValue[MAX_NAME_LEN_INCL_0];
    int currentTokenStringValueIndex;
    int lineNumber;
    bool isEscapeSequence;
    bool isHexEscapeSequence;
    char hexEscapeSequenceChar1;
    char hexEscapeSequenceChar2;
};

struct TokenizerState getInitialTokenizerState();

void parseChar(struct TokenizerState* state, int ch);

void validateEof(struct TokenizerState* state);



#endif