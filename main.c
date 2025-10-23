#include <stdio.h>
#include <string.h>
#include "program-input/program-input.h"
#include "assembler/assembler.h"

int main(int argc, const char * argv[]) {
    struct ProgramInput input = getProgramInput(argc, argv);

    if (input.error) { return 1; }

    if (input.asmFilePath == NULL || input.binaryFilePath == NULL) { return 0; }

    FILE* asmFile = fopen(input.asmFilePath, "r");

    if (asmFile == NULL) {
        printf("Error: could not read file \"%s\".\n", input.asmFilePath);
        return 1;
    }

    unsigned short* programMemory = assemble(asmFile);

    fclose(asmFile);

    if (programMemory == NULL) {
        return 1;
    }

    FILE* binFile = fopen(input.binaryFilePath, "w");

    if (binFile == NULL) {
        printf("Error: could not write to file \"%s\".\n", input.binaryFilePath);
        return 1;
    }

    int programSize = 0;

    for (int i = 0; i < PROGRAM_MEMORY_SIZE; ++i) {
        if (programMemory[i] != 0) {
            programSize = i + 1;
        }
    }

    if (programSize == 0) {
        printf("Error: the resulting program is empty.\n");
        return 1;
    }

    fwrite(programMemory, sizeof(unsigned short), programSize, binFile);

    fclose(binFile);

    // TODO create the symbols file

    return 0;
}