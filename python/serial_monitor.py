import serial
import threading

# ----- CONFIGURE PORT AND BAUD HERE -----
PORT = "COM4"  # Change to your Arduino port
BAUD = 9600


def reader(ser):
    """Continuously read from serial and print to console"""
    try:
        while True:
            line = ser.readline()
            if line:
                try:
                    print(line.decode("utf-8").rstrip())
                except UnicodeDecodeError:
                    print(repr(line))
    except Exception as e:
        print("Serial read error:", e)


def main():
    print("Opening serial port...")
    ser = serial.Serial(PORT, BAUD, timeout=0.1)
    print(
        f"Connected to {PORT} at {BAUD} baud.\nType commands and press ENTER to send."
    )

    # Start reader thread
    t = threading.Thread(target=reader, args=(ser,), daemon=True)
    t.start()

    try:
        while True:
            cmd = input("> ")
            if cmd.strip().lower() in {"exit", "quit"}:
                print("Exiting.")
                break
            ser.write((cmd + "\n").encode("utf-8"))
    except KeyboardInterrupt:
        print("\nExiting.")
    finally:
        ser.close()


if __name__ == "__main__":
    main()
