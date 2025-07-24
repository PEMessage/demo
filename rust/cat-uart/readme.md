# Test(only pty)

# Terminal 1: Create a test environment
socat -d -d pty,raw,echo=0 pty,raw,echo=0

# Terminal 2: Run your program
cargo run -- /dev/pts/5 --baud 9600

# Terminal 3: Send test data
echo "Hello World" > /dev/pts/6
