#include "shared.h"

char charUppercase(char ch) {
    if (ch >= 'a' && ch <= 'z') return ch - 0x20;
    else return ch;
}

bool stringsEqualCaseInsensitive(char* string1, char* string2) {
    for (int i = 0;; ++i) {
        if (charUppercase(string1[i]) != charUppercase(string2[i])) {
            return false;
        }

        if (string1[i] == 0 || string2[i] == 0) {
            return true;
        }
    }
}

const char* getInstructionName(enum Instruction instruction) {
    switch (instruction) {
        case InstructionLd: return "LD"; 
        case InstructionNot: return "NOT"; 
        case InstructionAdd: return "ADD"; 
        case InstructionAnd: return "AND"; 
        case InstructionSt: return "ST"; 
        case InstructionJmp: return "JMP"; 
        case InstructionJmn: return "JMN"; 
        case InstructionJmz: return "JMZ"; 
        case InstructionInvalid: return "";
    }
}