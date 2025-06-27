use clap::Parser;

/// Simple program to greet a person
#[derive(Parser, Debug)]
#[command(author, version, about, long_about = None)]
struct Args {
    /// Name of the person to greet
    #[arg(short, long)]
    name: String,

    /// Number of times to greet
    #[arg(short, long, default_value_t = 1)]
    count: u8,
}

trait Printable {
    fn print(&self);
}

// Implement the trait for Args
impl Printable for Args {
    fn print(&self) {
        println!("Printing Args:");
        println!("- Name: {}", self.name);
        println!("- Count: {}", self.count);
    }
}

#[derive(Parser, Debug)]
enum Subcmds {
    Subcmd1(Args),
    Subcmd2(Args),
}

fn main() {
    // let args = Args::parse();
    let subcmds = Subcmds::parse();
    match subcmds {
        Subcmds::Subcmd1(cmd) => cmd.print(),
        Subcmds::Subcmd2(cmd) => cmd.print(),
    }
}
