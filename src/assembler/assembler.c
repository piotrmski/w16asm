#include "assembler.h"
#include "../tokenizer/tokenizer.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "../../common/exit-code.h"

#define MAX_LABEL_DEFS 0x800
#define MAX_LABEL_USES 0x2000
#define MAX_LABEL_NAME_LEN_INCL_0 0x20

enum Instruction {
    InstructionLd = 0,
    InstructionNot = 1,
    InstructionAdd = 2,
    InstructionAnd = 3,
    InstructionSt = 4,
    InstructionJmp = 5,
    InstructionJmn = 6,
    InstructionJmz = 7,
    InstructionInvalid
};

enum Directive {
    DirectiveOrg,
    DirectiveAlign,
    DirectiveFill,
    DirectiveLsb,
    DirectiveMsb,
    DirectiveInvalid
};

struct LabelDefinition {
    char* name;
    int lineNumber;
    int address;
};

struct LabelUse {
    char* name;
    int offset;
    int byte;
    int lineNumber;
    int address;
};

struct LabelUseParseResult {
    char* name;
    int offset;
};

struct EscapeSequenceParseResult {
    char character;
    int length;
};

struct AssemblerState {
    char* source;
    int lineNumber;
    int currentAddress;
    bool programMemoryWritten[ADDRESS_SPACE_SIZE];
    struct LabelDefinition labelDefinitions[MAX_LABEL_DEFS];
    int labelDefinitionsCount;
    struct LabelUse labelUses[MAX_LABEL_USES];
    int labelUsesCount;
    struct AssemblerResult result;
};

static void assertNoMemoryViolation(struct AssemblerState* state, int address, int lineNumber) {
    if (address < 0 || address >= ADDRESS_SPACE_SIZE) {
        printf("Error on line %d: attempting to declare memory value outside of address space.\n", lineNumber);
        exit(ExitCodeDeclaringValueOutOfMemoryRange);
    }

    if (state->programMemoryWritten[address]) {
        printf("Error on line %d: attempting to override memory value.\n", lineNumber);
        exit(ExitCodeMemoryValueOverridden);
    }
}

static char charUppercase(char ch) {
    if (ch >= 'a' && ch <= 'z') return ch - 0x20;
    else return ch;
}

static bool stringsEqualCaseInsensitive(char* string1, char* string2) {
    for (int i = 0;; ++i) {
        if (charUppercase(string1[i]) != charUppercase(string2[i])) {
            return false;
        }

        if (string1[i] == 0 || string2[i] == 0) {
            return true;
        }
    }
}

static enum Instruction getInstruction(char* name) {
    if (stringsEqualCaseInsensitive(name, "LD")) {
        return InstructionLd;
    } else if (stringsEqualCaseInsensitive(name, "NOT")) {
        return InstructionNot;
    } else if (stringsEqualCaseInsensitive(name, "ADD")) {
        return InstructionAdd;
    } else if (stringsEqualCaseInsensitive(name, "AND")) {
        return InstructionAnd;
    } else if (stringsEqualCaseInsensitive(name, "ST")) {
        return InstructionSt;
    } else if (stringsEqualCaseInsensitive(name, "JMP")) {
        return InstructionJmp;
    } else if (stringsEqualCaseInsensitive(name, "JMN")) {
        return InstructionJmn;
    } else if (stringsEqualCaseInsensitive(name, "JMZ")) {
        return InstructionJmz;
    } else {
        return InstructionInvalid;
    }
}

static enum Directive getDirective(char* name) {
    if (stringsEqualCaseInsensitive(name, ".ORG")) {
        return DirectiveOrg;
    } else if (stringsEqualCaseInsensitive(name, ".ALIGN")) {
        return DirectiveAlign;
    } else if (stringsEqualCaseInsensitive(name, ".FILL")) {
        return DirectiveFill;
    } else if (stringsEqualCaseInsensitive(name, ".LSB")) {
        return DirectiveLsb;
    } else if (stringsEqualCaseInsensitive(name, ".MSB")) {
        return DirectiveMsb;
    } else {
        return DirectiveInvalid;
    }
}

static bool isStringLiteral(char* tokenValue) {
    return tokenValue[0] = '"';
}

static bool isNumberLiteral(char* tokenValue) {
    return tokenValue[0] >= '0' && tokenValue[0] <= '9' || tokenValue[0] == '-' && tokenValue[0] >= '0' && tokenValue[0] <= '9';
}

static bool isCharacterLiteral(char* tokenValue) {
    return tokenValue[0] == '\'' || tokenValue[0] == '-' && tokenValue[1] == '\'';
}

struct LabelDefinition* findLabelDefinition(struct AssemblerState* state, struct LabelUse* labelUse) {
    for (int i = 0; i < state->labelDefinitionsCount; ++i) {
        if (strcmp(state->labelDefinitions[i].name, labelUse->name) == 0) {
            return &state->labelDefinitions[i];
        }
    }

    printf("Error on line %d: label \"%s\" is undefined.\n", labelUse->lineNumber, labelUse->name);
    exit(ExitCodeUndefinedLabel);
}

static struct Token getNextToken(struct AssemblerState* state) {
    return getToken(&state->source, &state->lineNumber);
}

static bool isValidLabelDefinitionRemoveColon(struct Token token, struct AssemblerState* state) {
    if (token.value[--token.length] != ':') {
        return false;
    }

    token.value[token.length] = 0; // Trim the trailing colon character

    if (token.length > MAX_LABEL_NAME_LEN_INCL_0 - 1) {
        printf("Error on line %d: label name too long.\n", token.lineNumber);
        exit(ExitCodeLabelNameTooLong);
    }

    for (int i = 0; i < token.length; ++i) {
        char ch = token.value[i];
        bool characterValid = ch == '_' || ch >= 'a' && ch <= 'z' || ch >= 'A' && ch <= 'Z' || i > 0 && ch >= '0' && ch <= 9;
        if (!characterValid) {
            printf("Error on line %d: \"%s\" is not a valid label name.\n", token.lineNumber, token.value);
            exit(ExitCodeInvalidLabelName);
        }
    }

    for (int i = 0; i < state->labelDefinitionsCount; ++i) {
        if (strcmp(state->labelDefinitions[i].name, token.value) == 0) {
            printf("Error on line %d: label name \"%s\" is not unique.\n", token.lineNumber, token.value);
            exit(ExitCodeLabelNameNotUnique);
        }
    }

    return true;
}

static int parseNumberLiteral(struct Token token) {
    char* endChar;
    int result = strtol(token.value, &endChar, 0);
    if (*endChar != 0) {
        printf("Error on line %d: \"%s\" is not a valid number.\n", token.lineNumber, token.value);
        exit(ExitCodeInvalidNumberLiteral);
    }
    return result;
}

static struct LabelUseParseResult parseLabelUse(struct Token token) {
    char* offsetSign = strpbrk(token.value, "+-");
    int offset;
    if (offsetSign != NULL) {
        offset = parseNumberLiteral((struct Token) { token.lineNumber, 0, offsetSign });
        *offsetSign = 0;
    }
    return (struct LabelUseParseResult) { token.value, offset };
}

static bool isHexDigit(char character) {
    return character >= '0' && character <= '9' || character >= 'a' && character <= 'z' || character >= 'A' && character <= 'Z';
}

static struct EscapeSequenceParseResult parseEscapeSequence(struct Token token) {
    switch (token.value[1]) {
        case 'n': 
        case 'N':
            return (struct EscapeSequenceParseResult) { '\n', 2 };
        case 't': 
        case 'T': 
            return (struct EscapeSequenceParseResult) { '\t', 2 };
        case 'r': 
        case 'R': 
            return (struct EscapeSequenceParseResult) { '\r', 2 };
        case '\'':
            return (struct EscapeSequenceParseResult) { '\'', 2 };
        case '"':
            return (struct EscapeSequenceParseResult) { '"', 2 };
        case '\\':
            return (struct EscapeSequenceParseResult) { '\\', 2 };
        case 'x':
        case 'X':
            if (!isHexDigit(token.value[2]) || !isHexDigit(token.value[3])) {
                printf("Error on line %d: invalid escape sequence starting with \"\\%c\".\n", token.lineNumber, token.value[1]);
                exit(ExitCodeInvalidEscapeSequence);
            }
            char numberString[5] = "0x00";
            numberString[2] = token.value[2];
            numberString[3] = token.value[3];
            unsigned char number = parseNumberLiteral((struct Token) { token.lineNumber, 5, numberString });
            return (struct EscapeSequenceParseResult) { number, 4 };
        default:
            printf("Error on line %d: invalid escape sequence \"\\%c\".\n", token.lineNumber, token.value[1]);
            exit(ExitCodeInvalidEscapeSequence);
    }
}

static char parseCharacterLiteral(struct Token token) {
    char* fullTokenValue = token.value;

    bool isNegative = token.value[0] == '-';

    if (isNegative) {
        ++token.value;
    }

    int charLength = 1;
    int character = (isNegative ? -1 : 1) * token.value[1];
    if (character == '\\') {
        struct EscapeSequenceParseResult parsed = parseEscapeSequence((struct Token) { token.lineNumber, 0, token.value + 1 });
        character = parsed.character;
        charLength = parsed.length;
    }

    if (token.value[0] != '\''
        || token.value[charLength + 1] != '\''
        || !(token.value[charLength + 2] == '+'
            || token.value[charLength + 2] == '-'
            || token.value[charLength + 2] == 0)) {
        printf("Error on line %d: \"%s\" is not a valid character literal.\n", token.lineNumber, fullTokenValue);
        exit(ExitCodeInvalidCharacterLiteral);
    }

    if (token.value[charLength + 2] == 0) {
        return character;
    }

    int offset = parseNumberLiteral((struct Token) { token.lineNumber, 0, token.value + charLength + 2 });
    return character + offset;
 }

static void insertInstruction(struct AssemblerState* state, enum Instruction instruction) {
    assertNoMemoryViolation(state, state->currentAddress, state->lineNumber);
    assertNoMemoryViolation(state, state->currentAddress + 1, state->lineNumber);
    state->programMemoryWritten[state->currentAddress] = true;
    state->programMemoryWritten[state->currentAddress + 1] = true;
    state->result.dataType[state->currentAddress] = DataTypeInstruction;

    unsigned short instructionCode = instruction << 13;
    struct Token param = getNextToken(state);

    if (isNumberLiteral(param.value)) {
        int paramValue = parseNumberLiteral(param);
        if (paramValue < 0 || paramValue >= ADDRESS_SPACE_SIZE) {
            printf("Error on line %d: attempting to reference invalid address 0x%04X.\n", param.lineNumber, paramValue);
            exit(ExitCodeReferenceToInvalidAddress);
        } 
        instructionCode |= paramValue;
    } else {
        struct LabelUseParseResult labelUse = parseLabelUse(param);
        state->labelUses[state->labelUsesCount++] =
            (struct LabelUse) { labelUse.name, labelUse.offset, 0, param.lineNumber, state->currentAddress };
        state->labelUses[state->labelUsesCount++] =
            (struct LabelUse) { labelUse.name, labelUse.offset, 1, param.lineNumber, state->currentAddress + 1 };
    }
    state->result.programMemory[state->currentAddress++] = instructionCode;
    state->result.programMemory[state->currentAddress++] = instructionCode >> 8;
}

static void applyDirective(struct AssemblerState* state, enum Directive directive, int labelDefinitionsStartIndex) {
    // TODO
}

static void declareString(struct AssemblerState* state, struct Token token) {
    for (int i = 1; i < token.length - 1; ++i) {
        assertNoMemoryViolation(state, state->currentAddress, state->lineNumber);
        state->programMemoryWritten[state->currentAddress] = true;
        state->result.dataType[state->currentAddress] = DataTypeChar;
        if (token.value[i] == '\\') {
            struct EscapeSequenceParseResult parsed = parseEscapeSequence((struct Token) { token.length, 0, token.value + i });
            state->result.programMemory[state->currentAddress] = parsed.character;
            state->currentAddress += parsed.length;
        } else {
            state->result.programMemory[state->currentAddress++] = token.value[i];
        }
    }
}

static void declareNumber(struct AssemblerState* state, struct Token token) {
    assertNoMemoryViolation(state, state->currentAddress, state->lineNumber);
    state->programMemoryWritten[state->currentAddress] = true;
    state->result.dataType[state->currentAddress] = DataTypeInt;
    int number = parseNumberLiteral(token);
    if (number < CHAR_MIN || number > UCHAR_MAX) {
        printf("Error on line %d: number %d is out of range.\n", token.lineNumber, number);
        exit(ExitCodeNumberLiteralOutOutRange);
    }
    state->result.programMemory[state->currentAddress++] = number;
}

static void declareCharacter(struct AssemblerState* state, struct Token token) {
    assertNoMemoryViolation(state, state->currentAddress, state->lineNumber);
    state->programMemoryWritten[state->currentAddress] = true;
    state->result.dataType[state->currentAddress] = DataTypeChar;
    int number = parseCharacterLiteral(token);
    if (number < CHAR_MIN || number > UCHAR_MAX) {
        printf("Error on line %d: character literal \"%s\" evaluates to %d, which is out of range.\n", token.lineNumber, token.value, number);
        exit(ExitCodeCharacterLiteralOutOutRange);
    }
    state->result.programMemory[state->currentAddress++] = number;
}

static struct Token parseLabelDefinitionsGetNextToken(struct AssemblerState* state) {
    struct Token token;

    while (true) {
        token = getNextToken(state);
        if (isValidLabelDefinitionRemoveColon(token, state)) {
            if (state->labelDefinitionsCount == MAX_LABEL_DEFS - 1) {
                printf("Error on line %d: too many label definitions.\n", token.lineNumber);
                exit(ExitCodeTooManyLabelDefinitions);
            }
            state->labelDefinitions[state->labelDefinitionsCount++] = (struct LabelDefinition) { token.value, token.lineNumber, state->currentAddress };
        } else {
            break;
        }
    }

    return token;
}

/// Returns true if statement parsing should continue
static bool parseStatement(struct AssemblerState* state) {
    int labelDefinitionsStartIndex = state->labelDefinitionsCount;
    struct Token firstTokenAfterLabels = parseLabelDefinitionsGetNextToken(state);

    if (firstTokenAfterLabels.value == NULL) {
        if (state->labelDefinitionsCount > labelDefinitionsStartIndex) {
            printf("Error on line %d: unexpected label definition at the end of the file.\n", state->lineNumber);
            exit(ExitCodeUnexpectedLabelAtEndOfFile);
        }

        return false;
    }

    enum Instruction instruction;
    enum Directive directive;

    if ((instruction = getInstruction(firstTokenAfterLabels.value)) != InstructionInvalid) {
        insertInstruction(state, instruction);
    } else if ((directive = getDirective(firstTokenAfterLabels.value)) != DirectiveInvalid) {
        applyDirective(state, directive, labelDefinitionsStartIndex);
    } else if (isStringLiteral(firstTokenAfterLabels.value)) {
        declareString(state, firstTokenAfterLabels);
    } else if (isNumberLiteral(firstTokenAfterLabels.value)) {
        declareNumber(state, firstTokenAfterLabels);
    } else if (isCharacterLiteral(firstTokenAfterLabels.value)) {
        declareCharacter(state, firstTokenAfterLabels);
    } else {
        printf("Error on line %d: invalid token \"%s\".\n", firstTokenAfterLabels.lineNumber, firstTokenAfterLabels.value);
        exit(ExitCodeInvalidToken);
    }

    return true;
}

static void resolveLabels(struct AssemblerState* state) {
    for (int i = 0; i < state->labelUsesCount; ++i) {
        struct LabelUse* labelUse = &state->labelUses[i];
        struct LabelDefinition* labelDefinition = findLabelDefinition(state, labelUse);
        int evaluatedAddress = labelDefinition->address + labelUse->offset;
        state->result.programMemory[labelUse->address] = evaluatedAddress >> (labelUse->address * 8);
    }
}

struct AssemblerResult assemble(char* source) {
    struct AssemblerState state = { source, 1, 0, { false }, { 0 }, 0, { 0 }, 0, (struct AssemblerResult){ { 0 }, { DataTypeNone }, { NULL } } };

    while (parseStatement(&state)) {}

    resolveLabels(&state);

    return state.result;
}