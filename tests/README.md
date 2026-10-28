# Test suite notes

Run the whole suite with:

```sh
feral utils/testdir.fer tests
```

Every `.fer` file under `tests/` must exit 0. `tests/config.fecl`
configures the runner. Layout:

- `core/` — language semantics (parser, scoping, control flow,
  functions, prelude builtins)
- `std/` — one file per std module
- top level — end-to-end example programs (sorts, recursion, …)

`DESIGN-NOTES.md` (next to this file) documents language behaviors that
were confirmed to be *intentional design*, not bugs — read it before
"fixing" anything they cover.

## Coverage rationale

These tests were added (and later merged into this suite) after auditing
`tests/` against `Grammar.ebnf`, the parser, the prelude
(`lib/prelude`), and the native std modules (`lib/std/*.cpp`). During
the merge, near-identical content from the old `builtin/` tests and the
new tests was deduplicated (e.g. string multiplication/trim/upper/isSpace
cases, enum implicit values, compound assignment, `or`-block basics).

| File | Covers (previously untested) |
|---|---|
| `core/literals.fer` | hex/octal/binary literal forms incl. `0X`/upper digits, float literals, all three string quote styles, escape sequences, multi-line leading/trailing newline stripping, atom values (`.ok`), removal of prefix literal sugar (`ref"x"`) and dangling tokens being parse errors |
| `core/prelude.fer` (Operators section) | ternary `? :`, `->` member-access alias, `??` chains, unary `! + - ~`, precedence checks, `//` n-th root + float exponents on `**`/`//`, Str truthiness in conditions, `~=` being a parse error |
| `core/loops.fer` | `for ;; {}` and every omitted-clause combination, break/continue nesting semantics, for-in over `irange`/vec `.each()`/map `.each()`, **custom iterator via `next()` protocol**, for-in shadowing of outer variable |
| `core/scoping.fer` | bare-block scoping, if/elif/else block scoping, `inline if` scope merging, shadowing, multi-binding `let`, `let x in Ty = ...` keyed member attachment |
| `core/defer.fer` | LIFO order, defers inside loops/blocks running at block end, defers modifying captured variables, defers on early return, top-level defers at program end (the fd-open/defer-close usage test lives in `std/fs.fer`) |
| `core/prelude.fer` (Struct sections) | positional/named/mixed instantiation, nested structs and default-value isolation, chained member calls, `setTypeName`, `_type_()`/`_isType_`, enum implicit-vs-explicit values, struct pass-by-reference |
| `core/functions.fer` | functions as values (in vecs, returned, chosen), IIFE, anonymous recursion, hard error on function reassignment (`VarFn::onSet`), variadic unpack `f(args...)`, variadic params, default args, module aliasing, closure scope limits, call-stack overflow / `setMaxRecursion` |
| `core/error-handling.fer` | error object API (`str()` on `Failure`), or-block return values, empty or-block → nil, nested or-blocks, default `_` error variable, or-block on member calls, multi-level `.?` propagation, `feral.err` code/msg, catching lexer/parse/runtime errors around `evalCode`/`evalExpr`, distinct `<eval.N>` modules per eval |
| `core/prelude.fer` (String sections) | `int(base)` incl. hex + `from_chars` error checking, `flt()`, `bool()`, `isAlpha/isDigit/isSpace`, `sub`, `at`, `rfind`, `append` (in-place, returns nil), `fit` (right-align + `...` truncation), `lower`, split `maxDelimCount`/`ignoreEmpty`, runtime `Str * Int` incl. `* 0`/`* -1`, `Flt.str(precision = n)`, `Int.str(base = n)`, path join `/` |
| `std/mutex.fer` | `std/mutex` had **no tests at all**: lock/unlock/tryLock, recursive mutex, lockguard RAII, mutex-protected multithreaded counter |
| `std/atomic.fer` | `std/atomic` had **no direct tests**: `newBool`/`newInt`, `get`/`set`, `_copy_` independence, atomic stop-signal across threads |
| `std/logging.fer` | `std/logging` had **no tests at all**: level filtering to file, custom prefix with `$<<tag>>` fmt, `pushTag`/`popTag`, `scopedTag`/`scopedDepth` RAII |
| `std/term.fer` | `std/term` had **no tests at all**: `getWidth/getHeight`, `isTTY`, ANSI constants, const binding of constants, `link()` OSC-8 output |

## Status of previously reported issues

### Fixed ✔

- **Binary literals** — `0b1010` → `10`, `0B11` → `3` now lex correctly
  (grammar + lexer agree; `core/literals.fer` covers them).
- **`Int.str(base = n)`** — `(255).str(base = 16)` → `'ff'`
  (the `getAttr("precision")` copy-paste bug is fixed).
- **`feral.evalCode` with `return`** — return value propagates
  (`VirtualMachine::eval` now forwards `tmpRet`).
- **Str `*`** — correct for all n including `* 0` and `* -1` (both `''`);
  `std/logging`'s depth prefix renders correctly.
- **`~=` removed** — from lexer, parser, prelude **and the grammar**.
- **Prefixed/suffixed literal sugar removed** — gone from grammar and
  parser. `ref"x"` / `9 h` are now clean parse errors in file context,
  which also kills the newline-swallowing hazard (`let a = 9` ↵ `h;`
  used to silently become `h(9)`). `core/literals.fer` pins the removal.
- **Dangling tokens in `feral.evalExpr`/`evalCode`** — a complete
  expression followed by unconsumed tokens now fails with
  *"dangling token found: …"* (`evalExpr('1 2')` is an error, not `1`).
  Regression-asserted in `core/literals.fer`.
- **Error display for eval modules** — `ErrorHandler::addFile` now
  overwrites (`insert_or_assign`), so consecutive failed `evalCode`/
  `evalExpr` calls each print their own path and source line with the
  correct caret (previously `<eval.1>` errors replayed `<eval.0>`'s
  source, because failed modules register a module ID but `makeModule`
  only increments the counter on success — the next module reused the
  ID and the old `emplace` refused to re-map the file).
- **Grammar `//` comment** — now "integer root", matching the n-th root
  implementation (`64 // 2` → `8`, `27 // 3` → `3`).
- **`Str.int(base)`** — `'ff'.int(16)` → `255`; and `from_chars`' error
  code is now checked, so `'ff'.int()` → *"not a number: ff"*,
  `'99999999999999999999'.int()` → *"can't fit"*, `'8'.int(8)` raises
  (no more garbage ints; regression-asserted in `core/prelude.fer`).
- **`split(delim, 1)`** — remainder is kept: `['a', 'b,c']`.
- **`getUTF8CharFromHexString`** — removed from the prelude.
- **`**` with float exponents** — `2 ** 3.0` → `8.00` (Flt); `//` too.
- **`void`** — removed from the grammar's `primary` rule.
- **README** — `range(...)` → `irange(...)`.
- **Function reassignment** — hard error (`VarFn::onSet`), asserted in
  `core/functions.fer`.
- **Str truthiness in conditions** — `if ''` falsy, `if 'x'` truthy.
- **`Flt.str(precision = n)`** — works (default 2).

### Confirmed as intentional design — see `DESIGN-NOTES.md`

- `!` undefined for `Str` (use `Str.empty()`)
- closures do not capture enclosing function locals
- named call args are exclusively `.kw`-bundle keyword args
- `??=` does not exist
- enum implicit values are positional among positional args
- `Str` is not iterable (use `eachChar()` / indexing)

### Still broken / open ✘

1. **`feral.evalCode` returns nil for implicit last-expression values**
   (only explicit `return` produces a value; e.g. `evalCode('let a = 1; a;')`
   → nil). Fine if intentional — document it.
2. **Comma expressions parse but don't execute** — *"callable ',' does not
   exist for type: Int"*.
3. **Struct `==` compares only type identity.** `P(1) == P(2)` is `true`,
   and even two *different* struct types compare equal.
4. **Module-ID reuse caveat (latent).** The eval-display fix makes
   `addFile` overwrite, but `makeModule` still only consumes
   `moduleIdCtr` on success, so IDs are reused after failed modules.
   That's now harmless for display, but `ModuleId` is 16-bit and
   `err.files`/`gs->modules` entries for dead modules are never pruned:
   in a long-running process doing >65536 modules (each `eval` counts),
   a wrapped ID could remap the file entry of a still-live module and
   mis-attribute its errors. Consuming the ID unconditionally at the
   top of `makeModule` (and bumping the bytecode version, since `.bc`
   files embed module IDs) would remove the reuse entirely.
