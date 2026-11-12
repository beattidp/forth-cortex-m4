# Feature: Pure Assembly Boot (Zero-C Bootstrap)

## Overview
This feature introduces pure assembly bootstrapping routines for both the **ST Nucleo-G474RE** and the **Olimex STM32-E407** boards. By executing initialization directly in assembly starting from `Reset_Handler`, the Forth system eliminates all C compiler dependencies, the C runtime (`libc` / `libm`), CMSIS C initialization routines, and the STM32 HAL library from the final binary image.

The resulting firmware images are ~9.5 KB of pure ARM Thumb-2 machine code, linking zero C object files while retaining complete interactive Forth REPL functionality, hardware clock trees at maximum rated frequencies, hardware FPU activation, status LED indication, and high-speed serial communications.

---

## Architecture & Boot Sequence

Both boards follow a unified hardware initialization sequence implemented in assembly:

```
+-------------------------------------------------------------+
|                 Hardware Power-On / Reset                   |
+-------------------------------------------------------------+
                              |
                              v
+-------------------------------------------------------------+
| Vector Table (.section .isr_vector, "a", %progbits)         |
|  - Word 0: Top of SRAM (MSP = 0x20020000)                   |
|  - Word 1: Reset_Handler (Thumb mode address: Bit 0 set)    |
+-------------------------------------------------------------+
                              |
                              v
+-------------------------------------------------------------+
| Reset_Handler:                                              |
|  1. Set Main Stack Pointer (MSP) explicitly                 |
|  2. Enable Cortex-M4 FPU (CPACR CP10/CP11 full access)      |
|  3. Enable Power controller & set Voltage Scaling           |
|  4. Start Primary Oscillator & Wait for Ready               |
|  5. Configure Flash Latency + Prefetch + I/D Caches         |
|  6. Configure & Lock Main PLL                               |
|  7. Switch System Clock (SYSCLK) to PLL                     |
|  8. Enable Peripheral Clocks (GPIO + UART)                  |
|  9. Configure Status LED GPIO and drive Active State        |
| 10. Configure UART TX/RX Pins with Alternate Function       |
| 11. Configure UART Baud Rate (BRR) & Enable Transmitter/RX |
| 12. Branch directly to CamelForth COLD start (forth_cold)   |
+-------------------------------------------------------------+
                              |
                              v
+-------------------------------------------------------------+
| CamelForth COLD Engine:                                     |
|  - Initializes Return Stack (R7) and Data Stack (SP)        |
|  - Copies Trampoline templates to RAM                       |
|  - Initializes User Area (UP) and DP dictionary pointer     |
|  - Prints Startup Banner via assembly forth_tx_char         |
|  - Enters Forth Outer Interpreter QUIT loop                 |
+-------------------------------------------------------------+
```

---

## Hardware Implementation Details

| Feature | Olimex STM32-E407 | ST Nucleo-G474RE |
| :--- | :--- | :--- |
| **MCU** | STM32F407ZGT6 | STM32G474RET6 |
| **Boot Source File** | `boards/olimex_e407/src/boot_olimex_e407.S` | `boards/nucleo_g474re/src/boot_nucleo_g474re.S` |
| **SRAM Size / MSP** | 128 KB (0x20000000 .. 0x20020000) / `0x20020000` | 128 KB (0x20000000 .. 0x20020000) / `0x20020000` |
| **Flash Size / Base** | 1024 KB / `0x08000000` | 512 KB / `0x08000000` |
| **Oscillator Source** | 12.0 MHz External Crystal (HSE) | 16.0 MHz High-Speed Internal (HSI16) |
| **Core Frequency (SYSCLK)**| **168 MHz** (PLL: M=12, N=336, P=2) | **170 MHz** (PLL: M=4, N=85, R=2) |
| **Voltage Scaling** | Scale 1 (`PWR_CR` bit 14 `VOS` = 1) | Range 1 Boost Mode (`PWR_CR5` bit 8 `R1MODE` = 0) |
| **Flash Latency & Caches** | 5 Wait States + PRFTEN + ICEN + DCEN (`0x00000705`) | 4 Wait States + PRFTEN + ICEN + DCEN (`0x00000704`) |
| **Bus Dividers** | AHB /1 (168 MHz), APB1 /4 (42 MHz), APB2 /2 (84 MHz) | AHB /1 (170 MHz), APB1 /1 (170 MHz), APB2 /1 (170 MHz) |
| **User Status LED** | PC13 (Active Low, Open-Drain/PP) | PA5 (Active High, Push-Pull) |
| **Console UART** | USART3 @ PD8 (TX) / PD9 (RX) AF7 | LPUART1 @ PA2 (TX) / PA3 (RX) AF12 |
| **Baud Rate Generator** | `USART3->BRR = 0x016C` (115200 @ 42 MHz) | `LPUART1->BRR = 0x0005C3B2` (115200 @ 170 MHz) |
| **Error Clearing** | Read `USART_SR` followed by `USART_DR` | Write `0x0F` to `USART_ICR` |
| **COLD Startup Banner** | `CamelForth Cortex-M4 v1.0 (STM32F407 DTC 32-bit)` | `CamelForth Cortex-M4 v1.0 (STM32G474 DTC 32-bit)` |

---

## Memory Footprint Comparison

| Configuration | Olimex STM32-E407 Flash | Olimex STM32-E407 RAM | ST Nucleo-G474RE Flash | ST Nucleo-G474RE RAM |
| :--- | :---: | :---: | :---: | :---: |
| **With C Wrapper + HAL** | 12,992 B | 1,460 B | 17,288 B | 1,744 B |
| **Pure Assembly Boot** | **9,532 B** | **28 B** | **9,532 B** | **28 B** |
| **Net Reduction** | **-3,460 B (-26.6%)** | **-1,432 B (-98.1%)** | **-7,756 B (-44.9%)** | **-1,716 B (-98.4%)** |

---

## Key Technical Discoveries

1. **Thumb Function Addressing**:
   In GNU `as`, any function referenced in the Cortex-M vector table must be declared with `.thumb_func` immediately preceding the label. Without `.thumb_func`, the assembler emits an even address with bit 0 clear; on Cortex-M execution, this triggers an immediate `INVSTATE` UsageFault upon entering `Reset_Handler`.

2. **Thumb-2 Register Restrictions**:
   The ARMv7-M `movw`/`movt` instructions do not permit targeting `SP` (R13) directly (`Error: r13 not allowed here`). Setting the initial stack pointer is therefore accomplished by loading into scratch register `R0` followed by `mov sp, r0`.

3. **STM32G4 Power Voltage Scaling Boost Mode**:
   Operating the STM32G474 above 150 MHz (up to 170 MHz) requires Range 1 Boost mode. Unlike STM32F4, where voltage scale is set in `PWR_CR`, STM32G4 requires clearing `R1MODE` (bit 8) in `PWR_CR5` (offset `0x80`), followed by selecting Range 1 in `PWR_CR1`, and waiting for `VOSF` (bit 10) in `PWR_SR2` to clear.

4. **LPUART1 Baud Rate Calculation**:
   The low-power UART (`LPUART`) prescaler uses a 20-bit fractional baud rate division with a mandatory 256x multiplier:
   $$\text{BRR} = \text{round}\left(\frac{256 \times f_{\text{CK}}}{\text{Baud}}\right)$$
   At $f_{\text{CK}} = 170\text{ MHz}$ and $115,200\text{ baud}$, $\text{BRR} = 377,778 = \text{0x5C3B2}$.

---

## Verification & Validation
Both targets were flashed and validated on physical hardware:
1. **STM32-E407**: Segger J-Link JTAG, serial on `/dev/ttyUSB1`. Verified interactively via attached terminal session.
2. **Nucleo-G474RE**: ST-LINK V3, serial on `/dev/ttyACM0`. Automated test suite `tests/test_nucleo_g474re.py` passed 100% (arithmetic, ROT, RAM definitions, loops, and Sieve of Eratosthenes finding 1899 primes in 1.051s).
