

use std::io;

#[derive(Debug, Default)]
struct Point {
    x: u32,
    y: u32,
}


fn main() {
    // Part 1, hello world
    println!("Hello, world!");


    // Part 2, basic input
    print!("Please input something: ");
    io::Write::flush(&mut io::stdout()).expect("Failed to flush stdout"); // we have to flash
                                                                          // before input
                                                                          // Rust 的标准输出（stdout）默认是 行缓冲 的（在终端里）
                                                                          // 即遇到换行符（\n）才会真正输出
    let mut input = String::new();
    io::stdin()
        .read_line(&mut input)
        .expect("Failed to read line");


    println!("You entered: {}", input);

    // Part 3, learn basic trait Debug / Default
    let point1  = Point::default();
    let point2 = Point { x: 1,  ..Point::default() };

    dbg!(point1);
    dbg!(point2);
}
