# W16 assembler

This program is used to process assembly source files into a binary format, which can later be loaded into the memory of a W16 simulator.

## Usage

Run `w16asm path/to/source.asm path/to/result.bin` to assemble the `source.asm` assembly file and save the resulting binary to `result.bin`.

Run `w16asm path/to/source.asm path/to/result.bin path/to/symbols.csv` to do the same as before, and additionally produce a `symbols.csv` comma-separated file containing descriptions of memory addresses:

- the first column is the memory address in hexadecimal format,
- the second column is `int`, `char` or `instruction`,
- the third column is the label name describing the given address (or the first label name, in case multiple labels describe the same address).

## Building

A C compiler supporting the C23 standard, aliased as `CC` (such as `GCC` or `Clang`) and `make` are required to build this assembler from source.

Run `make` to build the assembler. The `w16asm` executable will be produced in the `dist` directory.

## Testing

Run `run-tests.sh` to build and run the test suite.

# W16 assembly language

W16 is an imaginary microarchitecture and ISA designed with extreme minimalism in mind. Full details can be found at [https://github.com/piotrmski/w16sim](https://github.com/piotrmski/w16sim).

The word size is 8 bits. 2<sup>13</sup> memory addresses are addressable. Byte order is little-endian.

Two registers exist in this architecture, both are initialized to 0:

- program counter "PC" containing the address of the currently executed instruction (13 bits),
- general purpose register "A" (8 bits).

Eight instructions are available, exclusively in absolute addressing mode. All instructions are two words wide and follow the same format:

- the 3 most significant bits are the opcode,
- the other 13 bits are the absolute memory address.

The reference simulator at [https://github.com/piotrmski/w16sim](https://github.com/piotrmski/w16sim) supports the following memory map:

- 0x0000-0x1FFA - program memory,
- 0x1FFB-0x1FFE - 32-bit relative time - reading from these addresses yields a number of milliseconds since the simulator has started until the last (or current) time 0x1FFB was (or is) read,
- 0x1FFF - terminal I/O:
    - Loading from 0x1FFF removes a character from the standard input buffer and yields it, or 0 if the buffer is empty,
    - Storing to 0x1FFF pushes a character to the standard output buffer.

The entry point is 0x0000.

## Syntax

W16 assembly code consists of one or more **statements**. A **statement** consists of zero or more **label definitions**, followed by:
- an **instruction** name and an argument,
- a **directive** name and arguments, or
- **data declaration**.

Whitespace must separate statements, arguments, and instruction or directive names from their arguments. Whitespace includes newline characters, therefore a single statement may span multiple lines of assembly code.

Any line of assembly code may end with a **comment** starting with a semicolon `;`.

## Labels

A label definition consists of a label name followed by a colon `:`. A label name must be between 1 and 31 characters, which can be uppercase or lowercase letters, digits and underscores (the first character can't be a digit). Label names are case-sensitive and must be unique.

When using a label name as an argument to an instruction or `.LSB` or `.MSB` directive, it can optionally be suffixed by a plus or minus sign followed by a number. For example if `foo` is a label that evaluates to `0x0123`, then `foo+1` evaluates to `0x0124`, and `foo-2` evaluates to `0x0121`.

## Instructions

Instruction names are case-insensitive. All instructions have one argument - an absolute memory address - which may be expressed as a number between 0 and 8191 in any representation (see Data declaration), or a label name, optionally with an offset.

Instruction set:

- `LD X` - loads value from memory address X to the register A,
- `NOT X` - loads bitwise complement of the value from memory address X to the register A,
- `ADD X` - performs addition between the value from memory address X and the value from register A, and stores the result in the register A,
- `AND X` - performs binary operation AND between the value from memory address X and the value from register A, and stores the result in the register A,
- `ST X` - stores value from the register A to the memory address X,
- `JMP X` - performs an unconditional jump to X,
- `JMN X` - if the most significant bit of the value in register A is 1, then performs a jump to X,
- `JMZ X` - if the value in register A is 0, then performs a jump to X.

## Directives

Directive names are case-insensitive.

- `.ORG` followed by a number between 0 and 8191 as an argument. Places the following statements at a specified memory address.
- `.ALIGN` followed by a number between 1 and 12 as an argument. Places the following statements at the nearest (towards higher) address where a number of least significant bits equal to the argument value are unset. Examples:
    - current address is 0x0123, `.ALIGN 4` sets the next address to 0x0130,
    - current address is 0x0120, `.ALIGN 4` sets the next address to 0x0120,
    - current address is 0x0123, `.ALIGN 8` sets the next address to 0x0200,
    - current address is 0x0100, `.ALIGN 8` sets the next address to 0x0100.
- `.FILL` followed by two arguments:
    - value to be filled (a number between -128 and 255 or a character literal, see Data declaration),
    - number of words to fill (a number greater than 0).
- `.LSB` followed by a label name, optionally with an offset. Places in memory the least significant byte of an address that a label evaluates to.
- `.MSB` followed by a label name, optionally with an offset. Places in memory the most significant byte of an address that a label evaluates to.

## Data declaration

Stating a number between -128 and 255 or a character literal constitutes a declaration of one byte of data. Stating a string of characters constitutes a declaration of multiple bytes of data followed by 0.

A number may be represented in base 10, or in base 16 if prefixed by `0x`, or in base 8 if prefixed by `0`. Negative numbers are represented in two's complement.

A character literal must be enclosed in single quotes `'`. It may be prefixed by `-`, in which case two's complement of a character value is placed in memory. It may also be suffixed by `+` or `-` followed by a number, in which case the number is added to or subtracted from the value of a character, or its two's complement. For example `-'z'-1` evaluates to -123.

A zero-terminated string of characters must be enclosed in double quotes `"`.

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

# License

Copyright (C) 2025 Piotr Marczyński. This program is licensed under GNU GPL v3. See the file COPYING.