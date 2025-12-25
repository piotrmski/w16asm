#include "tokenizer.h"
#include "stdbool.h"
#include "ctype.h"
#include "stdlib.h"
#include "../../common/exit-code.h"

struct Token getToken(char** string, int* lineNumber) {
    bool isComment = false;

    // Skip over non-token characters before the token
    while (isspace(**string) || isComment || **string == ';') {
        if (**string == ';') {
            isComment = true;
        } else if (**string == '\n') {
            isComment = false;
            ++*lineNumber;
        }

        ++*string;
    }

    if (**string == 0) {
        return (struct Token) { *lineNumber, NULL };
    }

    int tokenStartLineNumber = *lineNumber;

    char* result = *string;

    if (*result == '"') {
        do { // Find the end of the string
            if (**string == '\n') {
                ++*lineNumber;
            }

            if (**string == '\\' && *(*string + 1) == '"') {
                *string += 2;
            } else {
                ++*string;
            }
        } while (**string != '"' && **string != 0);

        if (**string == 0) {
            printf("Error on line %d: unterminated string literal.\n", *lineNumber);
            exit(ExitCodeUnterminatedString);
        }

        // Zero-terminate the token. **string should be '"', *(*string + 1) should be whitespace, *(*string + 2) should be the next token.
        if (*(*string + 1) == 0) {
            ++*string;
        } else {
            if (!isspace(*(*string + 1)) && *(*string + 1) != ';') {
                printf("Error on line %d: unexpected character '%c'.\n", *lineNumber, *(*string + 1));
                exit(ExitCodeUnexpectedCharacter);
            }

            if (*(*string + 1) == ';' && *(*string + 2) != '\n' && *(*string + 2) != 0) { // If a non-empty comment follows the token
                *(*string + 2) = ';'; // Move the start of the comment one character forward
            }

            if (*(*string + 1) == '\n') {
                ++*lineNumber;
            }
        
            *(*string + 1) = 0;
            *string += 2;
        }
    } else {
        while (!isspace(**string) && **string != ';' && **string != 0) { // Find the end of the token
            ++*string;
        }

        if (**string == 0) {
            return (struct Token) { tokenStartLineNumber, result };
        }

        if (**string == ';' && *(*string + 1) != '\n' && *(*string + 1) != 0) { // If a non-empty comment follows the token
            *(*string + 1) = ';'; // Move the start of the comment one character forward
        }

        if (**string == '\n') {
            ++*lineNumber;
        }

        // Zero-terminate the token
        **string = 0;
        ++*string;
    }

    return (struct Token) { tokenStartLineNumber, result };
}
