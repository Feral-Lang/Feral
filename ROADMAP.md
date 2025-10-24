# Feral's Roadmap

This file contains the roadmap for Feral. As the language grows, more things will be added.

**Note**: All tests must be verified for no memory leaks (I personally use Valgrind).

## General

- [ ] Coroutines
- [ ] IPC
- [ ] Develop REPL (in Feral)
  - [ ] Requires getch() or similar (does **not** show on output; reads each char as raw byte)
    - [ ] Will require a full set of key list (`kb.A`, `kb.B`, `kb.CAPS`, ...)

## Core & Standard Library Test Cases (Repository: [Feral-Lang/Feral](https://github.com/Feral-Lang/Feral))

More to be added...

## Book (Repository: [Feral-Lang/Book](https://github.com/Feral-Lang/Book))

- [ ] Implement more chapters
  - [ ] 9 - "Member" Functions
  - [ ] 10 - Modules/Imports
  - [ ] 11 - Scopes and Variable Assignment vs Copy
- [ ] Review existing chapters
- [ ] Implement highlighting for Feral code blocks (Highlight.js)

## Standard Library (Repository: [Feral-Lang/Feral-Std](https://github.com/Feral-Lang/Feral-Std))

More to be added...

## Other

These are possible ideas, but still in debate.

- [ ] A package manager - to install various external modules/libraries from a package index
