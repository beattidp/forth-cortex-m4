# Direct-Threaded Code (DTC) VM Architecture on ARM Cortex-M4

## Overview
This document specifies the Direct-Threaded Code (DTC) virtual machine architecture implemented for the ARM Cortex-M4 (STM32G474RET6). The implementation is based on Brad Rodriguez's CamelForth model, adapted specifically to utilize ARMv7E-M / Thumb-2 architectural features.

---

## 1. Execution Token (XT) and Code Field Address (CFA)

On the ARM Cortex-M4, all code executes in Thumb state. In this DTC implementation:
- An **XT** is the Code Field Address with bit 0 set (`CFA | 1`) to signal Thumb mode to the processor.
- A **CFA** is a 4-byte cell:
  - **Primitive Words (`CODE`)**: Thumb-2 machine code instructions begin directly at the CFA. The word terminates with the in-line `next` macro.
  - **High-Level Words (`docolon`)**: The CFA contains a single 32-bit Thumb `BL` instruction:
    - In Flash ROM: `BL docolon` (branches to `0x08000284`).
    - In RAM: `BL ram_docolon` (branches to SRAM trampoline `0x20001000`).
  - When executed, `BL` branches to the runtime routine and sets `LR` to `(CFA + 4) | 1`—the exact Parameter Field Address (PFA) containing the start of the threaded execution tokens.

---

## 2. Inner Interpreter (`next`)

The inner interpreter is a 2-instruction (6-byte) macro expanded at the end of every machine code primitive:
```assembly
.macro next
    ldr.w w, [f_ip], #4
    bx    w
.endm
```
- `f_ip` (`R4`) holds the Forth instruction pointer, pointing to the next 32-bit XT in the threaded word list.
- Post-indexed auto-increment advances `f_ip` by 4 bytes.
- `w` (`R1`) receives the XT and executes via `bx w`.

---

## 3. Register Allocations

All virtual machine registers are mapped to ARM low registers (`R0`–`R7`) for maximum 16-bit Thumb instruction density:
| Register | Forth VM Name | Role |
|:---------|:--------------|:-----|
| `R0` | `tos` | Top of Data Stack (cached in register) |
| `R1` | `w` | Working register / CFA / XT |
| `R2` | `t1` | Scratch temporary 1 |
| `R3` | `t2` | Scratch temporary 2 |
| `R4` | `f_ip` | Forth Instruction Pointer |
| `R5` | `psp` | Parameter Stack Pointer (grows downward) |
| `R6` | `rsp` | Return Stack Pointer (grows downward) |
| `R7` | `up` | User Area Pointer (points to `0x20001040`) |
| `R8`–`R12` | scratch / temp | General scratch registers |
| `R13` (`SP`) | hardware MSP | ARM Main Stack Pointer (hardware exceptions & C runtime) |
| `R14` (`LR`) | hardware LR | ARM Link Register (used by `BL` and subroutine calls) |
| `R15` (`PC`) | hardware PC | Program Counter |

---

## 4. Defining Word Runtimes

- **`docolon`**:
  ```assembly
  docolon:
      str f_ip, [rsp, #-4]!
      bic f_ip, lr, #1
      ldr w, [f_ip], #4
      bx  w
  ```
- **`docon`**:
  ```assembly
  docon:
      str tos, [psp, #-4]!
      bic lr, lr, #1
      ldr tos, [lr]
      ldr w, [f_ip], #4
      bx  w
  ```
- **`dovar` / `docreate`**:
  ```assembly
  dovar:
  docreate:
      str tos, [psp, #-4]!
      bic tos, lr, #1
      ldr w, [f_ip], #4
      bx  w
  ```
- **`douser`**:
  ```assembly
  douser:
      str tos, [psp, #-4]!
      bic lr, lr, #1
      ldr t1, [lr]
      add tos, up, t1
      ldr w, [f_ip], #4
      bx  w
  ```
- **`dodoes`**:
  ```assembly
  dodoes:
      str f_ip, [rsp, #-4]!
      bic f_ip, lr, #1
      pop {t1}
      bic t1, t1, #1
      str tos, [psp, #-4]!
      mov tos, t1
      ldr w, [f_ip], #4
      bx  w
  ```
