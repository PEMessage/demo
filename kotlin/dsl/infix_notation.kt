
// Marking a function with the infix keyword.
// Ensuring it has only one parameter.
infix fun String.concatWith(separator: String): String {
    return "$this$separator$separator$this"  // Example: "Hello" concatWith "-" → "Hello--Hello"
}


fun main() {
    // Using 'to' to create Pairs
    val pair1 = "name" to "Alice"  // Pair<String, String>
    val pair2 = 1 to "One"         // Pair<Int, String>

    println(pair1)  // Output: (name, Alice)
    println(pair2)  // Output: (1, One)

    // Destructuring a Pair
    val (key, value) = pair1
    println("Key: $key, Value: $value")  // Output: Key: name, Value: Alice

    // Using in a Map
    val map = mapOf(
        "a" to 1,
        "b" to 2,
        "c" to 3
    )
    println(map)  // Output: {a=1, b=2, c=3}

    val result1 = "Hello".concatWith("-")
    println(result1)  // Output: "Hello--Hello"

    // Infix notation (more readable)
    val result2 = "Kotlin" concatWith "***"
    println(result2)  // Output: "Kotlin******Kotlin"
}
