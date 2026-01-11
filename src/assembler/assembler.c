#include "assembler.h"
#include "../shared/shared.h"
#include "../../common/exit-code.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#define MAX_LABEL_DEFS 0x2000
#define MAX_LABEL_USES 0x2000
#define MAX_IMMEDIATE_VAL_USES 0x2000
#define MAX_LABEL_NAME_LEN_INCL_0 0x20

enum Directive {
    DirectiveOrg,
    DirectiveAlign,
    DirectiveFill,
    DirectiveLsb,
    DirectiveMsb,
    DirectiveImmediates,
    DirectiveInvalid
};

enum NumberLiteralRange {
    NumberLiteralRangeNone,
    NumberLiteralRangeByte,
    NumberLiteralRangeAddress
};

struct LabelDefinition {
    char* name;
    int address;
};

struct LabelUse {
    char* name;
    int offset;
    int byte;
    int lineNumber;
    int address;
};

struct ImmediateValueUse {
    struct Token token;
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

static struct Token* tokensArray;
static int currentAddress = 0;
static bool programMemoryWritten[ADDRESS_SPACE_SIZE] = { false };
static struct LabelDefinition labelDefinitions[MAX_LABEL_DEFS];
static int labelDefinitionsCount = 0;
static struct LabelUse labelUses[MAX_LABEL_USES];
static int labelUsesCount = 0;
static struct ImmediateValueUse immediateValueUses[MAX_IMMEDIATE_VAL_USES];
static int immediateValueUsesCount = 0;
static char* labelNamesByImmediateValue[256] = { NULL };
static struct AssemblerResult result = (struct AssemblerResult){ { 0 }, { DataTypeNone }, { NULL } };

static void assertNoMemoryViolation(int address, int lineNumber) {
    if (address < 0 || address >= ADDRESS_SPACE_SIZE) {
        printf("Error on line %d: attempting to declare memory value outside of address space.\n", lineNumber);
        exit(ExitCodeDeclaringValueOutOfMemoryRange);
    }

    if (programMemoryWritten[address]) {
        printf("Error on line %d: attempting to override memory value.\n", lineNumber);
        exit(ExitCodeMemoryValueOverridden);
    }

    programMemoryWritten[address] = true;
}

static void assertCanAddLabelDefinition(int lineNumber) {
    if (labelDefinitionsCount == MAX_LABEL_DEFS - 1) {
        printf("Error on line %d: too many label definitions.\n", lineNumber);
        exit(ExitCodeTooManyLabelDefinitions);
    }
}

static void assertCanAddLabelUses(int count, int lineNumber) {
    if (labelUsesCount == MAX_LABEL_USES - count) {
        printf("Error on line %d: too many label uses.\n", lineNumber);
        exit(ExitCodeTooManyLabelUses);
    }
}

static void assertCanAddImmediateValue(int lineNumber) {
    if (immediateValueUsesCount == MAX_IMMEDIATE_VAL_USES - 1) {
        printf("Error on line %d: too many immediate value uses.\n", lineNumber);
        exit(ExitCodeTooManyImmediateValueUses);
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
    } else if (stringsEqualCaseInsensitive(name, ".IMMEDIATES")) {
        return DirectiveImmediates;
    } else {
        return DirectiveInvalid;
    }
}

static bool isStringLiteral(char* tokenValue) {
    return tokenValue[0] == '"';
}

static bool isNumberLiteral(char* tokenValue) {
    return tokenValue[0] >= '0' && tokenValue[0] <= '9' || tokenValue[0] == '-' && tokenValue[1] >= '0' && tokenValue[1] <= '9';
}

static bool isCharacterLiteral(char* tokenValue) {
    return tokenValue[0] == '\'' || tokenValue[0] == '-' && tokenValue[1] == '\'';
}

static bool isImmediateValue(char* tokenValue) {
    return tokenValue[0] == '#';
}

static bool instructionAcceptsImmediateValue(enum Instruction instruction) {
    return instruction < InstructionSt;
}

static void trimComma(struct Token* token) {
    if (token->value[token->length - 1] != ',') {
        printf("Error on line %d: \"%s\" was not expected to be the last argument and was expected to end with a comma.\n", token->lineNumber, token->value);
        exit(ExitCodeMissingComma);
    }

    token->length--;
    token->value[token->length] = 0;
}

struct LabelDefinition* findLabelDefinition(struct LabelUse* labelUse) {
    for (int i = 0; i < labelDefinitionsCount; ++i) {
        if (strcmp(labelDefinitions[i].name, labelUse->name) == 0) {
            return &labelDefinitions[i];
        }
    }

    printf("Error on line %d: label \"%s\" is undefined.\n", labelUse->lineNumber, labelUse->name);
    exit(ExitCodeUndefinedLabel);
}

static struct Token getNextToken() {
    if (tokensArray->value == NULL) {
        return *tokensArray;
    }
    return *(tokensArray++);
}

static struct Token getNextNonEmptyToken() {
    struct Token result = getNextToken();
    if (result.value == NULL) {
        printf("Error on line %d: unexpected end of file.\n", result.lineNumber);
        exit(ExitCodeUnexpectedEndOfFile);
    }
    return result;
}

static bool isValidLabelDefinitionRemoveColon(struct Token token) {
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
        bool characterValid = ch == '_' || ch >= 'a' && ch <= 'z' || ch >= 'A' && ch <= 'Z' || i > 0 && ch >= '0' && ch <= '9';
        if (!characterValid) {
            printf("Error on line %d: \"%s\" is not a valid label name.\n", token.lineNumber, token.value);
            exit(ExitCodeInvalidLabelName);
        }
    }

    for (int i = 0; i < labelDefinitionsCount; ++i) {
        if (strcmp(labelDefinitions[i].name, token.value) == 0) {
            printf("Error on line %d: label name \"%s\" is not unique.\n", token.lineNumber, token.value);
            exit(ExitCodeLabelNameNotUnique);
        }
    }

    return true;
}

static int parseNumberLiteral(struct Token token, enum NumberLiteralRange range) {
    char* endChar;
    int result = strtol(token.value, &endChar, 0);

    if (errno  != 0 || *endChar != 0) {
        printf("Error on line %d: \"%s\" is not a valid number.\n", token.lineNumber, token.value);
        exit(ExitCodeInvalidNumberLiteral);
    }

    if (range == NumberLiteralRangeByte) {
        if (result < CHAR_MIN || result > UCHAR_MAX) {
            printf("Error on line %d: number %d is out of range.\n", token.lineNumber, result);
            exit(ExitCodeNumberLiteralOutOutRange);
        }
    } else if (range == NumberLiteralRangeAddress) {
        if (result < 0 || result >= ADDRESS_SPACE_SIZE) {
            printf("Error on line %d: attempting to reference invalid address 0x%04X.\n", token.lineNumber, result);
            exit(ExitCodeReferenceToInvalidAddress);
        } 
    }
    return result;
}

static struct LabelUseParseResult parseLabelUse(struct Token token) {
    char* offsetSign = strpbrk(token.value, "+-");
    int offset = 0;
    if (offsetSign != NULL) {
        offset = parseNumberLiteral((struct Token) { token.lineNumber, 0, offsetSign }, NumberLiteralRangeNone);
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
            unsigned char number = parseNumberLiteral((struct Token) { token.lineNumber, 5, numberString }, NumberLiteralRangeNone);
            return (struct EscapeSequenceParseResult) { number, 4 };
        default:
            printf("Error on line %d: invalid escape sequence \"\\%c\".\n", token.lineNumber, token.value[1]);
            exit(ExitCodeInvalidEscapeSequence);
    }
}

static unsigned char parseCharacterLiteral(struct Token token) {
    char* fullTokenValue = token.value;

    bool isNegative = token.value[0] == '-';

    if (isNegative) {
        ++token.value;
    }

    int charLength = 1;
    int character = (isNegative ? -1 : 1) * token.value[1];
    if (token.value[1] == '\\') {
        struct EscapeSequenceParseResult parsed = parseEscapeSequence((struct Token) { token.lineNumber, 0, token.value + 1 });
        character = (isNegative ? -1 : 1) * parsed.character;
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

    int offset = parseNumberLiteral((struct Token) { token.lineNumber, 0, token.value + charLength + 2 }, NumberLiteralRangeNone);
    int result = character + offset;

    if (result < CHAR_MIN || result > UCHAR_MAX) {
        printf("Error on line %d: character literal \"%s\" evaluates to %d, which is out of range.\n", token.lineNumber, token.value, result);
        exit(ExitCodeCharacterLiteralOutOutRange);
    }

    return result;
 }

static void insertInstruction(enum Instruction instruction, int lineNumber) {
    assertNoMemoryViolation(currentAddress, lineNumber);
    assertNoMemoryViolation(currentAddress + 1, lineNumber);
    result.dataType[currentAddress] = DataTypeInstruction;

    unsigned short instructionCode = instruction << 13;
    struct Token param = getNextNonEmptyToken();

    if (isNumberLiteral(param.value)) {
        int paramValue = parseNumberLiteral(param, NumberLiteralRangeAddress);
        instructionCode |= paramValue;
    } else if (!isImmediateValue(param.value)) {
        assertCanAddLabelUses(2, param.lineNumber);
        struct LabelUseParseResult labelUse = parseLabelUse(param);
        labelUses[labelUsesCount++] =
            (struct LabelUse) { labelUse.name, labelUse.offset, 0, param.lineNumber, currentAddress };
        labelUses[labelUsesCount++] =
            (struct LabelUse) { labelUse.name, labelUse.offset, 1, param.lineNumber, currentAddress + 1 };
    } else {
        assertCanAddImmediateValue(param.lineNumber);
        if (!instructionAcceptsImmediateValue(instruction)) {
            printf("Error on line %d: instruction \"%s\" does not accept an immediate value as an argument.\n", param.lineNumber, getInstructionName(instruction));
            exit(ExitCodeInvalidInstructionArgument);
        }
        immediateValueUses[immediateValueUsesCount++] = (struct ImmediateValueUse) { param, currentAddress };
    }
    result.programMemory[currentAddress++] = instructionCode;
    result.programMemory[currentAddress++] = instructionCode >> 8;
}

static void updateCurrentAddress(int newAddress, int lineNumber, int labelDefinitionsStartIndex) {
    if (newAddress < 0 || newAddress >= ADDRESS_SPACE_SIZE) {
        printf("Error on line %d: attempting to set origin to an invalid address 0x%04X.\n", lineNumber, newAddress);
        exit(ExitCodeOriginOutOfMemoryRange);
    }
    currentAddress = newAddress;
    for (int i = labelDefinitionsStartIndex; i < labelDefinitionsCount; ++i) {
        labelDefinitions[i].address = newAddress;
    }
}

static void applyOrgDirective(int labelDefinitionsStartIndex) {
    struct Token param = getNextNonEmptyToken();
    int paramValue = parseNumberLiteral(param, NumberLiteralRangeNone);
    updateCurrentAddress(paramValue, param.lineNumber, labelDefinitionsStartIndex);
}

static void applyAlignDirective(int labelDefinitionsStartIndex) {
    struct Token param = getNextNonEmptyToken();
    int paramValue = parseNumberLiteral(param, NumberLiteralRangeNone);
    if (paramValue < 1 || paramValue > 12) {
        printf("Error on line %d: invalid align argument \"%d\". Must be between 1 and 12.\n", param.lineNumber, paramValue);
        exit(ExitCodeInvalidDirectiveArgument);
    }
    unsigned short bitsToReset = (1 << paramValue) - 1;
    int newAddress = (currentAddress & bitsToReset) == 0
        ? currentAddress
        : ((currentAddress & ~bitsToReset) + bitsToReset + 1);
    updateCurrentAddress(newAddress, param.lineNumber, labelDefinitionsStartIndex);
}

static void applyFillDirective() {
    struct Token valueParam = getNextNonEmptyToken();
    trimComma(&valueParam);
    struct Token countParam = getNextNonEmptyToken();

    unsigned char value, count;
    enum DataType valueToFillType;
    
    if (isCharacterLiteral(valueParam.value)) {
        valueToFillType = DataTypeChar;
        value = parseCharacterLiteral(valueParam);
    } else if (isNumberLiteral(valueParam.value)) {
        valueToFillType = DataTypeInt;
        value = parseNumberLiteral(valueParam, NumberLiteralRangeByte);
    } else {
        printf("Error on line %d: \"%s\" is neither a character nor a number.\n", valueParam.lineNumber, valueParam.value);
        exit(ExitCodeInvalidDirectiveArgument);
    }

    count = parseNumberLiteral(countParam, NumberLiteralRangeNone);
    if (count < 1) {
        printf("Error on line %d: fill count must be positive.\n", countParam.lineNumber);
        exit(ExitCodeInvalidDirectiveArgument);
    }

    for (int i = 0; i < count; ++i) {
        assertNoMemoryViolation(currentAddress, countParam.lineNumber);
        result.dataType[currentAddress] = valueToFillType;
        result.programMemory[currentAddress++] = value;
    }
}

static void applyLsbOrMsbDirective(enum Directive directive) {
    struct Token param = getNextNonEmptyToken();
    struct LabelUseParseResult labelUse = parseLabelUse(param);
    int byte = directive == DirectiveLsb ? 0 : 1;
    assertNoMemoryViolation(currentAddress, param.lineNumber);
    result.dataType[currentAddress] = DataTypeInt;
    assertCanAddLabelUses(1, param.lineNumber);
    labelUses[labelUsesCount++] =
        (struct LabelUse) { labelUse.name, labelUse.offset, byte, param.lineNumber, currentAddress++ };
}

static void resolveImmediateValues() {
    for (int i = 0; i < immediateValueUsesCount; ++i) {
        struct Token token = immediateValueUses[i].token;
        struct Token valueToken = (struct Token) { token.lineNumber, token.length - 1, token.value + 1 };

        unsigned char value = isCharacterLiteral(valueToken.value)
            ? parseCharacterLiteral(valueToken)
            : parseNumberLiteral(valueToken, NumberLiteralRangeByte);
        enum DataType dataType = isCharacterLiteral(valueToken.value) ? DataTypeChar : DataTypeInt;
        
        if (labelNamesByImmediateValue[value] == NULL) {
            if (currentAddress >= ADDRESS_SPACE_SIZE) {
                printf("Error on line %d: can't add immediate values after the last explicit value declaration due to insufficient space.\n", token.lineNumber);
                exit(ExitCodeImmediateValueDeclarationOutOfMemoryRange);
            }
            assertNoMemoryViolation(currentAddress, token.lineNumber);
            assertCanAddLabelDefinition(token.lineNumber);
            labelNamesByImmediateValue[value] = token.value;
            labelDefinitions[labelDefinitionsCount++] = (struct LabelDefinition) { token.value, currentAddress };
            result.dataType[currentAddress] = dataType;
            result.programMemory[currentAddress++] = value;
        }

        assertCanAddLabelUses(2, token.lineNumber);
        labelUses[labelUsesCount++] =
            (struct LabelUse) { labelNamesByImmediateValue[value], 0, 0, token.lineNumber, immediateValueUses[i].address };
        labelUses[labelUsesCount++] =
            (struct LabelUse) { labelNamesByImmediateValue[value], 0, 1, token.lineNumber, immediateValueUses[i].address + 1 };
    }

    immediateValueUsesCount = 0;
}

static void applyImmediatesDirective() {
    resolveImmediateValues();
}

static void applyDirective(enum Directive directive, int labelDefinitionsStartIndex) {
    switch (directive) {
        case DirectiveOrg: return applyOrgDirective(labelDefinitionsStartIndex);
        case DirectiveAlign: return applyAlignDirective(labelDefinitionsStartIndex);
        case DirectiveFill: return applyFillDirective();
        case DirectiveLsb:
        case DirectiveMsb: return applyLsbOrMsbDirective(directive);
        case DirectiveImmediates: return applyImmediatesDirective();
        case DirectiveInvalid: break;
    }
}

static void declareString(struct Token token) {
    for (int i = 1; i < token.length - 1; ++i) {
        assertNoMemoryViolation(currentAddress, token.lineNumber);
        result.dataType[currentAddress] = DataTypeChar;
        if (token.value[i] == '\\') {
            struct EscapeSequenceParseResult parsed = parseEscapeSequence((struct Token) { token.length, 0, token.value + i });
            result.programMemory[currentAddress++] = parsed.character;
            i += parsed.length - 1;
        } else {
            result.programMemory[currentAddress++] = token.value[i];
        }
    }
    assertNoMemoryViolation(currentAddress, token.lineNumber);
    result.dataType[currentAddress] = DataTypeChar;
    result.programMemory[currentAddress++] = 0;
}

static void declareNumber(struct Token token) {
    assertNoMemoryViolation(currentAddress, token.lineNumber);
    result.dataType[currentAddress] = DataTypeInt;
    result.programMemory[currentAddress++] = parseNumberLiteral(token, NumberLiteralRangeByte);
}

static void declareCharacter(struct Token token) {
    assertNoMemoryViolation(currentAddress, token.lineNumber);
    result.dataType[currentAddress] = DataTypeChar;
    result.programMemory[currentAddress++] = parseCharacterLiteral(token);
}

static struct Token parseLabelDefinitionsGetNextToken() {
    struct Token token;

    while (true) {
        token = getNextToken();
        if (token.value != NULL && isValidLabelDefinitionRemoveColon(token)) {
            assertCanAddLabelDefinition(token.lineNumber);
            labelDefinitions[labelDefinitionsCount++] = (struct LabelDefinition) { token.value, currentAddress };
        } else {
            break;
        }
    }

    return token;
}

/// Returns true if statement parsing should continue
static bool parseStatement() {
    int labelDefinitionsStartIndex = labelDefinitionsCount;
    struct Token firstTokenAfterLabels = parseLabelDefinitionsGetNextToken();

    if (firstTokenAfterLabels.value == NULL) {
        if (labelDefinitionsCount > labelDefinitionsStartIndex) {
            printf("Error on line %d: unexpected label definition at the end of the file.\n", firstTokenAfterLabels.lineNumber);
            exit(ExitCodeUnexpectedEndOfFile);
        }

        return false;
    }

    enum Instruction instruction;
    enum Directive directive;

    if ((instruction = getInstruction(firstTokenAfterLabels.value)) != InstructionInvalid) {
        insertInstruction(instruction, firstTokenAfterLabels.lineNumber);
    } else if ((directive = getDirective(firstTokenAfterLabels.value)) != DirectiveInvalid) {
        applyDirective(directive, labelDefinitionsStartIndex);
    } else if (isStringLiteral(firstTokenAfterLabels.value)) {
        declareString(firstTokenAfterLabels);
    } else if (isNumberLiteral(firstTokenAfterLabels.value)) {
        declareNumber(firstTokenAfterLabels);
    } else if (isCharacterLiteral(firstTokenAfterLabels.value)) {
        declareCharacter(firstTokenAfterLabels);
    } else {
        printf("Error on line %d: invalid token \"%s\".\n", firstTokenAfterLabels.lineNumber, firstTokenAfterLabels.value);
        exit(ExitCodeInvalidToken);
    }

    return true;
}

static void resolveLabels() {
    for (int i = labelDefinitionsCount - 1; i >= 0; --i) {
        result.labelNameByAddress[labelDefinitions[i].address] = labelDefinitions[i].name;
    }

    for (int i = 0; i < labelUsesCount; ++i) {
        struct LabelUse* labelUse = &labelUses[i];
        struct LabelDefinition* labelDefinition = findLabelDefinition(labelUse);
        int evaluatedAddress = labelDefinition->address + labelUse->offset;
        
        if (evaluatedAddress < 0 || evaluatedAddress >= ADDRESS_SPACE_SIZE) {
            printf("Error on line %d: \"%s%s%d\" evaluates to %d, which is an invalid address.\n", labelUse->lineNumber, labelUse->name, labelUse->offset < 0 ? "" : "+", labelUse->offset, evaluatedAddress);
            exit(ExitCodeReferenceToInvalidAddress);
        }
        
        result.programMemory[labelUse->address] |= evaluatedAddress >> (labelUse->byte * 8);
    }
}

struct AssemblerResult assemble(struct Token* tokens) {
    tokensArray = tokens;

    while (parseStatement()) {}

    for (int i = 0; i < ADDRESS_SPACE_SIZE; ++i) {
        if (programMemoryWritten[i]) {
            currentAddress = i + 1;
        }
    }

    resolveImmediateValues();
    resolveLabels();

    return result;
}