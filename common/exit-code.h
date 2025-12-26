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
    ExitCodeCharacterLiteralOutOutRange,
    ExitCodeUnexpectedCharacter,
    ExitCodeLabelNameTooLong,
    ExitCodeInvalidLabelName,
    ExitCodeLabelNameNotUnique,
    ExitCodeInvalidMinus, // ??
    ExitCodeNumberWithoutDigits, // ??
    ExitCodeInvalidEscapeSequence,
    ExitCodeUnterminatedString,
    ExitCodeDeclaringValueOutOfMemoryRange,
    ExitCodeMemoryValueOverridden,
    ExitCodeTooManyLabelDefinitions,
    ExitCodeTooManyLabelUses,
    ExitCodeInvalidInstruction,
    ExitCodeReferenceToInvalidAddress,
    ExitCodeOriginOutOfMemoryRange,
    ExitCodeInvalidDirectiveArgument,
    ExitCodeInvalidInstructionArgument,
    ExitCodeFillCountNotPositive,
    ExitCodeInvalidDirective,
    ExitCodeInvalidToken,
    ExitCodeInvalidCharacter,
    ExitCodeUndefinedLabel,
    ExitCodeUnexpectedLabelAtEndOfFile
};

#endif