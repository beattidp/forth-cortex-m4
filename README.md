# forth-cortex-m4

CamelForth 32-bit Direct-Threaded Code (DTC) kernel for ARM Cortex-M4 microcontrollers.

## Supported Hardware Platforms

| Board | MCU | Frequency | Console Port | Debug / Upload | Board Directory |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **ST Nucleo-G474RE** | STM32G474RET6 | 170 MHz | LPUART1 (`PA2`/`PA3`) @ 115200 | ST-LINK V3E (OpenOCD) | [`boards/nucleo_g474re/`](boards/nucleo_g474re/) |
| **Olimex STM32-E407** | STM32F407ZGT6 | 168 MHz | USART3 (`PD8`/`PD9`) @ 115200 | Segger J-Link (JTAG) | [`boards/olimex_e407/`](boards/olimex_e407/) |

## Building

### Build all boards from repository root:
```bash
pio run
```

### Build a specific board:
```bash
pio run -e nucleo_g474re
# or
pio run -e olimex_e407
```

### Build from board directory:
```bash
cd boards/olimex_e407
pio run
```

## Flashing & Console

### ST Nucleo-G474RE
```bash
pio run -e nucleo_g474re -t upload
pio device monitor -p /dev/ttyACM0 -b 115200
```

### Olimex STM32-E407
```bash
pio run -e olimex_e407 -t upload
pio device monitor -p /dev/ttyUSB1 -b 115200
```

## Interactive Forth REPL

```forth
CamelForth for ARM Cortex-M4
Direct-Threaded Forth Kernel
Type 'WORDS' to list dictionary.
ok
WORDS
SAVEU AT-XY COLD WORDS ?ABORT SP! ABORT RP! QUIT ACCEPT EVALUATE INTERPRET ?NUMBER WITHIN >NUMBER ENDCASE ENDOF OF CASE LEAVE +LOOP LOOP ENDLOOP DO L> >L REPEAT WHILE AGAIN UNTIL BEGIN ELSE THEN IF USER CONSTANT VARIABLE RECURSE POSTPONE ['] DOES> (DOES>) ; : CREATE LITERAL ] [ IMMEDIATE REVEAL HIDE \ .( ( [CHAR] CHAR ' FIND IMMED? NFA>CFA NFA>LFA ." S" (S") WORD >COUNTED /STRING SOURCE CR TYPE SPACES SPACE ? . U. #> #S # >digit SIGN <# HOLD UD/MOD C, , ALLOT HERE COUNT DECIMAL HEX TUCK NIP */ */MOD MOD / /MOD M*/ D< MAX MIN M+ D- D+ ?DNEGATE DNEGATE ABS ?NEGATE S>D #INIT UINIT R0 L0 PAD S0 LP HP LATEST 'SOURCE DP STATE BASE >IN U0 TIB TIBSIZE FALSE TRUE BL FLUSH-CACHE !DEST ,DEST ,BRANCH ,EXIT !COLON ,CF !CF COMPILE, >BODY CHARS CHAR+ CELLS CELL+ CELL ALIGNED ALIGN BYE S= SCAN SKIP FILL CMOVE> CMOVE ?SIGN DIGIT? UD* KEY KEY? EMIT UNLOOP J I (+loop) (loop) (do) ?branch branch SM/REM FM/MOD UM/MOD M* UM* * U< > < = 0> 0< 0= RSHIFT LSHIFT XOR OR AND INVERT NEGATE 2/ 2* 2- 2+ 1- 1+ - + 2! 2@ +! C! C@ ! @ R@ R> >R 2SWAP 2DROP 2DUP ROT OVER SWAP DROP ?DUP DUP EXECUTE lit EXIT 
ok
```

See the [Architecture Documentation](doc/features/multi_board_architecture.md) and [Walkthrough](doc/walkthrough.md) for details.

---

### Credit where Credit is Due:
The code for this application was developed with [Google Antigravity](http://antigravity.google/).  _Thank you!_
