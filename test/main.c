/*
    W16ASM-test Copyright (C) 2025 Piotr Marczyński <piotrmski@gmail.com>

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
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../common/exit-code.h"

#define FAIL "\x1B[31m[FAIL]\x1B[0m"
#define WARN "\x1B[33m[WARN]\x1B[0m"
#define PASS "\x1B[32m[PASS]\x1B[0m"

struct TestResults {
    int passed;
    int warned;
    int failed;
} testResults;

enum FileType {
    FileTypeBinary,
    FileTypeText
};

static bool fileExists(char* path) {
    FILE* file = fopen(path, "r");
    if (file == NULL) {
        return false;
    } else {
        fclose(file);
        return true;
    }
}

static bool filesIdentical(char* testName, char* fileExtension, enum FileType fileType) {
    char expectedFilePath[1024];
    char actualFilePath[1024];

    sprintf(expectedFilePath, "test/test-cases/%s/expected.%s", testName, fileExtension);
    sprintf(actualFilePath, "test/test-cases/%s/actual.%s", testName, fileExtension);

    FILE* expectedFile = fopen(expectedFilePath, fileType == FileTypeBinary ? "rb" : "r");

    if (expectedFile == NULL) { 
        printf(FAIL " %s - reference file \"%s\" is missing, the test can't be evaluated.\n", testName, expectedFilePath);
        return false;
    }

    FILE* actualFile = fopen(actualFilePath, fileType == FileTypeBinary ? "rb" : "r");

    if (actualFile == NULL) { 
        fclose(expectedFile);
        printf(FAIL " %s - file \"%s\" was not produced.\n", testName, actualFilePath);
        return false; 
    }

    unsigned char expectedFileByte;
    unsigned char actualFileByte;

    int line = 1;
    int col = 1;

    for (int i = 0;; ++i) {
        bool expectedFileEof = !fread(&expectedFileByte, sizeof(unsigned char), 1, expectedFile);
        bool actualFileEof = !fread(&actualFileByte, sizeof(unsigned char), 1, actualFile);

        if (actualFileEof && expectedFileEof) {
            fclose(expectedFile);
            fclose(actualFile);
            return true;
        }

        if (expectedFileEof) {
            fseek(actualFile, 0, SEEK_END);          
            int actualFileSize = ftell(actualFile);
            printf(FAIL " %s - file \"%s\" was expected to be %d bytes, is %d bytes.\n", testName, actualFilePath, i, actualFileSize);
            fclose(expectedFile);
            fclose(actualFile);
            return false;
        }

        if (actualFileEof) {
            fseek(expectedFile, 0, SEEK_END);          
            int expectedFileSize = ftell(expectedFile);
            printf(FAIL " %s - file \"%s\" was expected to be %d bytes, is %d bytes.\n", testName, actualFilePath, expectedFileSize, i);
            fclose(expectedFile);
            fclose(actualFile);
            return false;
        }

        if (expectedFileByte != actualFileByte) {
            switch (fileType) {
                case FileTypeBinary:
                    printf(FAIL " %s - file \"%s\" at byte %d (0x%04X): expected 0x%02X, is 0x%02X.\n", testName, actualFilePath, i, i, expectedFileByte, actualFileByte);
                    break;
                case FileTypeText:
                    printf(FAIL " %s - file \"%s\" at line %d column %d: expected '%c' (0x%02X), is '%c' (0x%02X).\n", testName, actualFilePath, line, col, expectedFileByte, expectedFileByte, actualFileByte, actualFileByte);
                    break;
            }
            fclose(expectedFile);
            fclose(actualFile);
            return false;
        }

        if (actualFileByte != '\n') {
            ++col;
        } else {
            col = 1;
            ++line;
        }
    }
}

static int executeTestCase(char* testName) {
    char syscall[4096];
    sprintf(syscall, "./dist/w16asm test/test-cases/%s/test.asm test/test-cases/%s/actual.bin test/test-cases/%s/actual.csv", testName, testName, testName);
    int status = system(syscall);
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

static void expectErrorCode(char* testName, int expectedErrorCode) {
    char actualBinPath[1024];
    char actualCsvPath[1024];
    sprintf(actualBinPath, "test/test-cases/%s/actual.bin", testName);
    sprintf(actualCsvPath, "test/test-cases/%s/actual.csv", testName);
    remove(actualBinPath);
    remove(actualCsvPath);

    int returnCode = executeTestCase(testName);

    if (returnCode != expectedErrorCode) {
        if (returnCode != 0) {
            ++testResults.warned;
            printf(WARN " %s - code %d was expected, but code %d was produced.\n", testName, expectedErrorCode, returnCode);
        } else {
            ++testResults.failed;
            printf(FAIL " %s - code %d was expected, but success code was produced.\n", testName, expectedErrorCode);
        }
        return;
    }

    if (fileExists(actualBinPath) || fileExists(actualCsvPath)) {
        ++testResults.failed;
        printf(FAIL " %s - the expected code was produced, however output files were produced as well, when none were expected.\n", testName);
        return;
    }

    ++testResults.passed;
    printf(PASS " %s\n", testName);
}

static void expectSuccess(char* testName) {
    int returnCode = executeTestCase(testName);

    if (returnCode != 0) {
        ++testResults.failed;
        printf(FAIL " %s - success code was expected, but code %d was produced.\n", testName, returnCode);
        return;
    }

    if (!filesIdentical(testName, "bin", FileTypeBinary)) {
        ++testResults.failed;
        return;
    }

    if (!filesIdentical(testName, "csv", FileTypeText)) {
        ++testResults.failed;
        return;
    }

    ++testResults.passed;
    printf(PASS " %s\n", testName);
}

int main(int argc, const char * argv[]) {
    expectErrorCode("empty-program-should-fail", ExitCodeResultProgramEmpty);
    expectSuccess("validate-basic-functionality");
    expectSuccess("label-name-should-allow-valid-length-and-characters");
    expectErrorCode("label-name-should-disallow-invalid-length", ExitCodeLabelNameTooLong);
    expectErrorCode("label-name-should-disallow-invalid-character", ExitCodeUnexpectedCharacter);
    expectSuccess("declaration-should-allow-near-memory-range");
    expectErrorCode("declaration-should-disallow-beyond-memory-range", ExitCodeDeclaringValueOutOfMemoryRange);
    expectSuccess("instruction-should-allow-near-memory-range");
    expectErrorCode("instruction-should-disallow-beyond-memory-range", ExitCodeDeclaringValueOutOfMemoryRange);
    expectErrorCode("memory-overwrite-should-fail", ExitCodeMemoryValueOverridden);
    expectErrorCode("number-literals-binary-should-disallow-no-digits", ExitCodeNumberWithoutDigits);
    expectErrorCode("number-literals-hex-should-disallow-no-digits", ExitCodeNumberWithoutDigits);
    expectSuccess("number-literals-should-allow-in-range");
    expectErrorCode("number-literals-should-disallow-too-high", ExitCodeNumberLiteralOutOutRange);
    expectErrorCode("number-literals-should-disallow-too-low", ExitCodeNumberLiteralOutOutRange);
    expectErrorCode("origin-should-disallow-address-beyond-memory-range", ExitCodeOriginOutOfMemoryRange);
    expectErrorCode("origin-should-disallow-negative-address", ExitCodeOriginOutOfMemoryRange);
    expectErrorCode("reference-should-disallow-address-beyond-memory-range", ExitCodeReferenceToInvalidAddress);
    expectErrorCode("reference-should-disallow-negative-address", ExitCodeReferenceToInvalidAddress);
    expectErrorCode("string-should-disallow-unterminated", ExitCodeUnterminatedString);
    expectErrorCode("fill-should-disallow-multiple-characters", ExitCodeFillValueStringNotAChar);
    expectErrorCode("fill-should-disallow-non-positive-count", ExitCodeFillCountNotPositive);
    expectSuccess("label-before-align-should-point-to-valid-address");
    expectSuccess("label-before-fill-should-point-to-valid-address");
    expectSuccess("label-before-org-should-point-to-valid-address");
    expectErrorCode("align-should-disallow-param-too-low", ExitCodeInvalidAlignParameter);
    expectErrorCode("align-should-disallow-param-too-high", ExitCodeInvalidAlignParameter);
    expectErrorCode("align-should-disallow-beyond-memory-range", ExitCodeOriginOutOfMemoryRange);
    expectSuccess("align-should-allow-param-lowest");
    expectSuccess("align-should-allow-param-highest");
    expectSuccess("empty-string-should-produce-char-0");
    expectSuccess("empty-nzt-string-should-be-labeled-as-char");
    expectSuccess("label-at-higher-byte-of-instruction-should-be-int");
    expectSuccess("align-should-not-change-address-if-already-aligned");

    printf("Tests passed: %d\nTests warned: %d\nTests failed: %d\n", testResults.passed, testResults.warned, testResults.failed);
}