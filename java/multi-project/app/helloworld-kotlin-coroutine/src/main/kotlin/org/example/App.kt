package org.example

import kotlinx.coroutines.async
import kotlinx.coroutines.delay
import kotlinx.coroutines.runBlocking

class App {
    suspend fun getGreeting(): String {
        delay(100)
        return "Hello World!"
    }
}

fun main() = runBlocking {
    val greeting = async { App().getGreeting() }.await()
    println(greeting)
}
