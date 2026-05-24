
// Apply a specific Java toolchain to ease working on different environments.
plugins {
    java
}

java {
    toolchain {
        languageVersion = JavaLanguageVersion.of(17)
    }
}
