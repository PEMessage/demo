plugins {
	id("java")
	id("dev.nokee.jni-library")
	id("dev.nokee.cpp-language")
}

java {
    toolchain {
        languageVersion = JavaLanguageVersion.of(17)
    }
}
