# Multi-Board Project Architecture

## Overview

The `forth-cortex-m4` project implements a direct-threaded 32-bit Forth kernel based on CamelForth for ARM Cortex-M4 processors. To support multiple hardware evaluation boards and microcontroller families cleanly, the project is structured as a modular multi-board system with a shared core VM.

## Directory Organization

```text
forth-cortex-m4/
├── platformio.ini                    # Root multi-board PlatformIO build dispatcher
├── kernel/                           # Target-Independent Cortex-M4 Forth Engine
│   ├── include/
│   │   ├── forth_config.h            # Forth SRAM layout, stack sizes, buffers
│   │   └── console_io.h              # Common I/O C function prototypes
│   └── src/
│       ├── forth_kernel.S            # Direct-threaded VM entry, reset & UINIT table
│       ├── forth_m4_prim.inc         # Machine code Forth primitives (ALU, stacks, logic)
│       ├── forth_m4_d.inc            # Model & compiler dependencies (!CF, ALIGN, etc.)
│       └── forth_m4_high.inc         # High-level Forth core words (COLD, QUIT, EVAL, etc.)
├── boards/
│   ├── nucleo_g474re/                # STMicroelectronics Nucleo-G474RE (STM32G474RET6)
│   │   ├── platformio.ini            # Standalone project config (ST-Link V3E, OpenOCD)
│   │   ├── include/
│   │   │   ├── main.h                # Board hardware definitions (LD2 LED PA5)
│   │   │   └── stm32g4xx_hal_conf.h  # STM32CubeG4 HAL configuration
│   │   ├── src/
│   │   │   ├── main.c                # C bootstrap harness (170 MHz PLL, console, forth_cold)
│   │   │   ├── console_io.c          # LPUART1 driver (PA2 TX / PA3 RX @ 115200)
│   │   │   └── forth_kernel.S        # Symlink -> ../../../kernel/src/forth_kernel.S
│   │   └── doc/
│   │       ├── README.md             # Board guide & pin mapping
│   │       └── datasheets/           # MCU datasheet & RM0440 Reference Manual
│   └── olimex_e407/                  # Olimex STM32-E407 (STM32F407ZGT6)
│       ├── platformio.ini            # Standalone project config (J-Link JTAG)
│       ├── extra_upload.py           # J-Link Commander flashing script
│       ├── include/
│       │   ├── main.h                # Board hardware definitions (PC13 LED, USART3)
│       │   └── stm32f4xx_hal_conf.h  # STM32CubeF4 HAL configuration
│       ├── src/
│       │   ├── main.c                # C bootstrap harness (168 MHz PLL, console, forth_cold)
│       │   ├── console_io.c          # USART3 driver (PD8 TX / PD9 RX @ 115200)
│       │   └── forth_kernel.S        # Symlink -> ../../../kernel/src/forth_kernel.S
│       └── doc/
│           ├── README.md             # Board guide & connector pin mapping
│           └── datasheets/           # Schematics, MCU datasheet & RM0090 Manual
├── doc/                              # Repository-wide documentation, features, worklogs
└── test/                             # Architecture-independent Forth test scripts (.fs)
```

## Build System Capabilities

The build system offers two workflow options:

1. **Root Workspace Dispatcher**:
   - `pio run` builds all configured boards.
   - `pio run -e nucleo_g474re` builds only the Nucleo board.
   - `pio run -e olimex_e407` builds only the Olimex board.
   - Both environments share `kernel/` without duplication.

2. **Standalone Board Subprojects**:
   - Each directory under `boards/<board_name>/` contains its own `platformio.ini`.
   - Running `pio run` inside `boards/olimex_e407/` or running `pio run -d boards/olimex_e407` isolates build cache and configurations completely.

## Strategy Toward Pure Assembly Boot

Having two operational targets sharing the identical C bootstrap pattern (`HAL_Init` -> `SystemClock_Config` -> `console_init` -> `forth_cold`) allows clear separation of:
1. **Target-Independent Startup**:
   - RAM Trampoline table copy (`0x20001000`)
   - User Area initialization (`0x20001040`)
   - Forth Parameter & Return Stack setup (`0x20001800` / `0x20002000`)
   - Forth inner interpreter launch (`COLD`)
2. **Target-Specific Hardware Initialization**:
   - Oscillator / PLL setup (HSI/HSE clock sources, prescalers, multipliers, latency)
   - Bus clock gating (AHB/APB clock enable registers)
   - GPIO alternate function pin muxing for UART RX/TX
   - UART baud rate register (`BRR`) and control registers (`CR1`/`CR2`)
