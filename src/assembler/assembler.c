#include "assembler.h"
#include "../tokenizer/tokenizer.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "../../common/exit-code.h"

#define MAX_LABELS 0x800

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

struct LabelInfo {
    char* name;
    int lineNumber;
    int address;
};

// struct AssemblerState {
//     int address;
//     bool programMemoryWritten[ADDRESS_SPACE_SIZE];
//     bool lastTokenWasLabelDefinition;
//     struct LabelInfo labelDefinitions[MAX_LABELS];
//     int labelDefinitionsIndex;
//     struct LabelInfo labelUses[MAX_LABELS];
//     int labelUsesIndex;
// };

// static void assertNoMemoryViolation(struct AssemblerState* state, struct Token* token, int address) {
//     if (address >= ADDRESS_SPACE_SIZE) {
//         printf("Error on line %d: attempting to declare memory value outside of address space.\n", token->lineNumber);
//         exit(ExitCodeDeclaringValueOutOfMemoryRange);
//     }

//     if (state->programMemoryWritten[address]) {
//         printf("Error on line %d: attempting to override memory value.\n", token->lineNumber);
//         exit(ExitCodeMemoryValueOverridden);
//     }
// }

// static void assertNumberLiteralInRange(struct Token* token) {
//     if (token->numberValue < CHAR_MIN ||
//         token->numberValue > UCHAR_MAX) {
//         printf("Error on line %d: number %d is out of range for a number literal.\n", token->lineNumber, token->numberValue);
//         exit(ExitCodeNumberLiteralOutOutRange);
//     }
// }

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

// static void parseToken(struct Token* token, struct AssemblerState* state, struct AssemblerResult* result, FILE* filePtr) {
//     switch (token->type) {
//         case TokenTypeNone:
//             break;
//         case TokenTypeLabelDefinition:
//             if (state->labelDefinitionsIndex >= MAX_LABELS) {
//                 printf("Error on line %d: too many labels.\n", token->lineNumber);
//                 exit(ExitCodeTooManyLabels);
//             }
//             state->labelDefinitions[state->labelDefinitionsIndex].name = token->stringValue;
//             state->labelDefinitions[state->labelDefinitionsIndex].lineNumber = token->lineNumber;
//             state->labelDefinitions[state->labelDefinitionsIndex++].address = state->address;
//             state->lastTokenWasLabelDefinition = true;
//             for (int i = 0; i < state->labelDefinitionsIndex - 1; ++i) {
//                 if (strcmp(state->labelDefinitions[i].name, token->stringValue) == 0) {
//                     printf("Error on line %d: label name \"%s\" is not unique.\n", token->lineNumber, token->stringValue);
//                     exit(ExitCodeLabelNameNotUnique);
//                 }
//             }
//             break;
//         case TokenTypeLabelUseOrInstruction:
//             enum Instruction instruction = getInstruction(token->stringValue);
//             if (instruction == InstructionInvalid) {
//                 printf("Error on line %d: invalid instruction \"%s\".\n", token->lineNumber, token->stringValue);
//                 exit(ExitCodeInvalidInstruction);
//             } else {
//                 assertNoMemoryViolation(state, token, state->address);
//                 assertNoMemoryViolation(state, token, state->address + 1);
//                 unsigned short opcode = (char)instruction << 13;
//                 result->dataType[state->address] = DataTypeInstruction;
//                 state->programMemoryWritten[state->address] = true;
//                 state->programMemoryWritten[state->address + 1] = true;
//                 getToken(token, filePtr);
//                 switch (token->type) {
//                     case TokenTypeDecimalNumber:
//                     case TokenTypeHexNumber:
//                     case TokenTypeOctalNumber:
//                     case TokenTypeBinaryNumber:
//                         if (token->numberValue < 0 || token->numberValue >= ADDRESS_SPACE_SIZE) {
//                             printf("Error on line %d: attempting to reference invalid address 0x%04X.\n", token->lineNumber, token->numberValue);
//                             exit(ExitCodeReferenceToInvalidAddress);
//                         }
//                         opcode |= token->numberValue;
//                         break;
//                     case TokenTypeLabelUseOrInstruction:
//                         if (state->labelUsesIndex >= MAX_LABELS) {
//                             printf("Error on line %d: too many labels.\n", token->lineNumber);
//                             exit(ExitCodeTooManyLabels);
//                         }
//                         state->labelUses[state->labelUsesIndex].name = token->stringValue;
//                         state->labelUses[state->labelUsesIndex].lineNumber = token->lineNumber;
//                         state->labelUses[state->labelUsesIndex++].address = state->address;
//                         break;
//                     default:
//                         printf("Error on line %d: unexpected token following instruction name.\n", token->lineNumber);
//                         exit(ExitCodeUnexpectedTokenAfterInstruction);
//                 }
//                 result->programMemory[state->address] = opcode;
//                 result->programMemory[state->address + 1] = opcode >> 8;
//                 state->address += 2;
//             }
//             state->lastTokenWasLabelDefinition = false;
//             break;
//         case TokenTypeDirective:
//             enum Directive directive = getDirective(token->stringValue);
//             switch (directive) {
//                 case DirectiveOrg:
//                     getToken(token, filePtr);
//                     switch (token->type) {
//                         case TokenTypeDecimalNumber:
//                         case TokenTypeHexNumber:
//                         case TokenTypeOctalNumber:
//                         case TokenTypeBinaryNumber:
//                             if (token->numberValue < 0 || token->numberValue >= ADDRESS_SPACE_SIZE) {
//                                 printf("Error on line %d: attempting to set origin to invalid address 0x%04X.\n", token->lineNumber, token->numberValue);
//                                 exit(ExitCodeOriginOutOfMemoryRange);
//                             }
//                             state->address = token->numberValue;
//                             if (state->lastTokenWasLabelDefinition) {
//                                 state->labelDefinitions[state->labelDefinitionsIndex - 1].address = state->address;
//                             }
//                             break;
//                         default:
//                             printf("Error on line %d: unexpected token following .ORG.\n", token->lineNumber);
//                             exit(ExitCodeUnexpectedTokenAfterOrg);
//                     }
//                     break;
//                 case DirectiveAlign:
//                     getToken(token, filePtr);
//                     switch (token->type) {
//                         case TokenTypeDecimalNumber:
//                         case TokenTypeHexNumber:
//                         case TokenTypeOctalNumber:
//                         case TokenTypeBinaryNumber:
//                             if (token->numberValue < 1 || token->numberValue > 12) {
//                                 printf("Error on line %d: invalid align parameter \"%d\". Must be between 1 and 12.\n", token->lineNumber, token->numberValue);
//                                 exit(ExitCodeInvalidAlignParameter);
//                             }
//                             unsigned short bitsToReset = (1 << token->numberValue) - 1;
//                             state->address = (state->address & bitsToReset) == 0
//                                 ? state->address
//                                 : ((state->address & ~bitsToReset) + bitsToReset + 1);
//                             if (state->address >= ADDRESS_SPACE_SIZE) {
//                                 printf("Error on line %d: attempting to set origin via align to invalid address 0x%04X.\n", token->lineNumber, state->address);
//                                 exit(ExitCodeOriginOutOfMemoryRange);
//                             }
//                             if (state->lastTokenWasLabelDefinition) {
//                                 state->labelDefinitions[state->labelDefinitionsIndex - 1].address = state->address;
//                             }
//                             break;
//                         default:
//                             printf("Error on line %d: unexpected token following .ALIGN.\n", token->lineNumber);
//                             exit(ExitCodeUnexpectedTokenAfterOrg);
//                     }
//                     break;
//                 case DirectiveFill:
//                     unsigned char valueToFill;
//                     unsigned char fillCount;
//                     enum DataType valueToFillType;
//                     getToken(token, filePtr);
//                     switch (token->type) {
//                         case TokenTypeDecimalNumber:
//                         case TokenTypeHexNumber:
//                         case TokenTypeOctalNumber:
//                         case TokenTypeBinaryNumber:
//                             assertNumberLiteralInRange(token);
//                             valueToFill = token->numberValue;
//                             valueToFillType = DataTypeInt;
//                             break;
//                         case TokenTypeNZTString:
//                             if (token->stringValue[0] == 0 || token->stringValue[1] != 0) {
//                                 printf("Error on line %d: expected no more or less than one character.\n", token->lineNumber);
//                                 exit(ExitCodeFillValueStringNotAChar);
//                             }
//                             valueToFill = token->stringValue[0];
//                             valueToFillType = DataTypeChar;
//                             break;
//                         default:
//                             printf("Error on line %d: unexpected token following .FILL.\n", token->lineNumber);
//                             exit(ExitCodeUnexpectedTokenAfterFill);
//                     }
//                     getToken(token, filePtr);
//                     switch (token->type) {
//                         case TokenTypeDecimalNumber:
//                         case TokenTypeHexNumber:
//                         case TokenTypeOctalNumber:
//                         case TokenTypeBinaryNumber:
//                             if (token->numberValue < 1) {
//                                 printf("Error on line %d: fill count must be positive.\n", token->lineNumber);
//                                 exit(ExitCodeFillCountNotPositive);
//                             }
//                             fillCount = token->numberValue;
//                             break;
//                         default:
//                             printf("Error on line %d: unexpected token following .FILL value.\n", token->lineNumber);
//                             exit(ExitCodeUnexpectedTokenAfterFill);
//                     }
//                     for (int i = 0; i < fillCount; ++i) {
//                         assertNoMemoryViolation(state, token, state->address);
//                         result->programMemory[state->address] = valueToFill;
//                         result->dataType[state->address] = valueToFillType;
//                         state->programMemoryWritten[state->address++] = true;
//                     }
//                     break;
//                 default:
//                     printf("Error on line %d: invalid directive \"%s\".\n", token->lineNumber, token->stringValue);
//                     exit(ExitCodeInvalidDirective);
//             }
//             state->lastTokenWasLabelDefinition = false;
//             break;
//         case TokenTypeDecimalNumber:
//         case TokenTypeHexNumber:
//         case TokenTypeOctalNumber:
//         case TokenTypeBinaryNumber:
//             assertNoMemoryViolation(state, token, state->address);
//             assertNumberLiteralInRange(token);
//             result->programMemory[state->address] = token->numberValue;
//             result->dataType[state->address] = DataTypeInt;
//             state->programMemoryWritten[state->address++] = true;
//             state->lastTokenWasLabelDefinition = false;
//             break;
//         case TokenTypeZTString:
//         case TokenTypeNZTString:
//             assertNoMemoryViolation(state, token, state->address);
//             result->dataType[state->address] = DataTypeChar;
//             char* strptr = token->stringValue;
//             while(*strptr != 0) {
//                 assertNoMemoryViolation(state, token, state->address);
//                 result->programMemory[state->address] = *strptr;
//                 result->dataType[state->address] = DataTypeChar;
//                 state->programMemoryWritten[state->address++] = true;
//                 ++strptr;
//             }
//             if (token->type == TokenTypeZTString) {
//                 assertNoMemoryViolation(state, token, state->address);
//                 result->programMemory[state->address] = 0;
//                 result->dataType[state->address] = DataTypeChar;
//                 state->programMemoryWritten[state->address++] = true;
//             }
//             state->lastTokenWasLabelDefinition = false;
//             break;
//         default:
//             printf("Error on line %d: invalid token \"%s\".\n", token->lineNumber, token->stringValue);
//             exit(ExitCodeInvalidToken);
//     }
// }

// struct LabelInfo* findLabelDefinition(struct AssemblerState* state, struct LabelInfo* labelUse) {
//     for (int i = 0; i < state->labelDefinitionsIndex; ++i) {
//         if (strcmp(state->labelDefinitions[i].name, labelUse->name) == 0) {
//             return &state->labelDefinitions[i];
//         }
//     }

//     printf("Error on line %d: label \"%s\" is undefined.\n", labelUse->lineNumber, labelUse->name);
//     exit(ExitCodeUndefinedLabel);
// }

static char* getFileContents(FILE* filePtr) {
    fseek(filePtr, 0, SEEK_END);
    int fileSize = ftell(filePtr);
    rewind(filePtr);
    char* fileContents = malloc(fileSize + 1);
    char ch;
    int i = 0;
    while ((ch = getc(filePtr)) != EOF) {
        fileContents[i++] = ch;
    }
    fileContents[i] = 0;
    return fileContents;
}

struct AssemblerResult assemble(FILE* filePtr) {
    char* fileContents = getFileContents(filePtr);

    int lineNumber = 1;
    struct Token token;
    while ((token = getToken(&fileContents, &lineNumber)).value != NULL) {
        printf("%d: %s (%d)\n", token.lineNumber, token.value, token.length);
    }

}