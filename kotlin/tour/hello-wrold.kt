

fun section(url : String) {
    println()
    println("Section: ${url}")
    println("------------------")
}


fun hello_world() {
    section(object {}.javaClass.enclosingMethod?.name ?: "unknow")

    if (true) {
        println("Hello, world!")


        // aren't any types declared
        val a = 123 // readonly
        var b: Int // Mutable
        b = 3

        // String templates
        println("val a is $a, b is $b")
        // evaluate a piece of code in a template expression

        // println("var b + 1 is ${b + 1}") // NOTE: this not work for ubuntu22.04 kotlinc-jvm 1.3-SNAPSHOT
    }
}

@Suppress("UNUSED_VARIABLE")
fun basic_types() {
    section(object {}.javaClass.enclosingMethod?.name ?: "unknow")
    if (true) {
        // variable declared without initialization
        // val d: Int
        // triggers an error
        // println(d)
        // variable 'd' must be initialized

        // val a: UInt = 1000 // !! not able to UInt
        val a: Int = 1000
        val b: Long = 100_000_000_000_000
        val c: Double = 3.14

        // allow redefine ?
        val d: Boolean = false

        val f: String = "log message"
        val g: Char = '\n'


        println("just assgin, not print anything ...")
    }
}


fun list_collections() {
    section(object {}.javaClass.enclosingMethod?.name ?: "unknow")
    if(true) {
        // Read only list
        // java : List<String> shapes = new ArrayList<>(Arrays.asList("triangle", "square", "circle"));
        // java9: List<String> shapes = new ArrayList<>(      List.of("triangle", "square", "circle"));
        val readOnlyShapes = listOf("triangle", "square", "circle")
        println(readOnlyShapes)
        // [triangle, square, circle]

        // Mutable list with explicit type declaration
        val shapes: MutableList<String> = mutableListOf("triangle", "square", "circle")
        println(shapes)
        // [triangle, square, circle]

        println(
            "count is ${shapes.count()}"
            + ", first is ${shapes.first()}"
            + ", last is ${shapes.last()}"
            + ", check 'square' in shapes ${"square" in shapes}" // 'in' operator
        )


        // read-only view of a mutable list
        // shadow copy
        @Suppress("UNUSED_VARIABLE")
        val shapesLocked: List<String> = shapes
    }
}


fun set_collections() {
    section(object {}.javaClass.enclosingMethod?.name ?: "unknow")
    if (true) {
        val readOnlyFruit = setOf("apple", "banana", "cherry", "cherry")
        println(readOnlyFruit)
        // [apple, banana, cherry]

        val fruit: MutableSet<String> = mutableSetOf("apple", "banana", "cherry", "cherry")
        fruit.add("dragonfruit")
        println(fruit)

        fruit.add("dragonfruit")
        println(fruit)
    }
}

fun main(args : Array<String>) {

    hello_world()
    basic_types()
    list_collections()
    set_collections()
}
