"""
Automated Hardware Test Suite for CamelForth DTC on Olimex STM32-E407.

Communicates with the target MCU over the serial interface (USART3 on PD8/PD9)
via /dev/ttyUSB1 at 115200 baud, 8N1.
"""

import time
import serial


class ForthHardwareTester:
    """
    Test harness for interactive CamelForth execution over a serial connection.
    """

    def __init__(self, port: str = "/dev/ttyUSB1", baudrate: int = 115200, timeout: float = 1.0):
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.ser = None

    def connect(self) -> None:
        """Establish serial connection to target."""
        self.ser = serial.Serial(self.port, self.baudrate, timeout=self.timeout)
        time.sleep(0.1)
        self.ser.reset_input_buffer()

    def disconnect(self) -> None:
        """Close serial connection."""
        if self.ser and self.ser.is_open:
            self.ser.close()

    def send_line(self, line: str, wait_sec: float = 0.2) -> str:
        """
        Send a line of Forth code and return the received response string.
        """
        self.ser.write((line + "\r\n").encode("utf-8"))
        time.sleep(wait_sec)
        data = b""
        while self.ser.in_waiting:
            data += self.ser.read(self.ser.in_waiting)
            time.sleep(0.05)
        return data.decode("utf-8", errors="replace")


def run_tests() -> bool:
    """Run interactive test cases on STM32-E407 hardware."""
    tester = ForthHardwareTester("/dev/ttyUSB1", 115200)
    print("=== Connecting to Olimex STM32-E407 over /dev/ttyUSB1 @ 115200 ===")
    tester.connect()
    all_passed = True

    try:
        # 1. Warm-up / Synchronize
        resp = tester.send_line("")
        print(f"[Sync] Prompt response: {repr(resp.strip())}")
        if "ok" not in resp:
            print("FAILED to get 'ok' prompt!")
            return False

        # 2. Arithmetic & Stack
        print("\n[Test 1] Arithmetic & Stack operations:")
        resp = tester.send_line("10 25 + . CR")
        print(f"  Input: '10 25 + . CR' -> Response: {repr(resp.strip())}")
        assert "35" in resp and "ok" in resp, "Test 1 failed"
        print("  PASS")

        # 3. Stack Rotation (ROT)
        print("\n[Test 2] Stack Rotation (ROT):")
        resp = tester.send_line("1 2 3 ROT . . . CR")
        print(f"  Input: '1 2 3 ROT . . . CR' -> Response: {repr(resp.strip())}")
        assert "1 3 2" in resp and "ok" in resp, "Test 2 failed"
        print("  PASS")

        # 4. RAM Word Compilation (: SQUARE DUP * ;)
        print("\n[Test 3] RAM Word Compilation:")
        tester.send_line(": SQUARE DUP * ;")
        resp = tester.send_line("7 SQUARE . CR")
        print(f"  Input: '7 SQUARE . CR' -> Response: {repr(resp.strip())}")
        assert "49" in resp and "ok" in resp, "Test 3 failed"
        print("  PASS")

        # 5. Variables and Constants
        print("\n[Test 4] Variables and Constants in RAM:")
        tester.send_line("VARIABLE MYVAR")
        tester.send_line("9999 MYVAR !")
        resp = tester.send_line("MYVAR @ . CR")
        print(f"  Input: 'MYVAR @ . CR' -> Response: {repr(resp.strip())}")
        assert "9999" in resp and "ok" in resp, "Test 4 (Variable) failed"

        tester.send_line("1234 CONSTANT MYCONST")
        resp = tester.send_line("MYCONST . CR")
        print(f"  Input: 'MYCONST . CR' -> Response: {repr(resp.strip())}")
        assert "1234" in resp and "ok" in resp, "Test 4 (Constant) failed"
        print("  PASS")

        # 6. Control Flow (DO ... LOOP)
        print("\n[Test 5] DO ... LOOP summation:")
        tester.send_line(": SUM10 0 11 1 DO I + LOOP ;")
        resp = tester.send_line("SUM10 . CR")
        print(f"  Input: 'SUM10 . CR' -> Response: {repr(resp.strip())}")
        assert "55" in resp and "ok" in resp, "Test 5 failed"
        print("  PASS")

        # 7. Sieve of Eratosthenes Benchmark
        print("\n[Test 6] Sieve of Eratosthenes Benchmark:")
        tester.send_line("8190 CONSTANT SIZE")
        tester.send_line("VARIABLE FLAGS SIZE ALLOT")
        sieve_code = (
            ": DO-PRIME "
            "FLAGS SIZE 1 FILL 0 "
            "SIZE 0 DO "
            "FLAGS I + C@ IF "
            "I 2* 3 + DUP I + "
            "BEGIN DUP SIZE < WHILE 0 OVER FLAGS + C! OVER + REPEAT "
            "DROP DROP 1+ "
            "THEN LOOP . .\" primes found\" CR ;"
        )
        tester.send_line(sieve_code, wait_sec=0.3)
        start_time = time.time()
        resp = tester.send_line("DO-PRIME", wait_sec=0.8)
        elapsed = time.time() - start_time
        print(f"  Input: 'DO-PRIME' -> Response: {repr(resp.strip())}")
        print(f"  Benchmark elapsed time: {elapsed:.3f} s")
        assert "1899 primes found" in resp, "Test 6 (Sieve) failed"
        print("  PASS (1899 primes calculated correctly)")

        print("\n=======================================================")
        print(" ALL HARDWARE TESTS PASSED ON OLIMEX STM32-E407!")
        print("=======================================================")

    except AssertionError as err:
        print(f"\nTEST ASSERTION ERROR: {err}")
        all_passed = False
    except Exception as exc:
        print(f"\nUNEXPECTED EXCEPTION: {exc}")
        all_passed = False
    finally:
        tester.disconnect()

    return all_passed


if __name__ == "__main__":
    import sys
    success = run_tests()
    sys.exit(0 if success else 1)
