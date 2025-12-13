#include "tokenizer.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>
#include "../../common/exit-code.h"

struct TokenizerState {
    char stringValue[MAX_NAME_LEN_INCL_0];
    int stringValueIndex;
    bool isEscapeSequence;
    bool isHexEscapeSequence;
    char hexEscapeSequenceChar1;
};

static char uc(char ch) {
    if (ch >= 'a' && ch <= 'z') return ch - 0x20;
    else return ch;
}

static bool isHexDigit(char ch) {
    return ch >= '0' && ch <= '9' || ch >= 'a' && ch <= 'f' || ch >= 'A' && ch <= 'F';
}

static int hexDigitToNumber(char ch) {
    return ch >= '0' && ch <= '9'
        ? ch - '0'
        : 10 + uc(ch) - 'A';
}

static void assertNumberInRange(struct Token* token) {
    if (token->numberValue < SHRT_MIN ||
        token->numberValue > USHRT_MAX) {
        printf("Error on line %d: number %d is out of range.\n", token->lineNumber, token->numberValue);
        exit(ExitCodeNumberLiteralOutOutRange);
    }
}

static bool submitToken(struct Token* token, struct TokenizerState* state) {
    state->stringValue[state->stringValueIndex] = 0;
    int fullLength = state->stringValueIndex + 1;
    token->stringValue = malloc(fullLength);
    memcpy(token->stringValue, state->stringValue, fullLength);
    return true;
}

static bool isEndOfToken(int ch) {
    return ch == ';' || isspace(ch);
}

// Returns true if an entire token was parsed, or false if another character needs to be parsed
static bool parseChar(struct Token* token, struct TokenizerState* state, int ch) {
    if (ch == '\n') {
        ++token->lineNumber;
    }

    switch (token->type) {
        case TokenTypeNone:
            state->stringValue[0] = ch;
            state->stringValueIndex = 1;
            
            if (ch == '_' || ch >= 'a' && ch <= 'z' || ch >= 'A' && ch <= 'Z') {
                token->type = TokenTypeLabelUseOrInstruction;
            } else if (ch == '-') {
                token->type = _TokenTypeMinus;
            } else if (ch == '0') {
                token->type = _TokenTypeZero;
                token->numberValue = 0;
            } else if (ch >= '1' && ch <= '9') {
                token->type = TokenTypeDecimalNumber;
                token->numberValue = ch - '0';
            } else if (ch == '.') {
                token->type = TokenTypeDirective;
                state->stringValueIndex = 0;
            } else if (ch == ';') {
                token->type = _TokenTypeComment;
            } else if (ch == '\'') {
                token->type = TokenTypeNZTString;
                state->stringValueIndex = 0;
            } else if (ch == '"') {
                token->type = TokenTypeZTString;
                state->stringValueIndex = 0;
            } else if (!isspace(ch)) {
                printf("Error on line %d: unexpected character '%c'.\n", token->lineNumber, ch);
                exit(ExitCodeUnexpectedCharacter);
            }
            break;
        case _TokenTypeComment:
            if (ch == '\n') {
                token->type = TokenTypeNone;
            }
            break;
        case TokenTypeLabelUseOrInstruction:
            if (ch == '_' || ch >= 'a' && ch <= 'z' || ch >= 'A' && ch <= 'Z' || ch >= '0' && ch <= '9') {
                if (state->stringValueIndex + 1 >= MAX_NAME_LEN_INCL_0) {
                    printf("Error on line %d: label name too long.\n", token->lineNumber);
                    exit(ExitCodeLabelNameTooLong);
                } else {
                    state->stringValue[state->stringValueIndex++] = ch;
                }
            } else if (ch == ':' || isEndOfToken(ch)) {
                if (ch == ':') {
                    token->type = TokenTypeLabelDefinition;
                }
                return submitToken(token, state);
            } else {
                printf("Error on line %d: unexpected character '%c'.\n", token->lineNumber, ch);
                exit(ExitCodeUnexpectedCharacter);
            }
            break;
        case TokenTypeDirective:
            if (ch >= 'a' && ch <= 'z' || ch >= 'A' && ch <= 'Z') {
                state->stringValue[state->stringValueIndex++] = ch;
            } else if (isEndOfToken(ch)) {
                return submitToken(token, state);
            } else {
                printf("Error on line %d: unexpected character '%c'.\n", token->lineNumber, ch);
                exit(ExitCodeUnexpectedCharacter);
            }
            break;
        case TokenTypeDecimalNumber:
            if (ch >= '0' && ch <= '9') {
                state->stringValue[state->stringValueIndex++] = ch;
                int newDigit = ch - '0';
                int sign = token->numberValue >= 0 ? 1 : -1;
                token->numberValue *= 10;
                token->numberValue += sign * newDigit;
                assertNumberInRange(token);
            } else if (isEndOfToken(ch)) {
                return submitToken(token, state);
            } else {
                printf("Error on line %d: unexpected character '%c'.\n", token->lineNumber, ch);
                exit(ExitCodeUnexpectedCharacter);
            }
            break;
        case _TokenTypeMinus:
            if (ch >= '0' && ch <= '9') {
                state->stringValue[state->stringValueIndex++] = ch;
                int newDigit = ch - '0';
                token->numberValue = -newDigit;
                token->type = TokenTypeDecimalNumber;
            } else {
                printf("Error on line %d: character '%c' (0x%02X) following '-' is invalid.\n", token->lineNumber, ch, ch);
                exit(ExitCodeInvalidMinus);
            }
            break;
        case _TokenTypeZero:
            if (ch >= '0' && ch <= '7') {
                state->stringValue[state->stringValueIndex++] = ch;
                token->type = TokenTypeOctalNumber;
                token->numberValue = ch - '0';
            } else if (ch == 'x' || ch == 'X') {
                state->stringValue[state->stringValueIndex++] = ch;
                token->type = TokenTypeHexNumber;
            } else if (ch == 'b' || ch == 'B') {
                state->stringValue[state->stringValueIndex++] = ch;
                token->type = TokenTypeBinaryNumber;
            } else if (isEndOfToken(ch)) {
                token->type = TokenTypeDecimalNumber;
                return submitToken(token, state);
            } else {
                printf("Error on line %d: unexpected character '%c'.\n", token->lineNumber, ch);
                exit(ExitCodeUnexpectedCharacter);
            }
            break;
        case TokenTypeHexNumber:
            if (isHexDigit(ch)) {
                state->stringValue[state->stringValueIndex++] = ch;
                token->numberValue *= 0x10;
                token->numberValue += hexDigitToNumber(ch);
                assertNumberInRange(token);
            } else if (isEndOfToken(ch)) {
                if (state->stringValueIndex == 2) {
                    printf("Error on line %d: number without digits.\n", token->lineNumber);
                    exit(ExitCodeNumberWithoutDigits);
                } else {
                    return submitToken(token, state);
                }
            } else {
                printf("Error on line %d: unexpected character '%c'.\n", token->lineNumber, ch);
                exit(ExitCodeUnexpectedCharacter);
            }
            break;
        case TokenTypeOctalNumber:
            if (ch >= '0' && ch <= '7') {
                state->stringValue[state->stringValueIndex++] = ch;
                token->numberValue *= 010;
                token->numberValue += ch - '0';
                assertNumberInRange(token);
            } else if (isEndOfToken(ch)) {
                return submitToken(token, state);
            } else {
                printf("Error on line %d: unexpected character '%c'.\n", token->lineNumber, ch);
                exit(ExitCodeUnexpectedCharacter);
            }
            break;
        case TokenTypeBinaryNumber:
            if (ch == '0' || ch == '1') {
                state->stringValue[state->stringValueIndex++] = ch;
                token->numberValue *= 0b10;
                token->numberValue += ch - '0';
                assertNumberInRange(token);
            } else if (isEndOfToken(ch)) {
                if (state->stringValueIndex == 2) {
                    printf("Error on line %d: number without digits.\n", token->lineNumber);
                    exit(ExitCodeNumberWithoutDigits);
                } else {
                    return submitToken(token, state);
                }
            } else {
                printf("Error on line %d: unexpected character '%c'.\n", token->lineNumber, ch);
                exit(ExitCodeUnexpectedCharacter);
            }
            break;
        case TokenTypeZTString:
        case TokenTypeNZTString:
            if (state->stringValueIndex == MAX_STR_VAL_LEN_INCL_0 - 1) {
                printf("Error on line %d: string too long.\n", token->lineNumber);
                exit(ExitCodeStringTooLong);
            } else if (state->isHexEscapeSequence) {
                if (state->hexEscapeSequenceChar1 == 0) {
                    state->hexEscapeSequenceChar1 = ch;
                } else if (!isHexDigit(state->hexEscapeSequenceChar1) || !isHexDigit(ch)) {
                    printf("Error on line %d: invalid escape sequence '\\x%c%c'.\n", token->lineNumber, state->hexEscapeSequenceChar1, ch);
                    exit(ExitCodeInvalidEscapeSequence);
                } else {
                    char escapeSequenceChar = 0x10 * hexDigitToNumber(state->hexEscapeSequenceChar1) + hexDigitToNumber(ch);
                    state->stringValue[state->stringValueIndex++] = escapeSequenceChar;
                    state->isHexEscapeSequence = false;
                    state->hexEscapeSequenceChar1 = 0;
                }
            } else if (state->isEscapeSequence) {
                state->isEscapeSequence = false;

                if (ch == 'x' || ch == 'X') {
                    state->isHexEscapeSequence = true;
                } else if (ch == '\'' || ch == '"' || ch == '\\') {
                    state->stringValue[state->stringValueIndex++] = ch;
                } else if (ch == 'n' || ch == 'N') {
                    state->stringValue[state->stringValueIndex++] = '\n';
                } else if (ch == 't' || ch == 'T') {
                    state->stringValue[state->stringValueIndex++] = '\t';
                } else if (ch == 'r' || ch == 'R') {
                    state->stringValue[state->stringValueIndex++] = '\r';
                } else {
                    printf("Error on line %d: invalid escape sequence '\\%c'.\n", token->lineNumber, ch);
                    exit(ExitCodeInvalidEscapeSequence);
                }
            } else if (ch == '\\') {
                state->isEscapeSequence = true;
            } else if (ch == (token->type == TokenTypeZTString ? '"' : '\'')) {
                return submitToken(token, state);
            } else {
                state->stringValue[state->stringValueIndex++] = ch;
            }
            break;
        default: 
            break;
    }

    return false;
}

static void validateEof(struct Token* token, struct TokenizerState* state) {
    if (token->type == _TokenTypeComment) {
        token->type = TokenTypeNone;
    }

    if (token->type == _TokenTypeZero) {
        token->type = TokenTypeDecimalNumber;
    }

    if (token->type == _TokenTypeMinus) {
        printf("Error on line %d: '-' can't be at the end of the file.\n", token->lineNumber);
        exit(ExitCodeInvalidMinus);
    }

    if ((token->type == TokenTypeBinaryNumber
        || token->type == TokenTypeHexNumber)
        && state->stringValueIndex == 2) {
        printf("Error on line %d: number without digits.\n", token->lineNumber);
        exit(ExitCodeNumberWithoutDigits);
    }

    if (token->type == TokenTypeZTString
        || token->type == TokenTypeNZTString) {
        printf("Error on line %d: unterminated string literal.\n", token->lineNumber);
        exit(ExitCodeUnterminatedString);
    }

    submitToken(token, state);
}

struct Token getInitialTokenState() {
    return (struct Token) { 1, TokenTypeNone, 0, 0 };
}

void getToken(struct Token* token, FILE* filePtr) {
    token->type = TokenTypeNone;
    token->numberValue = 0;
    token->stringValue = NULL;

    struct TokenizerState state = { { 0 }, 0, false, false, 0 };

    int ch;

    while ((ch = getc(filePtr)) != EOF) {
        if (parseChar(token, &state, ch)) {
            if (ch == ';') {
                ungetc(ch, filePtr);
            }
            return;
        }
    }

    validateEof(token, &state);
}