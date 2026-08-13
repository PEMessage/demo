# DSO Global Symbol Interposition

## Problem

When multiple shared libraries (DSO, `.so`) each export a global symbol with the **same name, external linkage, and default visibility**, the dynamic linker merges them: per its symbol interposition rule, the **first-loaded definition wins** and all references collapse onto one instance.

This is **not a mutex problem** — any same-named global variable hits it (`int`, singletons, config tables...). A `std::mutex` just makes it visible: the merged instance gets constructed twice and destroyed twice, so the process aborts with a double-free at exit. Other symptoms: broken data isolation between libraries, deadlocks on a "shared" mutex.

## Principle

- A global defined in a `.cpp` without `static`/`extern` has external linkage and default visibility → it is exported.
- The dynamic linker binds every reference to the **first** definition found in the global scope (interposition).
- Result: the two libraries silently share one object instead of each owning its own.

## Best Practices

1. **Export only the API.** Build with `-fvisibility=hidden`; mark public functions with `__attribute__((visibility("default")))` (via a macro). Optionally lock the ABI with a version script (`global: <API>; local: *;`).
2. **Never export data symbols.** Keep state internal; if outsiders need it, expose accessor functions.
3. **Give internal globals internal linkage:** `static` or an anonymous namespace. A `const` namespace-scope variable is internal by default in C++.
4. **For one instance shared across a library's TUs**, hide it behind a function-local static (Meyers singleton) defined in a single `.cpp` — do *not* put inline functions with local statics in headers, they get interposed across DSOs too.
5. **Detect it early** with ASan's ODR check (`detect_odr_violation`), which reports duplicate globals at load time.

## Demo

`global_mutex_name_conflict/` reproduces the bug and compares fixes (source-level `static`/anonymous namespace, and no-source-change options `-Bsymbolic`, version script, `-fvisibility=hidden`). Run:

```bash
./run_all.sh
```
