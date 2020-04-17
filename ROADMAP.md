# Feral's Roadmap

This file contains the roadmap for Feral. As the language grows, more things will be added.

**Note**: All tests must be verified for no memory leaks (I personally use Valgrind).

## Core & Standard Library Test Cases (Repository: [Feral-Lang/Feral](https://github.com/Feral-Lang/Feral))

- [ ] Writing test cases for the language, related to:
  - [x] Nil
  - [x] Integers
  - [x] Floating point numbers
  - [x] Strings
  - [x] Modules (`tests/builtin/mod.fer`)
  - [x] Language Constructs
    - [x] Conditionals
    - [x] Loops
      - [x] For (`tests/multi_loops.fer`)
      - [x] Foreach (`tests/multi_loops_range.fer`)
      - [x] While
    - [x] Continue (`tests/multi_loops*.fer`)
    - [x] Break (`tests/multi_loops*.fer`)
    - [x] Functions (`tests/facto_*.fer`)
    - [x] Return (`tests/facto_*.fer`)

Standard Library:
- [x] IO
- [x] Vectors
- [ ] Maps
- [x] FS
- [x] Lang
- [ ] OS
- [x] Ptr
- [x] Runtime
- [ ] String
- [x] Sys

## Book (Repository: [Feral-Lang/Book](https://github.com/Feral-Lang/Book))

- [ ] Implement more chapters
  - [x] 7 - Conditionals
  - [x] 8 - Loops
    - [x] For
    - [x] Foreach
    - [x] While
  - [ ] 9 - "Member" Functions
  - [ ] 10 - Modules/Imports
  - [ ] 11 - Scopes and Variable Assignment vs Copy
  - [ ] 12 - Standard Library
    - [ ] 1 - IO
    - [ ] 2 - Structures & Enums
    - [ ] 3 - Strings
    - [ ] 4 - Vectors
    - [ ] 5 - Maps
    - [ ] 6 - FS
    - [ ] 7 - OS
    - [ ] 8 - Sys
    - [ ] 9 - Builder
- [ ] Review existing chapters
- [ ] Implement highlighting for Feral code blocks (Highlight.js)

## Standard Library (Repository: [Feral-Lang/Feral-Std](https://github.com/Feral-Lang/Feral-Std))

- [ ] FS (FileSystem)
  - [ ] For `var_file_t`
    - [ ] `fs_file_write(data)` - writes data to a file
    - [ ] `fs_file_writeln(data)` - writes data to a file and adds a newline character at the end
    - [ ] `fs_file_is_open()` - returns true if a file is open, false if not
    - [ ] `fs_file_mode()` - returns the (string) mode in which the file was opened
- [x] Maps
  - [x] `var_map_iterable_t` - Iterates through each key value pair in a map (similar to vector's `var_vec_iterable_t`)
- [x] OS
  - [x] `set_working_directory()` - Changes the program's working directory
- [x] Ptr
- [ ] Runtime
  - [ ] `var_exists()` - Checks if a variable name exists in the virtual machine at the time of function call

## Other

These are possible ideas, but still in debate.

- [ ] A proper build system - for building external modules/libraries without the need of CMake
- [ ] A package manager - to install various external modules/libraries from a package index