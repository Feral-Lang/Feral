# Design notes to document (intentional behavior, not bugs)

These behaviors were confirmed as **by design** — they should be written
up in the language docs/grammar notes so tests and users treat them as
specified behavior.

## 1. `!` is undefined for `Str`

`!x` is only defined for `Nil`, `Bool`, `Int`, `Flt`.
`!''` raises *"callable '!' does not exist for type: Str"*.

**Rationale:** force explicit intent — use `s.empty()` (or `s.len() == 0`)
instead of overloading truthiness. Note that empty strings *are* still
falsy inside conditions (`if '' { ... }`), so the explicit form only
matters when using the `!` operator directly.

Document: "For strings, use `Str.empty()`; the `!` operator is not
defined for `Str`."

## 2. Closures do NOT capture enclosing function locals

A function returned from another function can see **globals** but not
the enclosing function's locals:

```fer
let makeCounter = fn() {
    let c = 0;
    return fn() { ++c; return c; };   # runtime error: variable 'c' does not exist
};
```

**Rationale (confirmed):** not a feature Feral wants. State it explicitly
in the docs, and document the supported alternatives:
- `feral.closure(fn, captures...)` for explicit capture (e.g. binding `self`),
- passing state in containers (`Vec`, `Map`, structs) or globals,
- `ref(...)` cells where supported.

## 3. Named call arguments are exclusively for the `.kw` bundle

Named arguments at a call site (`f(a = 1, b = 2)`) **never** bind to
declared parameters — they are collected into the function's keyword
bundle parameter (`.kw`) if one exists, and otherwise ignored/error.

```fer
let f = fn(a, b = 2) { return a.str() + b.str(); };
f(1, b = 9);        # -> '12'  (the named arg does NOT set b)

let g = fn(a, .kw) { return kw['z']; };
g(1, z = 5);        # -> 5     (this is the intended mechanism)
```

Document: "Keyword arguments go to the `.kw` bundle; parameters with
defaults must be given positionally."

## 4. `??=` does not exist (and is not planned)

Only the binary `x ?? y` form exists. The commented-out `??=` test lines
in `tests/builtin/nil-operator.fer` were removed for this reason.
Grammar should not list `??=` as a compound assignment.

## 5. Enum implicit values are positional, not sequential

In `enum(...)`, an unnamed/implicit member gets the value equal to its
**position among the positional (unnamed) arguments**, starting at 0 —
explicitly assigned values do not advance a counter:

```fer
let E = enum(.a = 5, .b, .c = 100, .d);
# E.a == 5, E.b == 0, E.c == 100, E.d == 1
```

Document this rule explicitly (it surprises people who expect C-style
`6` and `101`).

## 6. `Str` is not iterable

`for ch in 'abc'` fails with *"callable 'next' does not exist for type:
Str"*. This is intentional. Iteration alternatives:

```fer
for i in irange(s.len()) { s[i]; }      # byte indexing
let it = s.eachChar();                  # std/str iterator (codepoints)
s.eachLine();                           # line iteration
```

Document: "Strings are sequences of bytes addressable by index; use
`Str.eachChar()` for character iteration."
