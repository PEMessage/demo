use std::fs::File;
use std::io::{self, Read};

// A function that returns a Result
fn divide(a: f64, b: f64) -> Result<f64, String> {
    if b == 0.0 {
        Err(String::from("Cannot divide by zero"))
    } else {
        Ok(a / b)
    }
}

// A function that uses the ? operator to propagate errors
fn read_file_contents(path: &str) -> Result<String, io::Error> {
    let mut file = File::open(path)?;
    let mut contents = String::new();
    file.read_to_string(&mut contents)?;
    Ok(contents)
}

fn main() {
    // Basic Result handling with match
    let result = divide(10.0, 2.0);
    match result {
        Ok(value) => println!("Division result: {}", value),
        Err(e) => println!("Error: {}", e),
    }

    // Handling division by zero
    let zero_div = divide(5.0, 0.0);
    match zero_div {
        Ok(value) => println!("Result: {}", value),
        Err(e) => println!("Error: {}", e),
    }

    // Using unwrap (panics on Err)
    let good_result = divide(8.0, 2.0).unwrap();
    println!("Unwrapped result: {}", good_result);

    // Using expect (similar to unwrap but with custom message)
    let another_result = divide(9.0, 3.0).expect("Failed to divide");
    println!("Expected result: {}", another_result);

    // Handling file operations with Result
    match read_file_contents("example.txt") {
        Ok(contents) => println!("File contents:\n{}", contents),
        Err(e) => println!("Failed to read file: {}", e),
    }

    // Using if let for simple cases
    if let Ok(value) = divide(100.0, 25.0) {
        println!("Quick check result: {}", value);
    }
}
