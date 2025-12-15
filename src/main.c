/*
    W16ASM Copyright (C) 2025 Piotr Marczyński <piotrmski@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

    See file COPYING.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "program-input/program-input.h"
#include "assembler/assembler.h"
#include "../common/exit-code.h"

#define IO_INTERFACE_ADDRESS 0x1fff

int main(int argc, const char * argv[]) {
    struct ProgramInput input = getProgramInput(argc, argv);

    FILE* asmFile = fopen(input.asmFilePath, "rb");

    if (asmFile == NULL) {
        printf("Error: could not read file \"%s\".\n", input.asmFilePath);
        exit(ExitCodeCouldNotReadAsmFile);
    }

    struct AssemblerResult result = assemble(asmFile);

    fclose(asmFile);

    int programSize = 0;

    for (int i = 0; i < ADDRESS_SPACE_SIZE; ++i) {
        if (result.dataType[i] != DataTypeNone) {
            programSize = i + (result.dataType[i] == DataTypeInstruction ? 2 : 1);
        }
    }

    if (programSize == 0) {
        printf("Error: the resulting program is empty.\n");
        exit(ExitCodeResultProgramEmpty);
    }

    FILE* binFile = fopen(input.binaryFilePath, "wb");

    if (binFile == NULL) {
        printf("Error: could not write to file \"%s\".\n", input.binaryFilePath);
        exit(ExitCodeCouldNotWriteBinFile);
    }

    fwrite(result.programMemory, sizeof(unsigned char), programSize, binFile);

    fclose(binFile);

    FILE* symbolsFile = input.symbolsFilePath == NULL ? NULL : fopen(input.symbolsFilePath, "w");

    if (input.symbolsFilePath != NULL) {
        if (symbolsFile == NULL) {
            printf("Error: could not write to file \"%s\".\n", input.symbolsFilePath);
            exit(ExitCodeCouldNotWriteSymbolsFile);
        }

        for (int i = 0; i < ADDRESS_SPACE_SIZE; ++i) {
        if (result.dataType[i] != DataTypeNone || result.labelNameByAddress[i] != NULL) {
                fprintf(symbolsFile, "0x%04X,", i);
                switch (result.dataType[i]) {
                    case DataTypeInstruction:
                        fprintf(symbolsFile, "instruction");
                        break;
                    case DataTypeChar:
                        fprintf(symbolsFile, "char");
                        break;
                    default:
                        fprintf(symbolsFile, i == IO_INTERFACE_ADDRESS ? "char" : "int");
                        break;
                }
                fprintf(symbolsFile, ",%s\n", result.labelNameByAddress[i] == NULL ? "" : result.labelNameByAddress[i]);
            }
        }
        
        fclose(symbolsFile);
    }

    return ExitCodeSuccess;
}