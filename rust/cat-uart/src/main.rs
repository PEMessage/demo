use clap::Parser;
use std::io::{self, Read, Write};

#[derive(Parser, Debug)]
#[clap(author, version, about)]
struct Args {
    /// Serial device path (e.g., /dev/ttyUSB0)
    device: String,

    /// Baud rate (e.g., 9600, 115200)
    #[clap(short, long, default_value_t = 115200)]
    baud: u32,

    /// Stop bits (1 or 2)
    #[clap(long, default_value_t = 1)]
    stop_bits: u32,
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let args = Args::parse();

    // Validate stop bits
    let stop_bits = match args.stop_bits {
        1 => serialport::StopBits::One,
        2 => serialport::StopBits::Two,
        _ => {
            eprintln!("Error: Stop bits must be 1 or 2");
            std::process::exit(1);
        }
    };

    // Open serial port
    let mut port = serialport::new(&args.device, args.baud)
        .stop_bits(stop_bits)
        .open()
        .map_err(|e| format!("Failed to open {}: {}", args.device, e))?;

    // Buffer for reading data
    let mut buffer = [0u8; 1024];

    // Main read loop
    loop {
        match port.read(&mut buffer) {
            Ok(bytes_read) => {
                io::stdout().write_all(&buffer[..bytes_read])?;
                io::stdout().flush()?;
            }
            Err(ref e) if e.kind() == std::io::ErrorKind::TimedOut => {
                // Timeout is normal for serialport, just continue
                continue;
            }
            Err(e) => {
                eprintln!("Error reading from {}: {}", args.device, e);
                std::process::exit(1);
            }
        }
    }
}
