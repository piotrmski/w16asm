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
    ExitCodeInvalidMinus,
    ExitCodeNumberWithoutDigits,
    ExitCodeStringTooLong,
    ExitCodeInvalidEscapeSequence,
    ExitCodeUnterminatedString,
    ExitCodeDeclaringValueOutOfMemoryRange,
    ExitCodeMemoryValueOverridden,
    ExitCodeTooManyLabels,
    ExitCodeInvalidInstruction,
    ExitCodeReferenceToInvalidAddress,
    ExitCodeUnexpectedTokenAfterInstruction,
    ExitCodeOriginOutOfMemoryRange,
    ExitCodeUnexpectedTokenAfterOrg,
    ExitCodeFillValueStringNotAChar,
    ExitCodeUnexpectedTokenAfterFill,
    ExitCodeFillCountNotPositive,
    ExitCodeInvalidDirective,
    ExitCodeInvalidToken,
    ExitCodeUndefinedLabel,
    ExitCodeUnexpectedLabelAtEndOfFile
};

#endif