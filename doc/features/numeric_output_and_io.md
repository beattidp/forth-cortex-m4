# Formatted Numeric Output & Console I/O Architecture

## Overview
This document covers the console serial I/O driver and formatted numeric output pipeline for the Cortex-M4 CamelForth port.

---

## 1. Console I/O Architecture

The target hardware uses **LPUART1** on `PA2` (TX) and `PA3` (RX) configured for 115200 baud, 8-N-1, clocked from the 170 MHz system PLL.

### Polled Non-Blocking / Blocking Console Driver (`src/console_io.c`)
- **`forth_tx_char(char c)`**:
  - Waits for `LPUART1->ISR & USART_ISR_TXE_TXFNF`.
  - Clears any pending transmission error flags.
  - Writes character byte to `LPUART1->TDR`.
- **`forth_rx_char()`**:
  - Clears overrun, framing, noise, and parity error flags in `LPUART1->ICR`.
  - Polls `LPUART1->ISR & USART_ISR_RXNE_RXFNE`.
  - Reads received byte from `LPUART1->RDR`.
- **`forth_rx_ready()`**:
  - Checks `LPUART1->ISR & USART_ISR_RXNE_RXFNE` and returns `-1` if character available, `0` otherwise.

### Primitives
- `EMIT ( char -- )`: Output single character.
- `KEY ( -- char )`: Receive single character.
- `KEY? ( -- flag )`: Query whether receive buffer has character.
- `TYPE ( c-addr u -- )`: Outputs string of length `u`. Register preservation: retains loop count in `t2` (`R3`) so character loading into `r0` cannot clobber the loop counter.

---

## 2. Formatted Numeric Output Pipeline

Numeric output is built backwards from lowest significant digit to highest into the `PAD` buffer (`0x20001200`).

### User Area Variable: `HP` (Hold Pointer)
- Offset 32 (`8 * 4`) in User Area.
- `<#`: Initializes `HP = PAD`.
- `HOLD`: Decrements `HP` by 1 and writes ASCII byte to `[HP]`.
- `SIGN`: If `n < 0`, appends ASCII `'-'` using `HOLD`.
- `#>`: Takes `( ud1 -- c-addr u )`, dropping `ud1`, setting `c-addr = HP`, and `u = PAD - HP`.

### 64/32-bit Division: `UD/MOD`
To correctly implement ANSI `#` (`NUM`), 64-bit double unsigned numbers must be divided by the 32-bit `BASE` radix:
$$\text{ud1 (64-bit)} \div \text{u2 (32-bit)} \implies \text{u3 (32-bit rem)}, \ \text{ud4 (64-bit quot)}$$

Implemented via:
```forth
head UDSLASHMOD, "UD/MOD", docolon
    .long TOR, LIT, 0, RFETCH, UMSLASHMOD, ROT, ROT
    .long RFROM, UMSLASHMOD, ROT, EXIT
```
- First `UM/MOD` divides high 32 bits: `(0 : d_hi) / u2` $\implies$ `rem_hi, quot_hi`.
- Second `UM/MOD` divides low 32 bits: `(rem_hi : d_lo) / u2` $\implies$ `rem_lo, quot_lo`.
- Stack result: `( rem_lo quot_lo quot_hi )` = `( rem ud_quot )`.

### `#` (`NUM`) & `#S` (`NUMS`)
```forth
head NUM, "#", docolon
    .long BASE_WORD, FETCH, UDSLASHMOD, ROT, TODIGIT, HOLD, EXIT

head NUMS, "#S", docolon
1:  .long NUM, TWODUP, ORR_WORD, ZEROEQUAL, QBRANCH, 1b, EXIT
```
- `#` divides `ud1` by `BASE`, extracts lowest digit to `PAD` via `HOLD`, leaving updated 64-bit quotient `ud2`.
- `#S` repeatedly invokes `#` until the 64-bit quotient is zero.

### `.` (DOT) & `U.` (UDOT)
- `U. ( u -- )`: `<# 0 #S #> TYPE SPACE`
- `. ( n -- )`: `<# DUP ABS 0 #S ROT SIGN #> TYPE SPACE`
