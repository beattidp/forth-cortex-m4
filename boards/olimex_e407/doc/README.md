# Olimex STM32-E407 Forth Port Guide

This directory contains the hardware support files and C startup harness for running CamelForth DTC on the **Olimex STM32-E407** development board.

## Hardware Specifications

- **MCU**: STMicroelectronics STM32F407ZGT6 (ARM Cortex-M4F)
- **Core Clock**: 168 MHz (via 12.0 MHz HSE crystal and main PLL)
- **Flash Memory**: 1024 KB (1 MB) starting at `0x08000000`
- **SRAM**: 128 KB contiguous system SRAM (`0x20000000` to `0x2001FFFF`)
- **Status LED**: Green User LED on `PC13` (active low)
- **Console Interface**: USART3 on extension connector PD:
  - `PD8` : USART3_TX (connect to serial cable RXD)
  - `PD9` : USART3_RX (connect to serial cable TXD)
  - `GND` : Connect to serial cable GND
  - **Serial Parameters**: 115200 baud, 8 data bits, no parity, 1 stop bit (8N1)
  - **Host Serial Device**: `/dev/ttyUSB1` (e.g., TTL-232R-RPi cable)
- **Debug / Flash Interface**: 20-pin JTAG connector via Segger J-Link

## Directory Layout

```text
boards/olimex_e407/
├── platformio.ini           # Standalone PlatformIO configuration
├── extra_upload.py          # J-Link Commander upload script
├── include/
│   ├── main.h               # Board pinouts, clocks, peripheral prototypes
│   └── stm32f4xx_hal_conf.h # STM32CubeF4 HAL module configuration
├── src/
│   ├── main.c               # SystemClock_Config (168 MHz), LED init, forth_cold()
│   ├── console_io.c         # USART3 driver (forth_tx_char, forth_rx_char, forth_rx_ready)
│   └── forth_kernel.S       # Symlink to ../../../kernel/src/forth_kernel.S
└── doc/
    ├── README.md            # This document
    └── datasheets/          # Hardware schematics, MCU datasheet, reference manual
```

## Building and Flashing

### Building

From the workspace root:
```bash
pio run -e olimex_e407
```

Or from within this directory:
```bash
pio run
```

### Uploading via J-Link JTAG

Ensure your J-Link is connected to the STM32-E407 JTAG header and USB:
```bash
pio run -e olimex_e407 -t upload
```

### Connecting to Forth Console

```bash
pio device monitor -p /dev/ttyUSB1 -b 115200
```
or with picocom:
```bash
picocom -b 115200 /dev/ttyUSB1
```
Press Enter or Reset to see the prompt:
```text
CamelForth for ARM Cortex-M4
Direct-Threaded Forth Kernel
Type 'WORDS' to list dictionary.
ok
```
