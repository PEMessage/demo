use clap::Parser;
use std::io::{self, Read, Write};

#[derive(Parser, Debug)]
#[clap(author, version, about)]
struct Args {
    /// Serial device path (e.g., /dev/ttyUSB0)
    #[clap(required_unless_present("list_ports"))]
    device: Option<String>,

    /// Baud rate (e.g., 9600, 115200)
    #[clap(short, long, default_value_t = 115200)]
    baud: u32,

    /// Stop bits (1 or 2)
    #[clap(long, default_value_t = 1)]
    stop_bits: u32,

    /// List available serial ports
    #[clap(short, long)]
    list_ports: bool,
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

    if args.list_ports {
        let ports = serialport::available_ports().expect("No ports found!");
        println!("Available ports:");
        for p in ports {
            println!("  {}", p.port_name);
        }
        return Ok(());
    }

    // Open serial port
    let device = args
        .device
        .expect("Device path is required when not listing ports");
    let mut port = serialport::new(&device, args.baud)
        .stop_bits(stop_bits)
        .open()
        .map_err(|e| format!("Failed to open {}: {}", device, e))?;

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
                eprintln!("Error reading from {}: {}", device, e);
                std::process::exit(1);
            }
        }
    }
}
