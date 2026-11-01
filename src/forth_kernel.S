/* forth_kernel.S -- ARM Cortex-M4 CamelForth Direct-Threaded Kernel
 *
 * Combines:
 *   - Forth VM register definitions & macros
 *   - Dictionary macros (head, immed)
 *   - forth_m4_prim.inc (Machine code primitives)
 *   - forth_m4_d.inc    (Architecture & Model Dependencies)
 *   - forth_m4_high.inc (High-Level Words)
 *   - Initial UINIT image with ROM_LATEST
 */

    .syntax unified
    .thumb

    /* Forth VM Register Allocation (ARM Cortex-M4 Low Registers) */
    tos .req r0   /* Top of Stack (data stack) */
    w   .req r1   /* Working register / CFA */
    t1  .req r2   /* Scratch temporary 1 */
    t2  .req r3   /* Scratch temporary 2 */
    f_ip .req r4  /* Forth Instruction Pointer (r4) */
#define f_ip r4
    psp .req r5   /* Parameter Stack Pointer */
    rsp .req r6   /* Return Stack Pointer */
    up  .req r7   /* User Area Base Pointer */

    /* Inner Interpreter Macro */
    .macro next
        ldr.w w, [f_ip], #4
        bx    w
    .endm

    /* 32-bit immediate load macro for Cortex-M */
    .macro mov32 reg, val
        movw \reg, #:lower16:\val
        movt \reg, #:upper16:\val
    .endm

    .equ docode, 0
    .set link, 0

    /* Forward Equates */
    .equ EXIT_XT, EXIT
    .equ LIT_XT,  LIT
    .equ COLD_XT, COLD

    /* Dictionary Header Macros */
    .macro head label, name, action
        .balign 4
        .long link
        .byte 0                /* 0 = normal word */
    98:
        .set link, 98b         /* link points to NFA */
        .byte 99f - 97f        /* string length */
    97:
        .ascii "\name"
    99:
        .balign 4
    .global \label
    .thumb_func
    \label:
        .ifnc \action, docode
        bl \action
        .endif
    .endm

    .macro immed label, name, action
        .balign 4
        .long link
        .byte 1                /* 1 = immediate word */
    98:
        .set link, 98b         /* link points to NFA */
        .byte 99f - 97f        /* string length */
    97:
        .ascii "\name"
    99:
        .balign 4
    .global \label
    .thumb_func
    \label:
        .ifnc \action, docode
        bl \action
        .endif
    .endm

    .text

    /* Include Primitives */
    .include "src/forth_m4_prim.inc"

    /* Include Architecture Dependencies */
    .include "src/forth_m4_d.inc"

    /* Include High-Level Words */
    .include "src/forth_m4_high.inc"

    /* Initial User Area Image (copied to 0x20001040 on cold boot) */
    .balign 4
    .global uinit_data
uinit_data:
    .long 0                      /* 0: U0 / reserved */
    .long 0                      /* 1: >IN = 0 */
    .long 10                     /* 2: BASE = 10 */
    .long 0                      /* 3: STATE = 0 */
    .long 0x20002000             /* 4: DP = start of RAM dictionary */
    .long 0, 0                   /* 5, 6: 'SOURCE (0, 0) */
    .long link                   /* 7: LATEST = last word in ROM */
    .long 0x20001200             /* 8: HP = PAD */
    .long 0x20001600             /* 9: LP */
    .long 0, 0, 0, 0, 0, 0       /* padding to 16 cells */

