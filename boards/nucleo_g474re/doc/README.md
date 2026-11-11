# ST Nucleo-G474RE Forth Port Guide

This directory contains the hardware support files and C startup harness for running CamelForth DTC on the **STMicroelectronics Nucleo-G474RE** development board.

## Hardware Specifications

- **MCU**: STMicroelectronics STM32G474RET6 (ARM Cortex-M4)
- **Core Clock**: 170 MHz (via 16 MHz HSI / PLL in Boost Mode)
- **Flash Memory**: 512 KB starting at `0x08000000`
- **SRAM**: 128 KB contiguous system SRAM (`0x20000000` to `0x2001FFFF`)
- **Status LED**: Green User LED LD2 on `PA5` (active high)
- **User Button**: Blue Button B1 on `PC13`
- **Console Interface**: LPUART1 connected to onboard ST-LINK V3E Virtual COM port:
  - `PA2` : LPUART1_TX (AF12)
  - `PA3` : LPUART1_RX (AF12)
  - **Serial Parameters**: 115200 baud, 8 data bits, no parity, 1 stop bit (8N1)
  - **Host Serial Device**: `/dev/ttyACM0` (ST-Link VCP)
- **Debug / Flash Interface**: Onboard ST-LINK V3E (SWD) via OpenOCD

## Directory Layout

```text
boards/nucleo_g474re/
├── platformio.ini           # Standalone PlatformIO configuration
├── include/
│   ├── main.h               # Board pinouts, clocks, peripheral prototypes
│   └── stm32g4xx_hal_conf.h # STM32CubeG4 HAL module configuration
├── src/
│   ├── main.c               # SystemClock_Config (170 MHz), LD2 init, forth_cold()
│   ├── console_io.c         # LPUART1 driver (forth_tx_char, forth_rx_char, forth_rx_ready)
│   └── forth_kernel.S       # Symlink to ../../../kernel/src/forth_kernel.S
└── doc/
    ├── README.md            # This document
    └── datasheets/          # MCU datasheet, RM0440 Reference Manual
```

## Building and Flashing

### Building

From the workspace root:
```bash
pio run -e nucleo_g474re
```

Or from within this directory:
```bash
pio run
```

### Uploading via ST-Link

```bash
pio run -e nucleo_g474re -t upload
```

### Connecting to Forth Console

```bash
pio device monitor -p /dev/ttyACM0 -b 115200
```
or with picocom:
```bash
picocom -b 115200 /dev/ttyACM0
```
