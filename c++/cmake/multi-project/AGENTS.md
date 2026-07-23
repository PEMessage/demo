# multi-project

Part of a big monorepo; usually only need to handle one module at a time.

## Commands

```sh
just configure     # cmake -B build (run once)
just build src/uv_version   # single target
just run src/uv_version     # build + run
just build-all       # build everything
just clean           # rm -rf build
```

## Module Structure

```
src/<module_name>/
├── CMakeLists.txt          # target name = module_name
└── main.c                  # source files
```

## Notes

- C and C++ project
- Add a new module: refer to existing modules for convention, then `add_subdirectory(src/NAME)` in root CMakeLists.txt
