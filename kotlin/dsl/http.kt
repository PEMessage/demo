
// Define a DSL for building HTML-like structures
class Html {
    private val children = mutableListOf<Any>()

    fun head(block: Head.() -> Unit) {
        children.add(Head().apply(block))
    }

    fun body(block: Body.() -> Unit) {
        children.add(Body().apply(block))
    }

    override fun toString(): String {
        return "<html>\n${children.joinToString("\n")}\n</html>"
    }
}

class Head {
    private val children = mutableListOf<String>()

    fun title(text: String) {
        children.add("<title>$text</title>")
    }

    override fun toString(): String {
        return "<head>\n${children.joinToString("\n")}\n</head>"
    }
}

class Body {
    private val children = mutableListOf<String>()

    fun h1(text: String) {
        children.add("<h1>$text</h1>")
    }

    fun p(text: String) {
        children.add("<p>$text</p>")
    }

    override fun toString(): String {
        return "<body>\n${children.joinToString("\n")}\n</body>"
    }
}

// DSL builder function
fun html(block: Html.() -> Unit): Html {
    return Html().apply(block)
}

// Usage example
fun main() {
    val myHtml = html {
        head {
            title("My Page")
        }
        body {
            h1("Welcome")
            p("This is a minimal Kotlin DSL example.")
        }
    }

    val myHtml_desugar = html({ // this in lambda /vs/ this in method???
        this.head({
            this.title("My Page")
        })
        this.body({
            this.h1("Welcome")
            this.p("This is a minimal Kotlin DSL example.")
        })
    })

    println(myHtml)
    println(myHtml_desugar)
}
