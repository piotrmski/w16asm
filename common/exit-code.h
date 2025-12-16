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
    ExitCodeInvalidAlignParameter,
    ExitCodeFillValueStringNotAChar,
    ExitCodeUnexpectedTokenAfterFill,
    ExitCodeFillCountNotPositive,
    ExitCodeInvalidDirective,
    ExitCodeInvalidToken,
    ExitCodeUndefinedLabel,
    ExitCodeUnexpectedLabelAtEndOfFile
};

#endif