package org.example

import kotlinx.coroutines.runBlocking
import org.junit.jupiter.api.Assertions.assertNotNull
import org.junit.jupiter.api.Test

class AppTest {
    @Test
    fun appHasAGreeting() {
        val greeting = runBlocking { App().getGreeting() }
        assertNotNull(greeting)
    }
}
