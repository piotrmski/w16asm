#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "program-input/program-input.h"
#include "assembler/assembler.h"
#include "../common/exit-code.h"

#define TIME_BYTE0_ADDRESS 0x1ffb
#define TIME_BYTE1_ADDRESS 0x1ffc
#define TIME_BYTE2_ADDRESS 0x1ffd
#define TIME_BYTE3_ADDRESS 0x1ffe
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
            if (result.dataType[i] != DataTypeNone) {
                fprintf(symbolsFile, "0x%04X,", i);
                switch (result.dataType[i]) {
                    case DataTypeInstruction:
                        fprintf(symbolsFile, "instruction");
                        break;
                    case DataTypeChar:
                        fprintf(symbolsFile, "char");
                        break;
                    case DataTypeInt:
                        fprintf(symbolsFile, "int");
                        break;
                    default:
                        break;
                }
                fprintf(symbolsFile, ",%s\n", result.labelNameByAddress[i] == NULL ? "" : result.labelNameByAddress[i]);
            }
        }

        if (result.labelNameByAddress[TIME_BYTE0_ADDRESS] != NULL) {
            fprintf(symbolsFile, "0x%04X,int,%s\n", TIME_BYTE0_ADDRESS, result.labelNameByAddress[TIME_BYTE0_ADDRESS]);
        }
        
        if (result.labelNameByAddress[TIME_BYTE1_ADDRESS] != NULL) {
            fprintf(symbolsFile, "0x%04X,int,%s\n", TIME_BYTE1_ADDRESS, result.labelNameByAddress[TIME_BYTE1_ADDRESS]);
        }
        
        if (result.labelNameByAddress[TIME_BYTE2_ADDRESS] != NULL) {
            fprintf(symbolsFile, "0x%04X,int,%s\n", TIME_BYTE2_ADDRESS, result.labelNameByAddress[TIME_BYTE2_ADDRESS]);
        }
        
        if (result.labelNameByAddress[TIME_BYTE3_ADDRESS] != NULL) {
            fprintf(symbolsFile, "0x%04X,int,%s\n", TIME_BYTE3_ADDRESS, result.labelNameByAddress[TIME_BYTE3_ADDRESS]);
        }
        
        if (result.labelNameByAddress[IO_INTERFACE_ADDRESS] != NULL) {
            fprintf(symbolsFile, "0x%04X,char,%s\n", IO_INTERFACE_ADDRESS, result.labelNameByAddress[IO_INTERFACE_ADDRESS]);
        }
        
        fclose(symbolsFile);
    }

    return ExitCodeSuccess;
}