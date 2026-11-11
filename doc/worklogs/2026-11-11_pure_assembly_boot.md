# Worklog: Pure Assembly Boot (Zero-C Bootstrap) for Olimex STM32-E407 and ST Nucleo-G474RE

- **Date**: 2026-11-11
- **Time**: 01:57 UTC
- **Author**: Antigravity Assistant (AI Pair Programmer)
- **User**: beattidp
- **Branch**: `feature/multi-board-e407`

---

## 1. Objective
Develop, flash, and validate pure assembly boot solutions (zero-C bootstrap) for both target boards:
1. **Olimex STM32-E407** (STM32F407ZGT6, 168 MHz SYSCLK, USART3 @ 115200, J-Link JTAG)
2. **ST Nucleo-G474RE** (STM32G474RET6, 170 MHz SYSCLK Boost, LPUART1 @ 115200, ST-LINK V3)

Ensure that all C wrappers (`main.c`, `console_io.c`, HAL drivers, CMSIS C startup) are removed from the active build path, achieving an ultra-compact (~9.5 KB) pure assembly Forth image running directly from reset.

---

## 2. Summary of Work Executed

### A. STM32-E407 Pure Assembly Implementation (`boot_olimex_e407.S`)
- Implemented `.section .isr_vector, "a", %progbits` with MSP (`0x20020000`), Thumb `.thumb_func Reset_Handler`, and exception vectors.
- FPU CP10/CP11 full-access activation via `SCB_CPACR`.
- 12 MHz HSE clock oscillator enable and stabilization.
- Flash latency (5 WS) + prefetch + I/D caches (`0x00000705`).
- Bus dividers: AHB /1, APB1 /4 (42 MHz), APB2 /2 (84 MHz).
- PLL configuration to 168 MHz (`M=12, N=336, P=2, Q=7`).
- Gated peripheral clocks: GPIOC, GPIOD, USART3.
- User LED PC13 configuration (Output, driven LOW).
- USART3 PD8/PD9 AF7 pinmux and 115200 baud rate (`BRR = 0x016C`).
- Native assembly console I/O routines: `forth_tx_char`, `forth_rx_ready`, `forth_rx_char`.
- Branch `b forth_cold`.
- Archived C harness files to `boards/olimex_e407/src/c_harness/`.
- Updated `boards/olimex_e407/platformio.ini` to filter out C files and link only `boot_olimex_e407.S` and `forth_kernel.S`.
- Flashed via J-Link JTAG and verified live interactively on `/dev/ttyUSB1`.

### B. Nucleo-G474RE Pure Assembly Implementation (`boot_nucleo_g474re.S`)
- Implemented `.section .isr_vector, "a", %progbits` with MSP (`0x20020000`), Thumb `.thumb_func Reset_Handler`, and exception vectors.
- FPU CP10/CP11 full-access activation via `SCB_CPACR`.
- Power interface clock enable (`RCC_APB1ENR1` bit 28).
- Range 1 Boost mode activation: cleared `R1MODE` (bit 8) in `PWR_CR5` (offset `0x80`), configured Range 1 in `PWR_CR1`, and polled `VOSF` bit 10 in `PWR_SR2`.
- 16 MHz HSI16 internal oscillator enable and stabilization.
- Flash latency (4 WS @ 170 MHz) + prefetch + I/D caches (`0x00000704`).
- PLL configuration to 170 MHz (`M=4, N=85, R=2`).
- Gated peripheral clocks: GPIOA (AHB2), LPUART1 (APB1-2).
- User Green LED LD2 on PA5 configured and driven HIGH.
- LPUART1 PA2/PA3 AF12 pinmux with PA3 pull-up.
- LPUART1 115200 baud configuration (`BRR = 0x0005C3B2` @ 170 MHz).
- Native assembly console I/O routines: `forth_tx_char`, `forth_rx_ready`, `forth_rx_char`.
- Branch `b forth_cold`.
- Archived C harness files to `boards/nucleo_g474re/src/c_harness/`.
- Updated `boards/nucleo_g474re/platformio.ini` and root `platformio.ini` to build pure assembly.
- Flashed via ST-LINK to physical hardware.
- Developed and executed automated test suite `tests/test_nucleo_g474re.py`: 100% pass (arithmetic, ROT, RAM definitions, loops, Sieve of Eratosthenes 1899 primes in 1.051s).

---

## 3. Verification Summary
- **Binary Footprints**:
  - `boards/nucleo_g474re`: Flash: 9,532 B (1.8%), RAM: 28 B (0.0%).
  - `boards/olimex_e407`: Flash: 9,532 B (0.9%), RAM: 28 B (0.0%).
- **Hardware Execution**:
  - Nucleo-G474RE verified on `/dev/ttyACM0` via automated tests.
  - Olimex-E407 verified on `/dev/ttyUSB1` via screen interactive session.
  - Both boards fully operational in pure assembly without any C libraries or runtime.

---

## 4. Integration & Release
- Merged `feature/multi-board-e407` into `main` via merge commit `6df11aa`.
- Post-merge build verified: `pio run` cleanly built both `nucleo_g474re` and `olimex_e407`.
- User reviewed and approved the commit log.
- Pushed `main` and milestone tag `v2.0-pure-asm-boot` upstream to `origin/main`.

