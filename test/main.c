#include <stdio.h>

struct TestResults {
    int passed;
    int failed;
};

void expectErrorCode(const char* asmFileName, int expectedErrorCode, struct TestResults* testResults) {

}

void expectSuccess(const char* asmFileName, struct TestResults* testResults) {
    
}

int main(int argc, const char * argv[]) {
    struct TestResults testResults = { 0, 0 };

    printf("Tests passed: %d\nTests failed: %d\n", testResults.passed, testResults.failed);
}