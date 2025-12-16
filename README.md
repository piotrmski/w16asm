# W16 assembler

This program is used to process assembly source files into a binary format, which can later be loaded into the memory of a W16 simulator.

## Building

A C compiler supporting the C23 standard, aliased as `CC` (such as `GCC` or `Clang`) and `make` are required to build this assembler from source.

Run `make` to build the assembler. The `w16asm` executable will be produced in the `dist` directory.

## Usage

Run `w16asm path/to/source.asm path/to/result.bin` to assemble the `source.asm` assembly file and save the resulting binary to `result.bin`.

Run `w16asm path/to/source.asm path/to/result.bin path/to/symbols.csv` to do the same as before, and additionally produce a `symbols.csv` comma-separated file containing descriptions of memory addresses:

- the first column is the memory address in hexadecimal format,
- the second column is `int`, `char` or `instruction`,
- the third column is the label name describing the given address (or the first label name, in case multiple labels describe the same address).

## Characteristics of W16

W16 is an imaginary microarchitecture and ISA designed with extreme minimalism in mind. Full details can be found at [https://github.com/piotrmski/w16sim](https://github.com/piotrmski/w16sim).

The word size is 8 bits. 2<sup>13</sup> memory addresses are addressable. One general purpose register is available. Byte order is little-endian.

Eight instructions are available, exclusively in absolute addressing mode. All instructions are two words wide and follow the same format:

- the 3 most significant bits are the opcode,
- the other 13 bits are the absolute memory address.

The reference simulator at [https://github.com/piotrmski/w16sim](https://github.com/piotrmski/w16sim) supports the following memory map:

- 0x0000-0x1FFD - program memory,
- 0x1FFB-0x1FFE - 32-bit relative time - reading from these addresses yields a number of milliseconds since the simulator has started until the last (or current) time 0x1FFB was (or is) read,
- 0x1FFF - terminal I/O:
    - Loading from 0x1FFF removes a character from the standard input buffer and yields it, or 0 if the buffer is empty,
    - Storing to 0x1FFF pushes a character to the standard output buffer.

## Syntax

The syntax of this W16 assembly language is very relaxed and mostly newline-agnostic. A single line may contain multiple instructions and directives, or a single instruction or directive with its arguments may span multiple lines.

Any number of labels may precede an instruction or a directive. A label name must be between 1 and 31 characters, which could be uppercase or lowercase letters, digits and underscores (the first character can’t be a digit). The label name must be followed by a colon `:`. Label names are case-sensitive and must be unique.

A comment starts with a semicolon `;` and ends with a newline character (CR or LF).

Instruction names are case-insensitive. All instructions have one argument - an absolute memory address - which may be expressed as a label name or a number between 0 and 8191 in any representation.

Directive names are also case-insensitive.

Data declarations are not required to be preceded by any directive - a number literal, a character, or a string by itself creates a data declaration.

## Instruction set

- `LD X` - loads value from memory address X to the register A,
- `NOT X` - loads bitwise complement of the value from memory address X to the register A,
- `ADD X` - performs addition between the value from memory address X and the value from register A, and stores the result in the register A,
- `AND X` - performs binary operation AND between the value from memory address X and the value from register A, and stores the result in the register A,
- `ST X` - stores value from the register A to the memory address X,
- `JMP X` - performs an unconditional jump to X,
- `JMN X` - if the most significant bit of the value in register A is 1, then performs a jump to X,
- `JMZ X` - if the value in register A is 0, then performs a jump to X.

## Directives

- `.ORG` followed by a number between 0 and 8191 as an argument. Places the following data, instructions, or directives at a specified memory address.
- `.ALIGN` followed by a number between 1 and 12 as an argument. Places the following data, instructions, or directives at the nearest (towards higher) address where a number of least significat bits equal to the argument value are unset. Examples:
    - current address is 0x0123, `.ALIGN 4` sets the next address to 0x0130,
    - current address is 0x0120, `.ALIGN 4` sets the next address to 0x0120,
    - current address is 0x0123, `.ALIGN 8` sets the next address to 0x0200,
    - current address is 0x0100, `.ALIGN 8` sets the next address to 0x0100.
- `.FILL` followed by two arguments:
    - value to be filled (a number between -128 and 255 or a character literal),
    - number of words to fill (a number greater than 0).

## Data types

- Numbers, which may be expressed in the following representations:
    - decimal `/^(0|\-?[1-9][0-9]*)$/` (negative numbers are expressed in 2’s complement),
    - hexadecimal `/^0x[0-9A-F]+$/` (case-insensitive),
    - binary `/^0b[0-1]+$/` (case-insensitive),
    - octal `/^0[0-7]+$/`,
- Single characters or non-zero-terminated strings in single quotes `'`,
- Zero-terminated strings in double quotes `"`.

Numbers in data declarations are limited to 8 bits.

The following escape sequences can be used in strings and characters (they are case-insensitive):

- `\n` - newline
- `\t` - horizontal tab
- `\r` - carriage return
- `\'` - '
- `\"` - "
- `\xNN` - the byte whose numerical value is given by NN interpreted as a hexadecimal number (two hex digits are required)
- `\\` - \

## Examples

The `examples` directory contains example assembly files alongside the produced binary and CSV files. Optionally use `assemble-examples.sh` to assemble the .asm files yourself.

## Testing

Run `run-tests.sh` to build and run the test suite.

# License

Copyright (C) 2025 Piotr Marczyński. This program is licensed under GNU GPL v3. See the file COPYING.