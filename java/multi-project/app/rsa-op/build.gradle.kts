plugins {
    application
}

dependencies {
    implementation(libs.bouncycastle.bcprov)
    implementation(libs.bouncycastle.bcutil)
    implementation(libs.bouncycastle.bcpkix)

    testImplementation(libs.junit.jupiter)
    testRuntimeOnly("org.junit.platform:junit-platform-launcher")
}

java {
    toolchain {
        languageVersion = JavaLanguageVersion.of(17)
    }
}

application {
    mainClass = "org.example.App"
}
