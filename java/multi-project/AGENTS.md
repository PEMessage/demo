## Project Overview
1. This is a big monorepo, usually only need handle one of module.
2. Do not introduce other dependencies if user not asking it.


## Module Structure
```
app/<module_name>/
├── build.gradle.kts          # Module build config
├── src/main/
│   ├── java/org/example/
│   │   └── App.java (or .kt)
└── src/test/java/            # Unit tests (JUnit 4)
```
