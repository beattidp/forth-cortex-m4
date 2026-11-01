# Worklog: ARM Cortex-M4 DTC Forth Implementation & Hardware Validation

- **Date**: November 1, 2026
- **Agent**: Antigravity Assistant
- **User**: Douglas Beattie Jr.
- **Platform**: STM32G474RE Nucleo (ARM Cortex-M4, 170 MHz, 512 KB Flash, 128 KB SRAM)
- **Host Tools**: PlatformIO, OpenOCD, ARM GCC 7.2.1, PySerial

---

## 1. Summary of Actions & Deliverables

1. **Hardware Bring-up & Serial Console**:
   - Initialized 170 MHz PLL system clock from 24 MHz HSE crystal on Nucleo board.
   - Configured LPUART1 on `PA2` (TX) and `PA3` (RX) at 115200 baud with polled error-clearing register driver (`src/console_io.c`).
   - Mapped status LED LD2 on `PA5`.

2. **DTC Virtual Machine Engine**:
   - Implemented 32-bit Direct-Threaded Code engine in Thumb-2 assembly (`src/forth_kernel.S`, `src/forth_m4_prim.inc`, `src/forth_m4_d.inc`, `src/forth_m4_high.inc`).
   - Mapped all VM registers (`tos`, `w`, `t1`, `t2`, `f_ip`, `psp`, `rsp`, `up`) to low registers `R0`–`R7`.

3. **Memory Isolation & Relocation**:
   - Relocated Forth RAM base to `0x20001000` to prevent collisions with C runtime `.data`, `.bss`, HAL variables (`uwTickFreq`, `hlpuart1`).
   - Implemented 40-byte SRAM trampoline table at `0x20001000` bridging the 384 MB gap between Flash and RAM.

4. **Root Cause Analysis & Bug Fixes**:
   - **`ROT` Stack Rotation Inversion**: Fixed `ROT` implementation where `x2` and `x1` were swapped, inadvertently producing `-ROT` (`x3 x1 x2`). This resolved parse failures in `/STRING` and `WORD`.
   - **`TYPE` Register Clobbering**: Preserved `u` loop counter in `t2` (`R3`) to prevent `ldrb r0, [t1]` from clobbering `tos` (`R0`).
   - **Formatted Numeric Output**: Implemented 64/32-bit division `UD/MOD` and machine-code `HOLD`, `<#`, and `#>`.
   - **Counted-String In-line Literals**: Updated `(S")`, `S"`, `."`, and `COLD` to use uniform counted-string representation.
   - **Control Flow**: Implemented `(do)`, `(loop)`, `(+loop)`, `I`, `J`, `UNLOOP`, `DO`, `LOOP`, `+LOOP`, `LEAVE`.
   - **Immediate Comments**: Added `.( ... )` and `\ ...`.

5. **Hardware Verification on Target**:
   - Tested interactive REPL over `/dev/ttyACM0` at 115200 baud.
   - Verified arithmetic, stack operations, variables, constants, recursion (`FACTORIAL`), control structures (`IF/ELSE/THEN`, `DO/LOOP`, `BEGIN/UNTIL`).
   - Executed `test/core_tests.fs`: 100% pass rate.
   - Executed `test/benchmarks.fs`: Sieve of Eratosthenes correctly computed 1899 primes up to 16384 in 0.5s.
