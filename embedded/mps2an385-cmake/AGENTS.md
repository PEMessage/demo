# AGENTS.md — mps2an385-cmake

> Compact instructions for OpenCode sessions working on this embedded Cortex-M3 repo.

## What this is

Collection of independent embedded example programs for ARM Cortex-M3 (MPS2-AN385 board), built with CMake and runnable under QEMU. Each subdirectory under `src/` is a self-contained firmware image.

## Build system

- **CMake** is the build system; **Makefile** is the primary developer interface.
- The root CMake auto-discovers all `src/*` directories via `file(GLOB)` — do **not** manually add `add_subdirectory(src/...)` lines.

## Exact commands

| Goal | Command |
|------|---------|
| Build a module | `make src/<MODULE>` (e.g. `make src/hello_world`) |
| Run in QEMU | `make src/<MODULE> run` |
| Debug in QEMU (GDB server on :1234) | `make src/<MODULE> debug` |
| Clean a module | `make src/<MODULE> clean` |
| List available modules | `make help` |

The Makefile also symlinks `compile_commands.json` to the root after a successful build so `clangd` works out of the box.

## Writing a new `src/` module

1. Create `src/<MODULE>/`.
2. Add a `CMakeLists.txt` following this template:
   ```cmake
   cmake_minimum_required(VERSION 3.12)
   project(<MODULE> LANGUAGES C ASM)
   set(MODULE_NAME <MODULE>)
   file(GLOB SRCS *.c)
   add_executable_with_binaries(${MODULE_NAME} ${SRCS})
   target_link_libraries(${MODULE_NAME} CMSDK_CM3 CMSIS)
   target_include_directories(${MODULE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
   ```
3. Use `add_executable_with_binaries` instead of `add_executable`. The macro (defined in root `CMakeLists.txt`) auto-generates `.bin` and `.lss` files via `objcopy`/`objdump` post-build.
4. No root CMake edit is required; `src/*` is globbed automatically.

## Library dependencies

- `CMSDK_CM3` / `CMSDK_CM3_EXT` — board HAL
- `CMSIS` — core headers
- `freertos_kernel` / `freertos_config` / `freertos_hook` — FreeRTOS (heap=4, port=GCC_ARM_CM3)
- `lvgl` / `lvgl_port` — LVGL graphics (v9.4.0)

## QEMU runtime flags (Makefile `src/<MODULE> run` target)

```
qemu-system-arm \
  -machine mps2-an385 \
  -device mps2-fb \
  -device i2c-echo,address=0x28 \
  -cpu cortex-m3 \
  -kernel <MODULE>.bin \
  -serial stdio \
  -monitor telnet:127.0.0.1:4445,server,nowait
```

For debug: add `-s -S` (GDB stub on port 1234, halted at startup). Connect with `gdb-multiarch -ex "target remote :1234" <ELF>`.

## Generated / fetched code

Several `lib/` subdirectories are fetched at configure time via CMake `FetchContent`:
- `lib/FreeRTOS-Kernel/code` → FreeRTOS/FreeRTOS-Kernel.git `V11.2.0`
- `lib/LVGL/code` → lvgl/lvgl.git `v9.4.0`

These directories are not tracked by git (see `.gitignore` and the `FetchContent` guards that skip re-fetch if they exist).

