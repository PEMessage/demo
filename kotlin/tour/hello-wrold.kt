

fun section(url : String) {
    println("========================================================")
    println(url)
    println("========================================================")
}

fun main(args : Array<String>) {

    section("kotlin-tour-hello-world")
    if (true) {
        println("Hello, world!")

        
        // aren't any types declared
        val a = 123 // readonly 
        var b = 2 // Mutable
        b = 3

        // String templates 
        println("val a is $a") 
        // evaluate a piece of code in a template expression
        
        // println("var b + 1 is ${b + 1}") // NOTE: this not work for ubuntu22.04 kotlinc-jvm 1.3-SNAPSHOT
    }

    section("kotlin-tour-basic-types")
    if (true) {
        // variable declared without initialization
        val d: Int = 3
        // triggers an error
        // println(d)
        // variable 'd' must be initialized
    }
    if (true) {

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

    section("kotlin-tour-collections")
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
        

        // read-only view of a mutable list
        // shadow copy
        val shapesLocked: List<String> = shapes
    }

}
