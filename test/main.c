#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../common/exit-code.h"

struct TestResults {
    int passed;
    int failed;
};

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

    FILE* expectedFile = fopen(expectedFilePath, "rb");

    if (expectedFile == NULL) { 
        printf("[FAIL] %s - reference file \"%s\" is missing, the test can't be evaluated.\n", testName, expectedFilePath);
        return false;
    }

    FILE* actualFile = fopen(actualFilePath, "rb");

    if (actualFile == NULL) { 
        fclose(expectedFile);
        printf("[FAIL] %s - file \"%s\" was not produced.\n", testName, actualFilePath);
        return false; 
    }

    int expectedFileSize;
    int actualFileSize;
          
    fseek(expectedFile, 0, SEEK_END);          
    expectedFileSize = ftell(expectedFile);            
    rewind(expectedFile);           
          
    fseek(actualFile, 0, SEEK_END);          
    actualFileSize = ftell(actualFile);            
    rewind(actualFile);   
    
    if (expectedFileSize != actualFileSize) {
        printf("[FAIL] %s - file \"%s\" was expected to be %d bytes, is %d bytes.\n", testName, actualFilePath, expectedFileSize, actualFileSize);
        return false;
    }

    unsigned char expectedFileByte;
    unsigned char producedFileByte;

    for (int i = 0; i < expectedFileSize; ++i) {
        fread(&expectedFileByte, sizeof(unsigned char), 1, expectedFile);
        fread(&producedFileByte, sizeof(unsigned char), 1, actualFile);

        if (expectedFileByte != producedFileByte) {
            switch (fileType) {
                case FileTypeBinary:
                    printf("[FAIL] %s - file \"%s\" at byte %d (0x%04X): expected 0x%02X, is 0x%02X.\n", testName, actualFilePath, i, i, expectedFileByte, producedFileByte);
                    break;
                case FileTypeText:
                    printf("[FAIL] %s - file \"%s\" at byte %d (0x%04X): expected '%c' (0x%02X), is '%c' (0x%02X).\n", testName, actualFilePath, i, i, expectedFileByte, expectedFileByte, producedFileByte, producedFileByte);
                    break;
            }
            return false;
        }
    }

    return true;
}

static int executeTestCase(char* testName) {
    char syscall[4096];
    sprintf(syscall, "./dist/w16asm test/test-cases/%s/test.asm test/test-cases/%s/actual.bin test/test-cases/%s/actual.csv", testName, testName, testName);
    int status = system(syscall);
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

static void expectErrorCode(char* testName, int expectedErrorCode, struct TestResults* testResults) {
    char actualBinPath[1024];
    char actualCsvPath[1024];
    sprintf(actualBinPath, "test/test-cases/%s/actual.bin", testName);
    sprintf(actualCsvPath, "test/test-cases/%s/actual.csv", testName);
    remove(actualBinPath);
    remove(actualCsvPath);

    int returnCode = executeTestCase(testName);

    if (returnCode != expectedErrorCode) {
        ++testResults->failed;
        printf("[FAIL] %s - error code %d was expected, but code %d was produced.\n", testName, expectedErrorCode, returnCode);
        return;
    }

    if (fileExists(actualBinPath) || fileExists(actualCsvPath)) {
        ++testResults->failed;
        printf("[FAIL] %s - the expected error code was produced, however output files were produced as well, when none were expected.\n", testName);
        return;
    }

    ++testResults->passed;
    printf("[PASS] %s\n", testName);
}

static void expectSuccess(char* testName, struct TestResults* testResults) {
    int returnCode = executeTestCase(testName);

    if (returnCode != 0) {
        ++testResults->failed;
        printf("[FAIL] %s - error code 0 was expected, but code %d was produced.\n", testName, returnCode);
        return;
    }

    if (!filesIdentical(testName, "bin", FileTypeBinary)) {
        ++testResults->failed;
        return;
    }

    if (!filesIdentical(testName, "csv", FileTypeText)) {
        ++testResults->failed;
        return;
    }

    ++testResults->passed;
    printf("[PASS] %s\n", testName);
}

int main(int argc, const char * argv[]) {
    struct TestResults testResults = { 0, 0 };

    expectErrorCode("assemble-empty-program", ExitCodeResultProgramEmpty, &testResults);
    expectSuccess("validate-basic-functionality", &testResults);

    printf("Tests passed: %d\nTests failed: %d\n", testResults.passed, testResults.failed);
}