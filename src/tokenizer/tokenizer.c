#include "tokenizer.h"
#include "stdbool.h"
#include "ctype.h"
#include "stdlib.h"
#include "../../common/exit-code.h"

static void skipUntilTokenStart(char** string, int* lineNumber) {
    bool isComment = false;

    while (isspace(**string) || isComment || **string == ';') {
        if (**string == ';') {
            isComment = true;
        } else if (**string == '\n') {
            isComment = false;
            ++*lineNumber;
        }

        ++*string;
    }
}

static void skipUntilAfterStringEnd(char** string, int* lineNumber) {
    char terminator = **string;

    do {
        if (**string == '\n') {
            ++*lineNumber;
        }

        if (**string == '\\' && *(*string + 1) == terminator) {
            *string += 2;
        } else {
            ++*string;
        }
    } while (**string != terminator && **string != 0);

    if (**string == 0) {
        printf("Error on line %d: unterminated %s literal.\n", *lineNumber, terminator == '"' ? "string" : "character");
        exit(ExitCodeUnterminatedString);
    }

    ++*string;
}

static void skipUntilAfterTokenEnd(char** string, int* lineNumber) {
    while (!isspace(**string) && **string != ';' && **string != 0) {
        if (**string == '"' || **string == '\'') {
            skipUntilAfterStringEnd(string, lineNumber);
        } else {
            ++*string;
        }
    }
}

static void zeroTerminate(char** string, int* lineNumber) {
    if (**string == ';' && *(*string + 1) != '\n' && *(*string + 1) != 0) { // If a non-empty comment follows the token
        *(*string + 1) = ';'; // Move the start of the comment one character forward
    }

    if (**string == '\n') {
        ++*lineNumber;
    }

    **string = 0;
    ++*string;
}

struct Token getToken(char** string, int* lineNumber) {
    skipUntilTokenStart(string, lineNumber);

    if (**string == 0) {
        return (struct Token) { *lineNumber, 0, NULL };
    }

    int tokenStartLineNumber = *lineNumber;

    char* result = *string;

    skipUntilAfterTokenEnd(string, lineNumber);

    char* end = *string;

    if (**string != 0) {
        zeroTerminate(string, lineNumber);
    }

    return (struct Token) { tokenStartLineNumber, end - result, result };
}
