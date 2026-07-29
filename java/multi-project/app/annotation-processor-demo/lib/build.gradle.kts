plugins {
    `java-library`
}

dependencies {
    implementation("com.squareup:javapoet:1.13.0")
}

java {
    toolchain {
        languageVersion = JavaLanguageVersion.of(17)
    }
}
