plugins {
    application
}

dependencies {
    implementation(project(":annotation-processor-lib"))
    annotationProcessor(project(":annotation-processor-lib"))

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

tasks.named<Test>("test") {
    useJUnitPlatform()
}
