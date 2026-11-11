# Worklog: Multi-Board Restructuring & Olimex STM32-E407 Forth Port

- **Date**: November 11, 2026
- **Agent**: Antigravity Assistant
- **User**: Douglas Beattie Jr.
- **Platforms**:
  - Olimex STM32-E407 (STM32F407ZGT6, ARM Cortex-M4F @ 168 MHz, 1 MB Flash, 128 KB SRAM, J-Link JTAG)
  - STMicroelectronics Nucleo-G474RE (STM32G474RET6, ARM Cortex-M4 @ 170 MHz, 512 KB Flash, 128 KB SRAM, ST-Link V3E)
- **Branch**: `feature/multi-board-e407`
- **Host Tools**: PlatformIO 6.2.0, ARM GCC 7.2.1, Python 3.13.1 (pyenv virtualenv `forth-cortex-m4`), SCons 4.11.1, PySerial 3.5

---

## 1. Summary of Actions & Deliverables

1. **Git Feature Branch Creation**:
   - Created and checked out feature branch `feature/multi-board-e407`.

2. **Modular Architecture & Project Restructuring**:
   - Extracted target-independent Forth core VM into top-level `kernel/`:
     - `kernel/src/forth_kernel.S` (direct-threaded VM entry, reset sequence, and UINIT table)
     - `kernel/src/forth_m4_prim.inc` (machine-code primitives)
     - `kernel/src/forth_m4_d.inc` (model & architecture dependencies)
     - `kernel/src/forth_m4_high.inc` (high-level Forth definitions)
     - `kernel/include/forth_config.h` (shared Cortex-M4 128 KB SRAM layout)
     - `kernel/include/console_io.h` (C I/O function prototypes)
   - Updated `.include` directives in `forth_kernel.S` to resolve relative to `kernel/src/`.

3. **Nucleo-G474RE Board Subproject (`boards/nucleo_g474re/`)**:
   - Re-homed Nucleo G474RE specific C harness (`main.c`), LPUART1 driver (`console_io.c`), headers (`main.h`, `stm32g4xx_hal_conf.h`), and documentation.
   - Created standalone `boards/nucleo_g474re/platformio.ini` with ST-Link V3E and OpenOCD debug configuration.
   - Symlinked `forth_kernel.S` into board source tree.
   - Organized board-specific datasheets into `boards/nucleo_g474re/doc/datasheets/`.

4. **Olimex STM32-E407 Board Port (`boards/olimex_e407/`)**:
   - Implemented C startup harness (`boards/olimex_e407/src/main.c`):
     - `SystemClock_Config`: 12.0 MHz HSE crystal multiplied to 168 MHz via PLL (M=12, N=336, P=2, Q=7, Flash Latency 5 WS, I/D caches and prefetch buffer enabled).
     - Configured User Green LED on `PC13` (active low).
     - Invoked `console_init()` followed by `forth_cold()` to launch Forth.
   - Implemented register-level USART3 driver (`boards/olimex_e407/src/console_io.c`):
     - Configured `PD8` (TX) and `PD9` (RX) with Alternate Function 7 (`USART3`).
     - Implemented `forth_tx_char(uint8_t c)` polling `USART3->SR & USART_SR_TXE` and writing to `USART3->DR`.
     - Implemented `forth_rx_ready()` polling `USART3->SR & USART_SR_RXNE` with STM32F4 error flag clearance (reading `SR` then `DR`).
     - Implemented `forth_rx_char()` reading `USART3->DR`.
   - Created standalone `boards/olimex_e407/platformio.ini` with J-Link JTAG upload and debug server definitions.
   - Integrated `extra_upload.py` for automated J-Link Commander flashing script generation.
   - Copied full board schematics and STM32F407 datasheets into `boards/olimex_e407/doc/datasheets/`.

5. **Root Multi-Board Configuration (`platformio.ini`)**:
   - Configured root `platformio.ini` with `default_envs = nucleo_g474re, olimex_e407`.
   - Set `src_dir = kernel/src` and `include_dir = kernel/include`.
   - Configured target-specific `src_filter` and include search paths for both boards.
   - Removed obsolete root `src/` and `include/` directories.

---

## 2. Verification & Build Results

- **Nucleo G474RE**:
  - `pio run -d boards/nucleo_g474re`: **SUCCESS** (RAM: 192 bytes, Flash: 14596 bytes)
  - `pio run -e nucleo_g474re`: **SUCCESS** (RAM: 192 bytes, Flash: 14596 bytes)
- **Olimex STM32-E407**:
  - `pio run -d boards/olimex_e407`: **SUCCESS** (RAM: 116 bytes, Flash: 12992 bytes)
  - `pio run -e olimex_e407`: **SUCCESS** (RAM: 116 bytes, Flash: 12992 bytes)
- **Full Workspace Build**:
  - `pio run` (root): **SUCCESS** (both environments compiled and linked with 0 errors, 0 warnings).
