# simple-demo

Slightly richer than hello-world: **one static library + one executable**,
demonstrating GN's core concepts. Everything is declared explicitly — no
implicit rules at all (no `set_defaults`, no `default_configs`, no bundled
`gcc_toolchain` template).

## Layout

```
.gn                          # repo root marker, points to buildconfig
build/config/BUILDCONFIG.gn  # build entry: set_default_toolchain, the only global default
build/toolchain/BUILD.gn     # toolchain("gcc"): all six tools (cc/cxx/alink/link/stamp/copy) hand-written
build/config/BUILD.gn        # config("warnings"): an explicit set of compile options
BUILD.gn                     # group("all"): root aggregation target
lib/greeting/                # static_library + public_configs (propagates header search path)
src/app/                     # executable, deps pointing at the library
```

## Concepts covered

| Concept | Location |
| --- | --- |
| `.gn` root marker and `buildconfig` | `.gn` |
| `set_default_toolchain` (the only global default) | `build/config/BUILDCONFIG.gn` |
| `toolchain` / `tool` / `{{...}}` substitutions / `depfile` | `build/toolchain/BUILD.gn` |
| `config` (named config holding cflags etc.) | `build/config/BUILD.gn` |
| `static_library`, `public`, `public_configs` | `lib/greeting/BUILD.gn` |
| `executable`, `deps` | `src/app/BUILD.gn` |
| `group` (no outputs, pure dependency aggregation) | `BUILD.gn` |
| Label syntax `//dir:target` and `:target` | each BUILD.gn |

Note: every target lists its own `configs` by hand (even warnings are not
added automatically), and include paths reach dependents only via the
library's `public_configs`. There is no other implicit behavior.

## Usage

```sh
cd simple-demo
gn gen out          # reads .gn -> BUILDCONFIG.gn -> writes out/build.ninja
ninja -C out        # builds group("all")
./out/app           # prints: Hello, GN!
gn check out        # header dependency check
gn desc out //src/app:app   # inspect a target's resolved configs/deps/outputs
```
