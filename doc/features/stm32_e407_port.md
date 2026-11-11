# Olimex STM32-E407 Forth Port

## Overview

The Olimex STM32-E407 port adapts the CamelForth ARM Cortex-M4 direct-threaded engine to the STMicroelectronics STM32F407ZGT6 MCU running on the Olimex STM32-E407 development board.

## Hardware & Peripheral Mapping

| Peripheral | Board Resource | MCU Pin / Register | Details |
| :--- | :--- | :--- | :--- |
| **System Clock** | 12.0 MHz Crystal (HSE) | OSC_IN / OSC_OUT | PLL: M=12, N=336, P=2, Q=7 $\rightarrow$ SYSCLK = 168 MHz |
| **Status LED** | Green LED (LED1) | `PC13` (Active Low) | Driven LOW at startup to indicate Forth runtime is active |
| **Console UART** | Header PD (PD8 / PD9) | `USART3` (AF7) | PD8 = TX, PD9 = RX, 115200 baud, 8N1 |
| **Flash Memory** | On-chip Flash (1 MB) | `0x08000000 - 0x080FFFFF` | Flash Latency: 5 WS with I/D caches & prefetch |
| **System SRAM** | On-chip SRAM1/2 (128 KB) | `0x20000000 - 0x2001FFFF` | Dedicated contiguous Forth data space |
| **JTAG Interface** | 20-pin ARM JTAG Header | Segger J-Link | Scripted flashing via `extra_upload.py` and J-Link Commander |

## Console I/O Implementation

The Forth console interface relies on register-level operations on STM32F4 `USART3`:

```c
void forth_tx_char(uint8_t c) {
    while ((CONSOLE_UART->SR & USART_SR_TXE) == 0U) {
    }
    CONSOLE_UART->DR = (uint8_t)c;
}

int32_t forth_rx_ready(void) {
    if (CONSOLE_UART->SR & (USART_SR_ORE | USART_SR_NE | USART_SR_FE | USART_SR_PE)) {
        volatile uint32_t sr_dummy = CONSOLE_UART->SR;
        volatile uint32_t dr_dummy = CONSOLE_UART->DR;
        (void)sr_dummy;
        (void)dr_dummy;
    }
    return (CONSOLE_UART->SR & USART_SR_RXNE) ? -1 : 0;
}

uint8_t forth_rx_char(void) {
    while (!forth_rx_ready()) {
    }
    return (uint8_t)(CONSOLE_UART->DR & 0xFFU);
}
```

### STM32F4 vs STM32G4 Register Differences

- **STM32F4**:
  - Combined Status Register: `USART->SR`
  - Flags: `USART_SR_TXE`, `USART_SR_RXNE`, `USART_SR_ORE`, `USART_SR_FE`, `USART_SR_PE`
  - Data Register: `USART->DR` (single 9-bit register for TX and RX)
  - Error clearance: reading `SR` followed by reading `DR`
- **STM32G4**:
  - Interrupt & Status: `USART->ISR`
  - Flags: `USART_ISR_TXE_TXFNF`, `USART_ISR_RXNE_RXFNE`, `USART_ISR_ORE`, etc.
  - Separate Transmit/Receive registers: `USART->TDR` and `USART->RDR`
  - Interrupt Flag Clear: `USART->ICR` (`USART_ICR_ORECF`, `USART_ICR_FECF`, etc.)

## Flashing & Debugging

The Olimex STM32-E407 board utilizes an external Segger J-Link connected via the standard 20-pin JTAG header. The `extra_upload.py` script generates a dynamic J-Link Commander command script:

```python
commands = [
    "connect",
    "h",
    "loadbin %s, 0x08000000" % source,
    "r",
    "g",
    "q"
]
```

Flashing is executed via:
```bash
pio run -e olimex_e407 -t upload
```
or from the board directory:
```bash
pio run -d boards/olimex_e407 -t upload
```

The serial console connects on `/dev/ttyUSB1` at 115200 baud 8N1.
