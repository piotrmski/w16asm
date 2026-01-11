#ifndef shared
#define shared

#include <stdbool.h>

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

char charUppercase(char ch);

bool stringsEqualCaseInsensitive(char* string1, char* string2);

const char* getInstructionName(enum Instruction instruction);

#endif