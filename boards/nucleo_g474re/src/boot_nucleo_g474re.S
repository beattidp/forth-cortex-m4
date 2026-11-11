/* boot_nucleo_g474re.S -- Pure Assembly Boot & Console Driver for ST Nucleo-G474RE
 *
 * Target: STMicroelectronics STM32G474RET6 (ARM Cortex-M4F)
 * Board:  ST Nucleo-G474RE
 *
 * Sequence:
 *   1. Hardware Reset entry at Reset_Handler
 *   2. Set Stack Pointer to top of SRAM (0x20020000, 128 KB)
 *   3. Enable Cortex-M4 FPU (CPACR CP10/CP11 full access)
 *   4. Enable PWR interface clock and configure Voltage Scaling Range 1 Boost mode
 *   5. Enable HSI16 (16 MHz RC oscillator)
 *   6. Configure Flash Latency: 4 WS + Prefetch + Instruction/Data Caches
 *   7. Configure Main PLL: HSI16 -> 170 MHz SYSCLK (PLLM=4, PLLN=85, PLLR=/2)
 *   8. Switch SYSCLK to PLL
 *   9. Enable Peripheral Clocks: GPIOA, LPUART1
 *  10. Initialize User Green LED (PA5 Output PP, Drive HIGH)
 *  11. Initialize Console Pins: PA2 (TX) / PA3 (RX) AF12 (LPUART1)
 *  12. Initialize LPUART1: 115200 baud @ 170 MHz PCLK1 (BRR=0x5C3B2), 8N1
 *  13. Jump directly to CamelForth forth_cold
 *  14. Provide assembly forth_tx_char, forth_rx_ready, forth_rx_char
 */

    .syntax unified
    .thumb

    .macro mov32 reg, val
        movw \reg, #:lower16:\val
        movt \reg, #:upper16:\val
    .endm

/* ===================================================================
 * Cortex-M4 Vector Table
 * ===================================================================
 */
    .section .isr_vector, "a", %progbits
    .type g_pfnVectors, %object
    .global g_pfnVectors
g_pfnVectors:
    .long 0x20020000            /* 0: Top of SRAM (MSP, 128KB at 0x20000000..0x20020000) */
    .long Reset_Handler         /* 1: Reset Handler */
    .long Default_Handler       /* 2: NMI */
    .long Default_Handler       /* 3: HardFault */
    .long Default_Handler       /* 4: MemManage */
    .long Default_Handler       /* 5: BusFault */
    .long Default_Handler       /* 6: UsageFault */
    .long 0, 0, 0, 0            /* 7-10: Reserved */
    .long Default_Handler       /* 11: SVCall */
    .long Default_Handler       /* 12: DebugMonitor */
    .long 0                     /* 13: Reserved */
    .long Default_Handler       /* 14: PendSV */
    .long SysTick_Handler       /* 15: SysTick */
    .size g_pfnVectors, .-g_pfnVectors

    .section .text.Default_Handler, "ax", %progbits
    .thumb_func
Default_Handler:
    b .

    .global SysTick_Handler
    .thumb_func
SysTick_Handler:
    bx lr

/* ===================================================================
 * Reset Handler -- Pure Assembly Boot Routine
 * ===================================================================
 */
    .section .text.Reset_Handler, "ax", %progbits
    .global Reset_Handler
    .type Reset_Handler, %function
    .thumb_func
Reset_Handler:
    /* Set stack pointer to top of SRAM (0x20020000) */
    mov32 r0, 0x20020000
    mov   sp, r0

    /* 1. Enable FPU (CP10 and CP11 Full Access in SCB->CPACR) */
    mov32 r0, 0xE000ED88        /* SCB_CPACR */
    ldr   r1, [r0]
    orr   r1, r1, #(0xF << 20)  /* Full access CP10, CP11 */
    str   r1, [r0]
    dsb
    isb

    /* 2. Enable Power Interface Clock (RCC_APB1ENR1 bit 28 PWREN) */
    mov32 r0, 0x40021058        /* RCC_APB1ENR1 */
    ldr   r1, [r0]
    orr   r1, r1, #(1 << 28)    /* PWREN */
    str   r1, [r0]

    /* 3. Configure Voltage Scaling Range 1 Boost Mode (required for 170 MHz) */
    mov32 r0, 0x40007000        /* PWR_BASE */
    /* Clear R1MODE (bit 8) in PWR_CR5 (offset 0x80) to enable Range 1 Boost */
    ldr   r1, [r0, #0x80]       /* PWR_CR5 */
    bic   r1, r1, #(1 << 8)
    str   r1, [r0, #0x80]

    /* Set Range 1 in PWR_CR1 (offset 0x00, bits [10:9] = 01b) */
    ldr   r1, [r0, #0x00]       /* PWR_CR1 */
    bic   r1, r1, #(3 << 9)
    orr   r1, r1, #(1 << 9)     /* Range 1 */
    str   r1, [r0, #0x00]

    /* Wait until VOSF flag in PWR_SR2 (offset 0x14) bit 10 is cleared */
1:  ldr   r1, [r0, #0x14]       /* PWR_SR2 */
    tst   r1, #(1 << 10)        /* VOSF */
    bne   1b

    /* 4. Enable HSI16 (16 MHz Internal RC Oscillator) */
    mov32 r0, 0x40021000        /* RCC_CR */
    ldr   r1, [r0]
    orr   r1, r1, #(1 << 8)     /* HSION */
    str   r1, [r0]

2:  ldr   r1, [r0]
    tst   r1, #(1 << 10)        /* Wait for HSIRDY */
    beq   2b

    /* 5. Configure Flash Latency (4 WS @ 170 MHz Boost) and Prefetch/Caches */
    mov32 r0, 0x40022000        /* FLASH_ACR */
    mov32 r1, 0x00000704        /* Latency 4 + PRFTEN (bit 8) + ICEN (bit 9) + DCEN (bit 10) */
    str   r1, [r0]

    /* 6. Configure Main PLL:
     * Source  = HSI16 (16 MHz) -> PLLSRC = 2 (bits [1:0])
     * PLLM    = 4 (divider = 3 + 1 = 4 -> 16 / 4 = 4 MHz VCO input) -> bits [7:4] = 3
     * PLLN    = 85 (multiplier = 85 -> 4 * 85 = 340 MHz VCO output) -> bits [14:8] = 85
     * PLLREN  = 1 (enable PLLR output for SYSCLK) -> bit 24 = 1
     * PLLR    = 2 (divider = 2 -> 340 / 2 = 170 MHz) -> bits [26:25] = 0
     * Value: 2 | (3 << 4) | (85 << 8) | (1 << 24) = 0x01005532
     */
    mov32 r0, 0x4002100C        /* RCC_PLLCFGR */
    mov32 r1, 0x01005532
    str   r1, [r0]

    /* 7. Enable PLL and wait for lock */
    mov32 r0, 0x40021000        /* RCC_CR */
    ldr   r1, [r0]
    orr   r1, r1, #(1 << 24)    /* PLLON */
    str   r1, [r0]

3:  ldr   r1, [r0]
    tst   r1, #(1 << 25)        /* Wait for PLLRDY */
    beq   3b

    /* 8. Switch SYSCLK to PLL (SW = 3) */
    mov32 r0, 0x40021008        /* RCC_CFGR */
    ldr   r1, [r0]
    bic   r1, r1, #0x03
    orr   r1, r1, #0x03         /* SW = 3 (PLL) */
    str   r1, [r0]

4:  ldr   r1, [r0]
    and   r2, r1, #0x0C         /* SWS bits [3:2] */
    cmp   r2, #0x0C             /* SWS == 0b11 (PLL)? */
    bne   4b

    /* 9. Enable Peripheral Clocks: GPIOA (AHB2), LPUART1 (APB1-2) */
    mov32 r0, 0x4002104C        /* RCC_AHB2ENR */
    ldr   r1, [r0]
    orr   r1, r1, #(1 << 0)     /* GPIOAEN */
    str   r1, [r0]

    mov32 r0, 0x4002105C        /* RCC_APB1ENR2 */
    ldr   r1, [r0]
    orr   r1, r1, #(1 << 0)     /* LPUART1EN */
    str   r1, [r0]

    /* 10. Configure User Green LED on PA5 (Output PP) and turn it ON */
    mov32 r0, 0x48000000        /* GPIOA_BASE */
    ldr   r1, [r0, #0x00]       /* GPIOA_MODER */
    bic   r1, r1, #(3 << 10)
    orr   r1, r1, #(1 << 10)    /* Pin 5: 01b (Output) */
    str   r1, [r0, #0x00]

    /* Drive PA5 HIGH (Turn on Green LED LD2) */
    mov32 r1, (1 << 5)          /* BS5 = Set PA5 */
    str   r1, [r0, #0x18]       /* GPIOA_BSRR */

    /* 11. Configure LPUART1 Pins on PA2 (TX) and PA3 (RX) as AF12 */
    /* MODER: Pin 2 [5:4] = 10b (AF), Pin 3 [7:6] = 10b (AF) */
    ldr   r1, [r0, #0x00]
    bic   r1, r1, #(0xF << 4)
    orr   r1, r1, #(0xA << 4)
    str   r1, [r0, #0x00]

    /* OSPEEDR: Pin 2/3 Very High Speed (11b) */
    ldr   r1, [r0, #0x08]
    orr   r1, r1, #(0xF << 4)
    str   r1, [r0, #0x08]

    /* PUPDR: Pin 3 Pull-up (01b), Pin 2 No pull (00b) */
    ldr   r1, [r0, #0x0C]
    bic   r1, r1, #(0xF << 4)
    orr   r1, r1, #(0x4 << 4)   /* Pin 3: 01b */
    str   r1, [r0, #0x0C]

    /* AFRL (offset 0x20): Pin 2 = AF12 (0xC), Pin 3 = AF12 (0xC) */
    ldr   r1, [r0, #0x20]       /* AFR[0] */
    bic   r1, r1, #(0xFF << 8)
    mov32 r2, (0xCC << 8)       /* AF12 for PA2 and PA3 */
    orr   r1, r1, r2
    str   r1, [r0, #0x20]

    /* 12. Configure LPUART1 Baud Rate (115200 baud @ 170 MHz PCLK1)
     * BRR = ((170000000 * 256) + 57600) / 115200 = 377778 = 0x5C3B2
     */
    mov32 r0, 0x40008000        /* LPUART1_BASE */
    mov32 r1, 0x0005C3B2        /* BRR */
    str   r1, [r0, #0x0C]       /* USART_BRR */

    /* Enable LPUART1, Transmitter, and Receiver */
    mov32 r1, 0x0000000D        /* UE (bit 0) | RE (bit 2) | TE (bit 3) */
    str   r1, [r0, #0x00]       /* USART_CR1 */

    /* 13. Jump directly to CamelForth COLD start in forth_m4_prim.inc */
    b     forth_cold

    .size Reset_Handler, .-Reset_Handler

/* ===================================================================
 * Native Assembly Console I/O Routines for STM32G474 LPUART1
 * ===================================================================
 */

    .text

/* forth_tx_char ( uint8_t c [R0] -- void )
 * Transmit a single character over LPUART1
 */
    .global forth_tx_char
    .thumb_func
forth_tx_char:
    mov32 r1, 0x40008000        /* LPUART1_BASE */
1:  ldr   r2, [r1, #0x1C]       /* USART_ISR */
    tst   r2, #(1 << 7)         /* TXE / TXFNF (bit 7) */
    beq   1b
    strb  r0, [r1, #0x28]       /* USART_TDR = c */
    bx    lr

/* forth_rx_ready ( void -- int32_t flag [R0] )
 * Returns -1 (TRUE) if character available in LPUART1 RX buffer, 0 (FALSE) otherwise.
 * Clears overrun/framing errors on STM32G4 by writing to USART_ICR.
 */
    .global forth_rx_ready
    .thumb_func
forth_rx_ready:
    mov32 r1, 0x40008000        /* LPUART1_BASE */
    ldr   r0, [r1, #0x1C]       /* USART_ISR */
    tst   r0, #0x0F             /* ORE (bit 3), NE (bit 2), FE (bit 1), PE (bit 0) */
    beq   1f
    mov   r2, #0x0F
    str   r2, [r1, #0x20]       /* USART_ICR: Clear error flags */
1:  ldr   r0, [r1, #0x1C]       /* Re-read ISR */
    tst   r0, #(1 << 5)         /* RXNE / RXFNE (bit 5) */
    ite   ne
    mvnne r0, #0                /* R0 = -1 (TRUE) */
    moveq r0, #0                /* R0 =  0 (FALSE) */
    bx    lr

/* forth_rx_char ( void -- uint8_t c [R0] )
 * Blocking receive of next character from LPUART1
 */
    .global forth_rx_char
    .thumb_func
forth_rx_char:
    mov32 r1, 0x40008000        /* LPUART1_BASE */
1:  ldr   r0, [r1, #0x1C]       /* Check ISR */
    tst   r0, #0x0F             /* Check error flags */
    beq   2f
    mov   r2, #0x0F
    str   r2, [r1, #0x20]       /* Clear error flags in ICR */
2:  ldr   r0, [r1, #0x1C]       /* Check RXNE */
    tst   r0, #(1 << 5)
    beq   1b
    ldrb  r0, [r1, #0x24]       /* Read byte from USART_RDR */
    bx    lr
