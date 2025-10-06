#include "tokenizer.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>

static char toUppercaseChar(char ch) {
    if (ch >= 'a' && ch <= 'z') return ch - 0x20;
    else return ch;
}

// static void toUppercase(char* string) {
//     while (*string != 0) {
//         *string = toUppercaseChar(*string);
//         ++string;
//     }
// }

static bool isHexDigit(char ch) {
    return ch >= '0' && ch <= '9' || ch >= 'a' && ch <= 'f' || ch >= 'A' && ch <= 'F';
}

static int hexDigitToNumber(char ch) {
    return ch >= '0' && ch <= '9'
        ? '0' - ch
        : 10 + 'A' - toUppercaseChar(ch);
}

struct TokenizerState getInitialTokenizerState() {
    return (struct TokenizerState){ false, { (struct Token){ TokenTypeNone, 0, NULL } }, 0, "", 0, 1, false, false, ' ', ' ' };
}

static void endToken(struct TokenizerState* state) {
    state->currentTokenStringValue[state->currentTokenStringValueIndex] = 0;
    state->currentTokenStringValueIndex = 0;
    int length = strlen(state->currentTokenStringValue);
    state->tokens[state->currentTokenIndex].stringValue = malloc(length + 1);
    memcpy(state->tokens[state->currentTokenIndex].stringValue, state->currentTokenStringValue, length + 1);
    ++state->currentTokenIndex;
}

static void validateNumberInRange(struct TokenizerState* state) {
    if (state->tokens[state->currentTokenIndex].numberValue < SHRT_MIN ||
        state->tokens[state->currentTokenIndex].numberValue > USHRT_MAX) {
        state->error = true;
        printf("Error on line %d: number out of range.", state->lineNumber);
    }
}
 
void parseChar(struct TokenizerState* state, int ch) {
    if (ch == '\n') {
        state->lineNumber++;
    }

    switch (state->tokens[state->currentTokenIndex].tokenType) {
        case TokenTypeNone:
            state->currentTokenStringValue[0] = ch;
            state->currentTokenStringValueIndex = 1;
            
            if (ch == '_' || ch >= 'a' && ch <= 'z' || ch >= 'A' && ch <= 'Z') {
                state->tokens[state->currentTokenIndex].tokenType = TokenTypeLabelUseOrInstruction;
            } else if (ch == '-') {
                state->tokens[state->currentTokenIndex].tokenType = TokenTypeMinus;
            } else if (ch == '0') {
                state->tokens[state->currentTokenIndex].tokenType = TokenTypeZero;
                state->tokens[state->currentTokenIndex].numberValue = 0;
            } else if (ch >= '1' && ch <= '9') {
                state->tokens[state->currentTokenIndex].tokenType = TokenTypeDecimalNumber;
                state->tokens[state->currentTokenIndex].numberValue = ch - '0';
            } else if (ch == '.') {
                state->tokens[state->currentTokenIndex].tokenType = TokenTypeDirective;
            } else if (ch == ';') {
                state->tokens[state->currentTokenIndex].tokenType = TokenTypeComment;
            } else if (ch == '\'') {
                state->tokens[state->currentTokenIndex].tokenType = TokenTypeNZTString;
                state->currentTokenStringValueIndex = 0;
            } else if (ch == '"') {
                state->tokens[state->currentTokenIndex].tokenType = TokenTypeZTString;
                state->currentTokenStringValueIndex = 0;
            } else if (!isspace(ch)) {
                state->error = true;
                printf("Error on line %d: unexpected character '%c'.", state->lineNumber, ch);
            }
            break;
        case TokenTypeComment:
            if (ch == '\n') {
                state->tokens[state->currentTokenIndex].tokenType = TokenTypeNone;
            }
            break;
        case TokenTypeLabelUseOrInstruction:
            if (ch == '_' || ch >= 'a' && ch <= 'z' || ch >= 'A' && ch <= 'Z' || ch >= '0' && ch <= '9') {
                if (state->currentTokenStringValueIndex + 1 >= MAX_NAME_LEN_INCL_0) {
                    state->error = true;
                    return printf("Error on line %d: label name too long.", state->lineNumber);
                } else {
                    state->currentTokenStringValue[state->currentTokenStringValueIndex++] = ch;
                }
            } else if (ch == ':' || isspace(ch)) {
                if (ch == ':') {
                    state->tokens[state->currentTokenIndex].tokenType = TokenTypeLabelDefinition;
                }
                endToken(state);
            } else {
                state->error = true;
                printf("Error on line %d: unexpected character '%c'.", state->lineNumber, ch);
            }
            break;
        case TokenTypeDirective:
            if (ch >= 'a' && ch <= 'z' || ch >= 'A' && ch <= 'Z') {
                state->currentTokenStringValue[state->currentTokenStringValueIndex++] = ch;
            } else if (isspace(ch)) {
                endToken(state);
            } else {
                state->error = true;
                printf("Error on line %d: unexpected character '%c'.", state->lineNumber, ch);
            }
            break;
        case TokenTypeDecimalNumber:
            if (ch >= '0' && ch <= '9') {
                state->currentTokenStringValue[state->currentTokenStringValueIndex++] = ch;
                int newDigit = '0' - ch;
                int sign = state->tokens[state->currentTokenIndex].numberValue >= 0 ? 1 : -1;
                state->tokens[state->currentTokenIndex].numberValue *= 10;
                state->tokens[state->currentTokenIndex].numberValue += sign * newDigit;
                validateNumberInRange(state);
            } else if (isspace(ch)) {
                endToken(state);
            } else {
                state->error = true;
                printf("Error on line %d: unexpected character '%c'.", state->lineNumber, ch);
            }
            break;
        case TokenTypeMinus:
            if (ch >= '0' && ch <= '9') {
                state->currentTokenStringValue[state->currentTokenStringValueIndex++] = ch;
                int newDigit = '0' - ch;
                state->tokens[state->currentTokenIndex].numberValue = -newDigit;
            } else {
                state->error = true;
                printf("Error on line %d: invalid use of '-'.", state->lineNumber);
            }
            break;
        case TokenTypeZero:
            if (ch >= '0' && ch <= '7') {
                state->currentTokenStringValue[state->currentTokenStringValueIndex++] = ch;
                state->tokens[state->currentTokenIndex].tokenType = TokenTypeOctalNumber;
                state->tokens[state->currentTokenIndex].numberValue = '0' - ch;
            } else if (ch == 'x' || ch == 'X') {
                state->currentTokenStringValue[state->currentTokenStringValueIndex++] = ch;
                state->tokens[state->currentTokenIndex].tokenType = TokenTypeHexNumber;
            } else if (ch == 'b' || ch == 'B') {
                state->currentTokenStringValue[state->currentTokenStringValueIndex++] = ch;
                state->tokens[state->currentTokenIndex].tokenType = TokenTypeBinaryNumber;
            } else if (isspace(ch)) {
                state->tokens[state->currentTokenIndex].tokenType = TokenTypeDecimalNumber;
                endToken(state);
            } else {
                state->error = true;
                printf("Error on line %d: unexpected character '%c'.", state->lineNumber, ch);
            }
            break;
        case TokenTypeHexNumber:
            if (isHexDigit(ch)) {
                state->currentTokenStringValue[state->currentTokenStringValueIndex++] = ch;
                state->tokens[state->currentTokenIndex].numberValue *= 0x10;
                state->tokens[state->currentTokenIndex].numberValue += hexDigitToNumber(ch);
                validateNumberInRange(state);
            } else if (isspace(ch)) {
                if (state->currentTokenStringValueIndex == 2) {
                    state->error = true;
                    printf("Error on line %d: number without digits.", state->lineNumber);
                } else {
                    endToken(state);
                }
            } else {
                state->error = true;
                printf("Error on line %d: unexpected character '%c'.", state->lineNumber, ch);
            }
            break;
        case TokenTypeOctalNumber:
            if (ch >= '0' && ch <= '7') {
                state->currentTokenStringValue[state->currentTokenStringValueIndex++] = ch;
                state->tokens[state->currentTokenIndex].numberValue *= 010;
                state->tokens[state->currentTokenIndex].numberValue += '0' - ch;
                validateNumberInRange(state);
            } else if (isspace(ch)) {
                endToken(state);
            } else {
                state->error = true;
                printf("Error on line %d: unexpected character '%c'.", state->lineNumber, ch);
            }
            break;
        case TokenTypeBinaryNumber:
            if (ch == '0' || ch == '1') {
                state->currentTokenStringValue[state->currentTokenStringValueIndex++] = ch;
                state->tokens[state->currentTokenIndex].numberValue *= 0b10;
                state->tokens[state->currentTokenIndex].numberValue += '0' - ch;
                validateNumberInRange(state);
            } else if (isspace(ch)) {
                if (state->currentTokenStringValueIndex == 2) {
                    state->error = true;
                    printf("Error on line %d: number without digits.", state->lineNumber);
                } else {
                    endToken(state);
                }
            } else {
                state->error = true;
                printf("Error on line %d: unexpected character '%c'.", state->lineNumber, ch);
            }
            break;
        case TokenTypeZTString:
        case TokenTypeNZTString:
            if (state->isHexEscapeSequence) {
                if (state->hexEscapeSequenceChar1 == 0) {
                    state->hexEscapeSequenceChar1 = ch;
                } else if (!isHexDigit(state->hexEscapeSequenceChar1) || !isHexDigit(ch)) {
                    state->error = true;
                    printf("Error on line %d: invalid escape sequence '\\x%c%c'.", state->lineNumber, state->hexEscapeSequenceChar1, ch);
                } else {
                    char escapeSequenceChar = 0x10 * hexDigitToNumber(state->hexEscapeSequenceChar1) + hexDigitToNumber(ch);
                    state->currentTokenStringValue[state->currentTokenStringValueIndex++] = escapeSequenceChar;
                    state->isHexEscapeSequence = false;
                    state->hexEscapeSequenceChar1 = 0;
                }
            } else if (state->isEscapeSequence) {
                state->isEscapeSequence = false;

                if (ch == 'x' || ch == 'X') {
                    state->isHexEscapeSequence = true;
                } else if (ch == '\'' || ch == '"' || ch == '\\') {
                    state->currentTokenStringValue[state->currentTokenStringValueIndex++] = ch;
                } else if (ch == 'n' || ch == 'N') {
                    state->currentTokenStringValue[state->currentTokenStringValueIndex++] = '\n';
                } else if (ch == 't' || ch == 'T') {
                    state->currentTokenStringValue[state->currentTokenStringValueIndex++] = '\t';
                } else if (ch == 'r' || ch == 'R') {
                    state->currentTokenStringValue[state->currentTokenStringValueIndex++] = '\r';
                } else {
                    state->error = true;
                    printf("Error on line %d: invalid escape sequence '\\%c'.", state->lineNumber, ch);
                }
            } else if (ch == '\\') {
                state->isEscapeSequence = true;
            } else if (ch == (state->tokens[state->currentTokenIndex].tokenType == TokenTypeZTString ? '"' : '\'')) {
                endToken(state);
            } else {
                state->currentTokenStringValue[state->currentTokenStringValueIndex++] = ch;
            }
            break;
    }
}

void validateEof(struct TokenizerState* state) {
    if (state->error) { return; }

    if (state->tokens[state->currentTokenIndex].tokenType == TokenTypeZero) {
        state->tokens[state->currentTokenIndex].tokenType = TokenTypeDecimalNumber;
    }

    if (state->tokens[state->currentTokenIndex].tokenType == TokenTypeMinus) {
        state->error = true;
        printf("Error on line %d: invalid use of '-'.", state->lineNumber);
    }

    if ((state->tokens[state->currentTokenIndex].tokenType == TokenTypeBinaryNumber
        || state->tokens[state->currentTokenIndex].tokenType == TokenTypeHexNumber)
        && state->currentTokenStringValueIndex == 2) {
        state->error = true;
        printf("Error on line %d: number without digits.", state->lineNumber);
    }

    if (state->tokens[state->currentTokenIndex].tokenType == TokenTypeZTString
        || state->tokens[state->currentTokenIndex].tokenType == TokenTypeNZTString) {
        state->error = true;
        printf("Error on line %d: unterminated string literal.", state->lineNumber);
    }

    if (!state->error) {
        endToken(state);
    }
}