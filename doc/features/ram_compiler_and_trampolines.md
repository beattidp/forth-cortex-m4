# RAM Compiler & Trampolines for ARM Cortex-M4

## Problem Statement
On the STM32G474 (and other Cortex-M devices):
- **Flash ROM** is mapped to `0x08000000`–`0x0807FFFF` (512 KB).
- **SRAM** is mapped to `0x20000000`–`0x2001FFFF` (128 KB).

The distance between Flash ROM and RAM is 384 MB (`0x20000000 - 0x08000000 = 0x18000000`).
The ARM Thumb-2 `BL` (Branch with Link) instruction has a maximum signed displacement of $\pm 16\text{ MB}$ ($\pm 0x01000000$).
Consequently, a newly compiled Forth word in RAM cannot directly encode a `BL docolon` into Flash.

## Architectural Solution: SRAM Trampolines

A 40-byte trampoline table is copied to SRAM during `forth_cold` startup:
- Located at `0x20001000` (safe from C runtime `.bss` at `0x20000000`–`0x20000FFF`):

```assembly
/* RAM Trampoline Table (0x20001000 - 0x20001027) */
0x20001000 : ram_docolon  -> ldr.w pc, [pc]; .long docolon
0x20001008 : ram_docon    -> ldr.w pc, [pc]; .long docon
0x20001010 : ram_dovar    -> ldr.w pc, [pc]; .long dovar
0x20001018 : ram_douser   -> ldr.w pc, [pc]; .long douser
0x20001020 : ram_dodoes   -> ldr.w pc, [pc]; .long dodoes
```

### Trampoline Operation During Execution
When a user-compiled word in RAM executes:
1. The word's CFA contains a 4-byte 32-bit Thumb `BL ram_docolon` instruction.
2. The distance between the RAM word (e.g. `0x20002000`) and the trampoline (`0x20001000`) is only a few kilobytes, easily reachable with a standard 16-bit displacement.
3. The `BL` instruction automatically sets the ARM Link Register `LR` to `(CFA + 4) | 1`—the exact PFA containing the threaded tokens in RAM!
4. The trampoline `ldr.w pc, [pc]` performs an indirect jump into Flash `docolon` *without modifying `LR`*.
5. `docolon` in Flash runs:
   ```assembly
   str f_ip, [rsp, #-4]!
   bic f_ip, lr, #1
   ldr w, [f_ip], #4
   bx  w
   ```
   Saving `f_ip` to the Forth return stack, loading `f_ip` from `LR`, and threading through the RAM definition.

### Uniform Word Layout
Because of this design:
- Both Flash ROM words and RAM user words have an identical 4-byte CFA.
- `>BODY` is consistently `adds tos, tos, #3; bics tos, tos, #1; next` across all words regardless of whether they reside in ROM or RAM.
- Header fields (`LINK`, `FLAGS`, `NAME`, `CFA`, `PFA`) are 100% standard across both ROM and RAM.

## Dynamic Compilation (`!CF` and `,CF`)
In `src/forth_m4_d.inc`:
- `!CF ( target_adrs cfa -- )`: Compares `target_adrs` against Flash action routines and selects the corresponding SRAM trampoline. It then calculates the signed relative branch offset:
  $$\text{offset} = \text{trampoline\_addr} - (\text{cfa} + 4)$$
  and generates the Thumb-2 `BL` opcode:
  $$H_1 = 0xF000 \mid ((offset \gg 12) \ \& \ 0x07FF)$$
  $$H_2 = 0xF800 \mid ((offset \gg 1) \ \& \ 0x07FF)$$
  writing the 32-bit halfword pair into `cfa`.
- `,CF ( target_adrs -- )`: Allocates 4 bytes at `HERE` and calls `!CF`.
- `!COLON`: Sets up a colon definition header in RAM, calling `,CF` targeting `docolon`.
