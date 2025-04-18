// 定义一个 trait
trait Greet {
    fn say_hi(&self);
}

// 为两个不同类型实现 Greet
struct Person;
struct Robot;

impl Greet for Person {
    fn say_hi(&self) {
        println!("Person says: Hello!");
    }
}

impl Greet for Robot {
    fn say_hi(&self) {
        println!("Robot says: Beep boop!");
    }
}

// 静态分发（编译期单态化）
fn static_dispatch<T: Greet>(item: &T) {
    item.say_hi();
}

// 动态分发（运行时虚表）
fn dynamic_dispatch(item: &dyn Greet) {
    item.say_hi();
}

fn main() {
    let person = Person;
    let robot = Robot;

    println!("=== 静态分发 ===");
    static_dispatch(&person); // 编译期确定调用 Person 的 say_hi
    static_dispatch(&robot);  // 编译期确定调用 Robot 的 say_hi

    println!("=== 动态分发 ===");
    dynamic_dispatch(&person); // 运行时通过虚表调用
    dynamic_dispatch(&robot);  // 运行时通过虚表调用

    // 动态分发的常见用法：存储异构类型集合
    let greeters: Vec<&dyn Greet> = vec![&person, &robot];
    println!("=== 动态分发集合 ===");
    for greeter in greeters {
        greeter.say_hi(); // 运行时动态调用
    }
}
