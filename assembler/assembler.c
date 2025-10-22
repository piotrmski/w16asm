#include "assembler.h"
#include "../tokenizer/tokenizer.h"
#include <stdbool.h>
#include <stdio.h>

#define ADDRESS_SPACE_SIZE 0x2000
#define PROGRAM_MEMORY_SIZE 0x1fff
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
    DirectiveFill,
    DirectiveInvalid
};

struct LabelInfo {
    char* name;
    int lineNumber;
    int address;
};

struct AssemblerState {
    int address;
    unsigned short programMemory[PROGRAM_MEMORY_SIZE];
    bool programMemoryWritten[PROGRAM_MEMORY_SIZE];
    bool lastTokenWasLabelDefinition;
    struct LabelInfo labelDefinitions[MAX_LABELS];
    int labelDefinitionsIndex;
    struct LabelInfo labelUses[MAX_LABELS];
    int labelUsesIndex;
};

static isMemoryViolation(struct Token* token, struct AssemblerState* state) {
    if (state->address >= PROGRAM_MEMORY_SIZE) {
        printf("Error on line %d: attempting to declare memory value outside of memory space.", token->lineNumber);
        return true;
    }

    if (state->programMemoryWritten[state->address]) {
        printf("Error on line %d: attempting to override memory value.", token->lineNumber);
        return true;
    }
}

static char uc(char ch) {
    if (ch >= 'a' && ch <= 'z') return ch - 0x20;
    else return ch;
}

static enum Instruction getInstruction(char* name) {
    if (uc(name[0]) == 'L' && uc(name[1]) == 'D' && name[2] == 0) {
        return InstructionLd;
    } else if (uc(name[0]) == 'N' && uc(name[1]) == 'O' && uc(name[2]) == 'T' && name[3] == 0) {
        return InstructionNot;
    } else if (uc(name[0]) == 'A' && uc(name[1]) == 'D' && uc(name[2]) == 'D' && name[3] == 0) {
        return InstructionAdd;
    } else if (uc(name[0]) == 'A' && uc(name[1]) == 'N' && uc(name[2]) == 'D' && name[3] == 0) {
        return InstructionAnd;
    } else if (c(name[0]) == 'S' && uc(name[1]) == 'T' && name[2] == 0) {
        return InstructionSt;
    } else if (uc(name[0]) == 'J' && uc(name[1]) == 'M' && uc(name[2]) == 'P' && name[3] == 0) {
        return InstructionJmp;
    } else if (uc(name[0]) == 'J' && uc(name[1]) == 'M' && uc(name[2]) == 'N' && name[3] == 0) {
        return InstructionJmn;
    } else if (uc(name[0]) == 'J' && uc(name[1]) == 'M' && uc(name[2]) == 'Z' && name[3] == 0) {
        return InstructionJmz;
    } else {
        return InstructionInvalid;
    }
}

static enum Directive getDirective(char* name) {
    if (uc(name[0]) == 'O' && uc(name[1]) == 'R' && uc(name[2]) == 'G' && name[3] == 0) {
        return DirectiveOrg;
    } else if (uc(name[0]) == 'F' && uc(name[1]) == 'I' && uc(name[2]) == 'L' && uc(name[3]) == 'L' && name[4] == 0) {
        return DirectiveFill;
    } else {
        return DirectiveInvalid;
    }
}

// Returns true if an error occurs
static bool parseToken(struct Token* token, struct AssemblerState* state, FILE* filePtr) {
    // TODO always validate memory write in range and not overwritten
    switch (token->type) {
        case TokenTypeNone:
            return false;
        case TokenTypeLabelDefinition:
            if (state->labelDefinitionsIndex >= MAX_LABELS) {
                printf("Error on line %d: too many labels.", token->lineNumber);
                return true;
            }
            state->labelDefinitions[state->labelDefinitionsIndex].name = token->stringValue;
            state->labelDefinitions[state->labelDefinitionsIndex].lineNumber = token->lineNumber;
            state->labelDefinitions[state->labelDefinitionsIndex++].address = state->address;
            state->lastTokenWasLabelDefinition = true;
            break;
        case TokenTypeLabelUseOrInstruction:
            enum Instruction instruction = getInstruction(token->stringValue);
            if (instruction == InstructionInvalid) {
                printf("Error on line %d: invalid instruction \"%s\".", token->lineNumber, token->stringValue);
                return true;
            } else {
                if (isMemoryViolation(state, token)) { return true; }
                state->programMemory[state->address] = (char)instruction << 13;
                state->programMemoryWritten[state->address] = true;
                getToken(&token, filePtr);
                switch (token->type) {
                    case TokenTypeDecimalNumber:
                    case TokenTypeHexNumber:
                    case TokenTypeOctalNumber:
                    case TokenTypeBinaryNumber:
                        if (token->numberValue < 0 || token->numberValue >= ADDRESS_SPACE_SIZE) {
                            printf("Error on line %d: attempting to reference invalid address \"%d\".", token->lineNumber, token->numberValue);
                            return true;
                        }
                        state->programMemory[state->address] |= token->numberValue;
                        break;
                    case TokenTypeLabelUseOrInstruction:
                        if (state->labelUsesIndex >= MAX_LABELS) {
                            printf("Error on line %d: too many labels.", token->lineNumber);
                            return true;
                        }
                        state->labelUses[state->labelUsesIndex].name = token->stringValue;
                        state->labelUses[state->labelUsesIndex].lineNumber = token->lineNumber;
                        state->labelUses[state->labelUsesIndex++].address = state->address;
                        break;
                    default:
                        printf("Error on line %d: unexpected token following instruction name.", token->lineNumber);
                        return true;
                }
                ++state->address;
            }
            state->lastTokenWasLabelDefinition = false;
            break;
        case TokenTypeDirective:
            enum Directive directive = getDirective(token->stringValue);
            switch (directive) {
                case DirectiveOrg:
                    getToken(&token, filePtr);
                    switch (token->type) {
                        case TokenTypeDecimalNumber:
                        case TokenTypeHexNumber:
                        case TokenTypeOctalNumber:
                        case TokenTypeBinaryNumber:
                            if (token->numberValue < 0 || token->numberValue >= ADDRESS_SPACE_SIZE) {
                                printf("Error on line %d: attempting to set origin to invalid address \"%d\".", token->lineNumber, token->numberValue);
                                return true;
                            }
                            state->address = token->numberValue;
                            if (state->lastTokenWasLabelDefinition) {
                                state->labelDefinitions[state->labelDefinitionsIndex - 1].address = state->address;
                            }
                            break;
                        default:
                            printf("Error on line %d: unexpected token following .ORG.", token->lineNumber);
                            return true;
                    }
                    break;
                case DirectiveFill:
                    unsigned short valueToFill;
                    unsigned short fillCount;
                    getToken(&token, filePtr);
                    switch (token->type) {
                        case TokenTypeDecimalNumber:
                        case TokenTypeHexNumber:
                        case TokenTypeOctalNumber:
                        case TokenTypeBinaryNumber:
                            valueToFill = token->numberValue;
                            break;
                        case TokenTypeNZTString:
                            if (token->stringValue[0] == 0 || token->stringValue[1] != 0) {
                                printf("Error on line %d: expected no more or less than one character.", token->lineNumber);
                                return true;
                            }
                            valueToFill = token->stringValue[0];
                            break;
                        default:
                            printf("Error on line %d: unexpected token following .FILL.", token->lineNumber);
                            return true;
                    }
                    getToken(&token, filePtr);
                    switch (token->type) {
                        case TokenTypeDecimalNumber:
                        case TokenTypeHexNumber:
                        case TokenTypeOctalNumber:
                        case TokenTypeBinaryNumber:
                            if (token->numberValue < 1) {
                                printf("Error on line %d: fill count must be positive.", token->lineNumber);
                                return true;
                            }
                            fillCount = token->numberValue;
                            break;
                        default:
                            printf("Error on line %d: unexpected token following .FILL value.", token->lineNumber);
                            return true;
                    }
                    for (int i = 0; i < fillCount; ++i) {
                        if (isMemoryViolation(state, token)) { return true; }
                        state->programMemory[state->address] = valueToFill;
                        state->programMemoryWritten[state->address++] = true;
                    }
                    break;
                default:
                    printf("Error on line %d: invalid directive \"%s\".", token->lineNumber, token->stringValue);
                    return true;
            }
            state->lastTokenWasLabelDefinition = false;
            break;
        case TokenTypeDecimalNumber:
        case TokenTypeHexNumber:
        case TokenTypeOctalNumber:
        case TokenTypeBinaryNumber:
            if (isMemoryViolation(state, token)) { return true; }
            state->programMemory[state->address] = token->numberValue;
            state->programMemoryWritten[state->address++] = true;
            state->lastTokenWasLabelDefinition = false;
            break;
        case TokenTypeZTString:
        case TokenTypeNZTString:
            char* strptr = token->stringValue;
            while(&strptr != 0) {
                if (isMemoryViolation(state, token)) { return true; }
                state->programMemory[state->address] = &strptr;
                state->programMemoryWritten[state->address++] = true;
                ++strptr;
            }
            if (token->type == TokenTypeZTString) {
                if (isMemoryViolation(state, token)) { return true; }
                state->programMemory[state->address] = 0;
                state->programMemoryWritten[state->address++] = true;
            }
            state->lastTokenWasLabelDefinition = false;
            break;
        default:
            printf("Error on line %d: invalid token \"%s\".", token->lineNumber, token->stringValue);
            return true;
    }

    return false;
}

void assemble(FILE* filePtr) {
    struct Token token = getInitialTokenState();

    struct AssemblerState state = { 0, { 0 }, { false }, false, { { NULL, 0 } }, 0, { { NULL, 0 } }, 0 };

    do {
        getToken(&token, filePtr);
        if (parseToken(&token, &state, filePtr)) { return; }
    } while (token.type != TokenTypeNone && token.type != TokenTypeError);

    if (state.lastTokenWasLabelDefinition) {
        printf("Error on line %d: unexpected label definition at the end of the file.", token.lineNumber);
        return true;
    }

    for (int labelUsesIndex = 0; labelUsesIndex < state.labelUsesIndex; ++labelUsesIndex) {
        struct LabelInfo* labelDefinition = findLabelDefinition(&state, state.labelUses[labelUsesIndex].name);
        if (labelDefinition == NULL) {
            printf("Error on line %d: label \"%s\" is undefined.", state.labelUses[labelUsesIndex].lineNumber, state.labelUses[labelUsesIndex].name);
        }
        state.programMemory[state.labelUses[labelUsesIndex].address] |= labelDefinition->address;
    }
}