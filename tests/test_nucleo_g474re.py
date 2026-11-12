"""
Automated Hardware Test Suite for CamelForth DTC on ST Nucleo-G474RE.

Communicates with the target MCU over the ST-LINK Virtual COM Port
(LPUART1 on PA2/PA3) via /dev/ttyACM0 at 115200 baud, 8N1.
"""

import time
import serial


class ForthHardwareTester:
    """
    Test harness for interactive CamelForth execution over a serial connection.
    """

    def __init__(self, port: str = "/dev/ttyACM0", baudrate: int = 115200, timeout: float = 1.0):
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
    """Run interactive test cases on Nucleo-G474RE hardware."""
    tester = ForthHardwareTester("/dev/ttyACM0", 115200)
    print("=== Connecting to ST Nucleo-G474RE over /dev/ttyACM0 @ 115200 ===")
    tester.connect()
    all_passed = True

    try:
        # 1. Warm-up / Synchronize
        resp = tester.send_line("")
        print(f"[Sync] Prompt response: {repr(resp.strip())}")
        if "ok" not in resp:
            print("FAILED to get 'ok' prompt!")
            return False

        # 2. COLD Banner Verification (Unique MCU Identification)
        print("\n[Test Banner] Target MCU identification in COLD:")
        resp = tester.send_line("COLD", wait_sec=0.3)
        print(f"  Input: 'COLD' -> Response: {repr(resp.strip())}")
        assert "STM32G474" in resp, "Expected 'STM32G474' in COLD banner!"
        assert "STM32F407" not in resp, "Found unexpected 'STM32F407' in G474 banner!"
        print("  PASS (Identified as STM32G474)")

        # 3. Arithmetic & Stack
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

        # 4. Word Definition in RAM
        print("\n[Test 3] User Word Definition (: SQUARE DUP * ;):")
        resp = tester.send_line(": SQUARE DUP * ;")
        print(f"  Define SQUARE -> Response: {repr(resp.strip())}")
        assert "ok" in resp, "Test 3 definition failed"

        resp = tester.send_line("15 SQUARE . CR")
        print(f"  Input: '15 SQUARE . CR' -> Response: {repr(resp.strip())}")
        assert "225" in resp and "ok" in resp, "Test 3 execution failed"
        print("  PASS")

        # 5. DO ... LOOP Control Flow
        print("\n[Test 4] DO ... LOOP Control Structure:")
        resp = tester.send_line(": LOOPTEST 5 0 DO I . LOOP ;")
        print(f"  Define LOOPTEST -> Response: {repr(resp.strip())}")
        assert "ok" in resp, "Test 4 definition failed"

        resp = tester.send_line("LOOPTEST CR")
        print(f"  Input: 'LOOPTEST CR' -> Response: {repr(resp.strip())}")
        assert "0 1 2 3 4" in resp and "ok" in resp, "Test 4 execution failed"
        print("  PASS")

        # 6. Sieve of Eratosthenes Benchmark
        print("\n[Test 5] Sieve of Eratosthenes Benchmark (8190 flags):")
        tester.send_line("8190 CONSTANT SIZE")
        tester.send_line("VARIABLE FLAGS SIZE ALLOT")
        sieve_def = (
            ": DO-PRIME "
            "FLAGS SIZE 1 FILL 0 "
            "SIZE 0 DO "
            "FLAGS I + C@ IF "
            "I 2* 3 + DUP I + "
            "BEGIN DUP SIZE < WHILE 0 OVER FLAGS + C! OVER + REPEAT "
            "DROP DROP 1+ "
            "THEN LOOP . .\" primes found\" CR ;"
        )
        resp = tester.send_line(sieve_def, wait_sec=0.3)
        print(f"  Define DO-PRIME -> Response: {repr(resp.strip())}")
        assert "ok" in resp, "Sieve definition failed"

        t_start = time.time()
        resp = tester.send_line("DO-PRIME", wait_sec=1.0)
        t_elapsed = time.time() - t_start
        print(f"  Run DO-PRIME -> Response: {repr(resp.strip())}")
        print(f"  Execution time: {t_elapsed:.3f} s")
        assert "1899 primes found" in resp and "ok" in resp, "Sieve benchmark failed"
        print("  PASS")

    except AssertionError as err:
        print(f"\nAssertion Failed: {err}")
        all_passed = False
    except Exception as exc:
        print(f"\nUnexpected error during tests: {exc}")
        all_passed = False
    finally:
        tester.disconnect()

    if all_passed:
        print("\n>>> ALL TESTS PASSED SUCCESSFULLY ON ST NUCLEO-G474RE! <<<")
    else:
        print("\n>>> TEST FAILURES ENCOUNTERED ON ST NUCLEO-G474RE <<<")
    return all_passed


if __name__ == "__main__":
    success = run_tests()
    exit(0 if success else 1)
