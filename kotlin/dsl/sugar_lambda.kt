// This file will report error under kotlin 1.3 (Ubuntu22.04 apt default)
// But work fine in kotlin 2.2
//
// Build: kotlinc sugar_lambda.kt && kotlin Sugar_lambdaKt.class
//
// Same function definition

// See:
// 【Kotlin颜值为啥遥遥领先 | 不可变变量 | lambda | 语法糖 | 构造函数 | 教程 | 中缀表达式 | val var】
// https://www.bilibili.com/video/BV1ng4y1X7nC/?share_source=copy_web

fun operateOnNumbers(a: Int, b: Int, operation: (Int, Int) -> Int): Int {
    return operation(a, b)
}

fun call(fn: () -> Unit) {
    fn()
}

fun main() {
    call({
        println("This is normal call")
    })

    call() {
        println("last argument could me out of (), if it's a lambda")
    }

    call {
        println("Empty () could be rm")
    }

    val sum1 = operateOnNumbers(5, 3, { x: Int, y: Int -> x + y })
    val sum2 = operateOnNumbers(5, 3) { x, y -> x + y } // same as above

    println(sum1)
    println(sum2)
}
