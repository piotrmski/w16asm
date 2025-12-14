#include "program-input.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../common/exit-code.h"

struct ProgramInput getProgramInput(int argc, const char * argv[]) {
    const char* asmFilePath = NULL;
    const char* binaryFilePath = NULL;
    const char* symbolsFilePath = NULL;

    bool helpFlag = false;

    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') {
            if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
                if (helpFlag) {
                    printf("Error: help flag was used more than once.\n");
                    exit(ExitCodeProgramArgumentsInvalid);
                } else {
                    helpFlag = true;
                }
            } else {
                printf("Error: unknown flag \"%s\".\n", argv[i]);
                exit(ExitCodeProgramArgumentsInvalid);
            }
        } else {
            if (i == 1) {
                asmFilePath = argv[i];
            } else if (i == 2) {
                binaryFilePath = argv[i];
            } else if (i == 3) {
                symbolsFilePath = argv[i];
            } else {
                printf("Error: too many arguments.\n");
                exit(ExitCodeProgramArgumentsInvalid);
            }
        }
    }

    if (argc == 1 || helpFlag) {
        printf("W16 assembler. Copyright (C) 2025 Piotr Marczyński. This program is licensed under GNU GPL v3. See file COPYING.\n");
        printf("Usage:\n");
        printf("w16asm [path/to/assembly-source.asm] [path/to/binary-destination.bin] [path/to/symbols-destination.csv]\n");
        printf("Assembles the source file and saves the resulting binary file.\n");
        printf("Assembly source and binary destination paths are required.\n");
        printf("Symbols destination path is optional.\n");
        printf("Flags:\n");
        printf("-h or --help - prints this message.\n");
        exit(ExitCodeSuccess);
    } else if (binaryFilePath == NULL) {
        printf("Error: destination file path was not provided.\n");
        exit(ExitCodeProgramArgumentsInvalid);
    }

    return (struct ProgramInput) { asmFilePath, binaryFilePath, symbolsFilePath };
}