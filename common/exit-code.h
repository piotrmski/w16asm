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
    ExitCodeLabelNameTooLong,
    ExitCodeInvalidLabelName,
    ExitCodeLabelNameNotUnique,
    ExitCodeInvalidNumberLiteral,
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
    ExitCodeInvalidCharacterLiteral,
    ExitCodeUndefinedLabel,
    ExitCodeUnexpectedLabelAtEndOfFile
};

#endif