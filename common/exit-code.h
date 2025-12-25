#ifndef exit_code
#define exit_code

enum ExitCode {
    ExitCodeSuccess = 0,
    ExitCodeCouldNotReadAsmFile,
    ExitCodeCouldNotWriteBinFile,
    ExitCodeCouldNotWriteSymbolsFile,
    ExitCodeResultProgramEmpty,
    ExitCodeProgramArgumentsInvalid,
    ExitCodeNumberLiteralOutOutRange,
    ExitCodeUnexpectedCharacter,
    ExitCodeLabelNameTooLong,
    ExitCodeLabelNameNotUnique,
    ExitCodeInvalidMinus,
    ExitCodeNumberWithoutDigits,
    ExitCodeInvalidEscapeSequence,
    ExitCodeUnterminatedString,
    ExitCodeDeclaringValueOutOfMemoryRange,
    ExitCodeMemoryValueOverridden,
    ExitCodeTooManyLabels,
    ExitCodeInvalidInstruction,
    ExitCodeReferenceToInvalidAddress,
    ExitCodeUnexpectedTokenAfterInstruction,
    ExitCodeOriginOutOfMemoryRange,
    ExitCodeInvalidDirectiveArgument,
    ExitCodeInvalidInstructionArgument,
    ExitCodeFillCountNotPositive,
    ExitCodeInvalidDirective,
    ExitCodeInvalidToken,
    ExitCodeUndefinedLabel,
    ExitCodeUnexpectedLabelAtEndOfFile
};

#endif