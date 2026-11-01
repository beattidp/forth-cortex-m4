# Walkthrough: Direct-Threaded Forth Kernel for ARM Cortex-M4 (STM32G474)

This document describes the design, implementation, and hardware verification of the 32-bit direct-threaded Forth kernel ported to the ARM Cortex-M4 (STM32G474RE Nucleo), based on Brad Rodriguez's CamelForth model and the Z382 32-bit CamelForth implementation.

---

## Architecture and Design Decisions

### 1. Direct-Threaded Code (DTC) Model
In this DTC model, an Execution Token (**XT**) is the Code Field Address with bit 0 set (`CFA | 1`) to signal Thumb mode to the Cortex-M4 processor.

- **For Primitive Words (`CODE`)**:
  Machine code instructions start directly at the CFA. The word terminates with the in-line `next` macro.
- **For High-Level Words (`docolon`)**:
  The CFA contains a 4-byte Thumb `BL` instruction:
  - Flash ROM words: `BL docolon` (branches to `0x08000284`).
  - RAM words: `BL ram_docolon` (branches to SRAM trampoline `0x20001000`).
  When executed, `BL` branches to the runtime routine and sets `LR` to `(CFA + 4) | 1`—the exact Parameter Field Address (PFA) containing the start of the threaded execution tokens.

```assembly
/* Inner Interpreter Macro (6 bytes) */
.macro next
    ldr.w w, [f_ip], #4
    bx    w
.endm
```

### 2. Bridging the 384 MB Flash-to-RAM Distance
On the STM32G474:
- **Flash ROM** begins at `0x08000000` (512 KB)
- **SRAM** begins at `0x20000000` (128 KB)

Because the distance is 384 MB and the ARM Thumb-2 `BL` instruction has an immediate branch range of $\pm 16\text{ MB}$, a word compiled dynamically into RAM cannot execute `BL docolon` directly into Flash.

**Solution**:
A 40-byte **RAM Trampoline Table** is placed in SRAM at `0x20001000` (safely separated from the C runtime `.bss` at `0x20000000`–`0x20000FFF`):
- `0x20001000`: `ram_docolon` (`ldr.w pc, [pc]; .long docolon`)
- `0x20001008`: `ram_docon` (`ldr.w pc, [pc]; .long docon`)
- `0x20001010`: `ram_dovar` (`ldr.w pc, [pc]; .long dovar`)
- `0x20001018`: `ram_douser` (`ldr.w pc, [pc]; .long douser`)
- `0x20001020`: `ram_dodoes` (`ldr.w pc, [pc]; .long dodoes`)

When a word in RAM is compiled:
`,CF` and `!CF` encode `BL ram_docolon` (distance $< 128\text{ KB}$, well within range).
When executed:
1. `BL ram_docolon` updates `LR` to point to the PFA in RAM.
2. `ram_docolon` performs an indirect load into `PC` targeting `docolon` in Flash without altering `LR`.
3. `docolon` in Flash saves `f_ip` to `RSP`, sets `f_ip = LR & ~1`, and begins threading the RAM definition.
4. **Result**: Words in both Flash and RAM have an identical 4-byte CFA and uniform `>BODY` (`3 +`).

### 3. Register Allocation
All virtual machine registers are mapped to ARM low registers (`R0`–`R7`) for maximum 16-bit Thumb instruction density:
- `R0` (`tos`): Top of Data Stack
- `R1` (`w`): Working register / CFA / XT
- `R2` (`t1`): Temporary scratch 1
- `R3` (`t2`): Temporary scratch 2
- `R4` (`f_ip`): Forth Instruction / Interpreter Pointer
- `R5` (`psp`): Parameter Stack Pointer (grows downward)
- `R6` (`rsp`): Return Stack Pointer (grows downward)
- `R7` (`up`): User Area Pointer
- Hardware `SP` (`R13`) remains dedicated to Cortex-M exception / C runtime stack.

---

## Memory Map

```
Flash Memory (512 KB, 0x08000000 - 0x0807FFFF)
+-------------------------------------------------------------+
| 0x08000000: ARM Vector Table & C Startup / Reset Handler    |
| 0x08000218: forth_cold (Boot entry point)                   |
| 0x08000284: docolon, docon, dovar, douser, dodoes runtimes  |
| 0x080002E4: ram_trampolines_src (40-byte flash image)       |
| 0x08000318: Forth Primitives (EXIT, DUP, +, @, CMOVE, etc.)|
| 0x08000C00: Architecture Words (ALIGN, !CF, ,CF, !COLON)    |
| 0x08001000: High-Level Forth Words (WORD, FIND, QUIT, etc.) |
| 0x080023B0: ROM_LATEST (Points to NFA of SAVEU)             |
| 0x080023B8: uinit_data (Initial user area image)            |
+-------------------------------------------------------------+

SRAM (128 KB, 0x20000000 - 0x2001FFFF)
+-------------------------------------------------------------+
| 0x20000000 - 0x20000FFF: C runtime (.data, .bss, HAL) (4 KB)|
| 0x20001000 - 0x2000103F: RAM Trampolines (40 bytes)         |
| 0x20001040 - 0x200010FF: User Area (UP = 0x20001040, 192 B) |
| 0x20001100 - 0x200011FF: Terminal Input Buffer (TIB, 256 B) |
| 0x20001200 - 0x200015FF: Numeric PAD / HOLD Area (1 KB)     |
| 0x20001600 - 0x200017FF: Leave Stack (LP, 512 B)            |
| 0x20001800             : Parameter Stack Top (PSP, down)    |
| 0x20002000             : Return Stack Top (RSP, down)       |
| 0x20002000 - 0x2001DFFF: RAM Dictionary Space (~112 KB free)|
| 0x2001E000 - 0x2001FFFF: C MSP Hardware Stack (8 KB)        |
+-------------------------------------------------------------+
```

---

## Key Debugging Discoveries & Fixes

1. **`ROT` Stack Rotation Inversion**:
   - `ROT ( x1 x2 x3 -- x2 x3 x1 )` originally put `x2` into `tos`, `x1` into `[psp]`, and `x3` into `[psp, #4]`, which inverted the rotation to `-ROT` (`x3 x1 x2`).
   - This broke `/STRING`, `WORD` parse calculations, `UD/MOD`, and numeric conversions.
   - Fixed by storing `x2` at `[psp, #4]`, `x3` at `[psp]`, and `x1` into `tos`.

2. **`TYPE` Loop Register Conflict**:
   - In `TYPE`, `ldrb r0, [t1], #1` loaded the character byte directly into `r0`, overwriting `tos` which was holding the remaining character count.
   - Fixed by retaining the loop count in `t2` (`R3`), properly isolating `r0` for character transmission.

3. **ANSI Formatted Numeric Output & 64/32-bit Division**:
   - Fixed `#` (`NUM`) which previously called `UM/MOD` (returning 2 stack items instead of 3).
   - Implemented `UD/MOD ( ud1 u2 -- u3 ud4 )` performing double-precision 64/32-bit division into 32-bit remainder and 64-bit quotient.
   - Implemented `HOLD`, `<#`, and `#>` directly in machine code for performance and immunity to stack misalignment.

4. **Counted-String In-line Literals**:
   - Standardized `(S")`, `S"`, `."`, and `COLD` to use uniform counted-string representation (`.byte len` followed by ASCII characters aligned to 4 bytes).

5. **Loop and Control Flow Engine**:
   - Added standard Thumb-2 implementations for `(do)`, `(loop)`, `(+loop)`, `I`, `J`, `UNLOOP`, `DO`, `LOOP`, `+LOOP`, and `LEAVE`.
   - Added immediate load-time comment words `.( ... )` and `\ ...`.

---

## Hardware Verification Results

Hardware verification was performed on the STM32G474RE Nucleo board over `/dev/ttyACM0` at 115200 baud:

### 1. Interactive REPL & Arithmetic
```forth
1 2 + .
3  ok
10 20 * .
200  ok
-5 12 + .
7  ok
255 HEX . DECIMAL
FF  ok
```

### 2. RAM Word Compilation & Trampoline Execution
```forth
: SQUARE DUP * ;
 ok
7 SQUARE .
49  ok
: CUBE DUP SQUARE * ;
 ok
3 CUBE .
27  ok
```

### 3. Recursion & Control Flow
```forth
: FACTORIAL DUP 2 < IF DROP 1 ELSE DUP 1- RECURSE * THEN ;
 ok
5 FACTORIAL .
120  ok
: TESTLOOP 5 0 DO I . LOOP ;
 ok
TESTLOOP
0 1 2 3 4  ok
```

### 4. Automated Core Tests (`test/core_tests.fs`)
- Stack operations (`ROT`, `SWAP`, `DUP`): PASS
- Arithmetic & logic (`+`, `-`, `*`, `/MOD`, `ABS`): PASS
- Dynamic compilation of variables (`MYVAR`) and constants (`MYCONST`): PASS
- Loop control flow (`BEGIN ... UNTIL` countdown): PASS
- Overall Result: **100% Pass**

### 5. Benchmark: Sieve of Eratosthenes (`test/benchmarks.fs`)
- 8190-element array benchmark computing primes up to 16384:
- Result: **1899 primes found** in 0.5s.
- Validates memory safety across `FILL`, `ALLOT`, nested `DO/LOOP`, and array operations.
