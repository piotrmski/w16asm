#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

struct TestResults {
    int passed;
    int failed;
};

bool fileExists(char* path) {
    FILE* file = fopen(path, "r");
    if (file == NULL) {
        return false;
    } else {
        fclose(file);
        return true;
    }
}

bool filesIdentical(char* path1, char* path2) {
    FILE* file1 = fopen(path1, "rb");

    if (file1 == NULL) { 
        return false;
    }

    FILE* file2 = fopen(path2, "rb");

    if (file2 == NULL) { 
        fclose(file1);
        return false; 
    }

    int size1;
    int size2;
          
    fseek(file1, 0, SEEK_END);          
    size1 = ftell(file1);            
    rewind(file1);           
          
    fseek(file2, 0, SEEK_END);          
    size2 = ftell(file2);            
    rewind(file2);   
    
    if (size1 != size2) {
        return false;
    }

    unsigned char byte1;
    unsigned char byte2;

    for (int i = 0; i < size1; ++i) {
        fread(&byte1, sizeof(unsigned char), 1, file1);
        fread(&byte2, sizeof(unsigned char), 1, file2);

        if (byte1 != byte2) {
            return false;
        }
    }

    return true;
}

int executeTestCase(char* testName) {
    char syscall[2047];
    sprintf(syscall, "./dist/w16asm test/test-cases/%s/test.asm test/test-cases/%s/actual.bin test/test-cases/%s/actual.csv", testName, testName, testName);
    return system(syscall);
}

void expectErrorCode(const char* testName, int expectedErrorCode, struct TestResults* testResults) {
    char actualBinPath[1023];
    char actualCsvPath[1023];
    sprintf(actualBinPath, "test/test-cases/%s/actual.bin", testName);
    sprintf(actualCsvPath, "test/test-cases/%s/actual.csv", testName);
    remove(actualBinPath);
    remove(actualCsvPath);

    int returnCode = executeTestCase(testName);

    if (returnCode != expectedErrorCode) {
        ++testResults->failed;
        printf("[FAIL] %s - error code %d was expected, but code %d was produced.", testName, expectedErrorCode, returnCode);
    }

    if (fileExists(actualBinPath) || fileExists(actualCsvPath)) {
        ++testResults->failed;
        printf("[FAIL] %s - the expected error code was produced, however output files were produced as well, when none were expected.", testName);
    }

    ++testResults->passed;
    printf("[PASS] %s", testName);
}

void expectSuccess(const char* testName, struct TestResults* testResults) {
    char expectedBinPath[1023];
    char expectedCsvPath[1023];
    char actualBinPath[1023];
    char actualCsvPath[1023];
    sprintf(expectedBinPath, "test/test-cases/%s/expected.bin", testName);
    sprintf(expectedCsvPath, "test/test-cases/%s/expected.csv", testName);
    sprintf(actualBinPath, "test/test-cases/%s/actual.bin", testName);
    sprintf(actualCsvPath, "test/test-cases/%s/actual.csv", testName);
    remove(actualBinPath);
    remove(actualCsvPath);

    int returnCode = executeTestCase(testName);

    if (returnCode != 0) {
        ++testResults->failed;
        printf("[FAIL] %s - error code 0 was expected, but code %d was produced.", testName, returnCode);
    }

    if (!filesIdentical(expectedBinPath, actualBinPath)) {
        ++testResults->failed;
        printf("[FAIL] %s - produced binary file was different, than expected.", testName, returnCode);
    }

    if (!filesIdentical(expectedCsvPath, actualCsvPath)) {
        ++testResults->failed;
        printf("[FAIL] %s - produced symbols file was different, than expected.", testName, returnCode);
    }

    ++testResults->passed;
    printf("[PASS] %s", testName);
}

int main(int argc, const char * argv[]) {
    struct TestResults testResults = { 0, 0 };

    // TODO

    printf("Tests passed: %d\nTests failed: %d\n", testResults.passed, testResults.failed);
}