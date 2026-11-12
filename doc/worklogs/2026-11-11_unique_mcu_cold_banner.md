# Worklog: Unique Target MCU Name in COLD Banner

- **Date**: 2026-11-11
- **Time**: 19:44 UTC
- **Author**: Antigravity Assistant (AI Pair Programmer)
- **User**: beattidp
- **Branch**: `feature/multi-board-e407`

---

## 1. Objective
Ensure the Forth `COLD` boot banner displays a unique target processor name corresponding to each physical hardware platform:
- `STM32G474` on ST Nucleo-G474RE
- `STM32F407` on Olimex STM32-E407

Previously, the string was hardcoded to `CamelForth Cortex-M4 v1.0 (STM32G474 DTC 32-bit)` across all builds in `kernel/src/forth_m4_high.inc`.

---

## 2. Implementation Details

1. **Preprocessor Definition in `kernel/src/forth_kernel.S`**:
   Added a configurable `TARGET_MCU_NAME` symbol before including `forth_m4_high.inc`:
   ```assembly
   #ifndef TARGET_MCU_NAME
   #if defined(STM32F407xx)
   #define TARGET_MCU_NAME "STM32F407"
   #elif defined(STM32G474xx)
   #define TARGET_MCU_NAME "STM32G474"
   #else
   #define TARGET_MCU_NAME "Cortex-M4"
   #endif
   #endif
   ```
   Converted `.include "forth_m4_high.inc"` to `#include "forth_m4_high.inc"` to enable C preprocessor expansion within high-level assembly definitions.

2. **Dynamic Assembly Banner in `kernel/src/forth_m4_high.inc`**:
   Updated `COLD` word to concatenate `TARGET_MCU_NAME` and dynamically calculate the exact byte length using assembler label differences:
   ```assembly
   /* COLD ( -- ) Cold start */
       head COLD, "COLD", docolon
           .long CR_WORD
           .long XSQUOTE
           .byte (banner_str_end - banner_str_start)
   banner_str_start:
           .ascii "CamelForth Cortex-M4 v1.0 ("
           .ascii TARGET_MCU_NAME
           .ascii " DTC 32-bit)"
   banner_str_end:
           .balign 4
           .long TYPE, CR_WORD
   ```

3. **PlatformIO Configuration Polish**:
   - Upgraded deprecated `src_filter` to `build_src_filter` in root `platformio.ini`.
   - Updated OpenOCD ST-LINK configuration to `reset_config srst_only connect_assert_srst` for reliable programming across all power states.

4. **Automated Test Updates**:
   - Added COLD banner assertions to `tests/test_nucleo_g474re.py` and `tests/test_olimex_e407.py`.

---

## 3. Hardware Verification
Both targets flashed and verified live:
- **Olimex STM32-E407** (`/dev/ttyUSB1`): Output: `CamelForth Cortex-M4 v1.0 (STM32F407 DTC 32-bit)`
- **ST Nucleo-G474RE** (`/dev/ttyACM0`): Output: `CamelForth Cortex-M4 v1.0 (STM32G474 DTC 32-bit)`
- Interactive arithmetic, word definitions, and loops validated on both devices.
