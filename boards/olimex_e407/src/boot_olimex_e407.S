/* boot_olimex_e407.S -- Pure Assembly Boot & Console Driver for Olimex STM32-E407
 *
 * Target: STMicroelectronics STM32F407ZGT6 (ARM Cortex-M4F)
 * Board:  Olimex STM32-E407
 *
 * Sequence:
 *   1. Hardware Reset entry at Reset_Handler
 *   2. Enable Cortex-M4 FPU (CPACR CP10/CP11 full access)
 *   3. Configure Clocks: 12 MHz HSE -> 168 MHz SYSCLK (PLL: M=12, N=336, P=2, Q=7)
 *   4. Configure Flash Latency: 5 WS + Prefetch + Instruction/Data Caches
 *   5. Configure Bus Prescalers: AHB /1, APB1 /4 (42 MHz), APB2 /2 (84 MHz)
 *   6. Enable Peripheral Clocks: GPIOC, GPIOD, USART3
 *   7. Initialize Status LED (PC13 Output PP, Active Low)
 *   8. Initialize Console Pins: PD8 (TX) / PD9 (RX) AF7 (USART3)
 *   9. Initialize USART3: 115200 baud @ 42 MHz APB1, 8N1
 *  10. Jump directly to CamelForth forth_cold
 *  11. Provide assembly forth_tx_char, forth_rx_ready, forth_rx_char
 */

    .syntax unified
    .thumb

    .macro mov32 reg, val
        movw \reg, #:lower16:\val
        movt \reg, #:upper16:\val
    .endm

    .section .isr_vector, "a", %progbits
    .type g_pfnVectors, %object
    .global g_pfnVectors
g_pfnVectors:
    .long 0x20020000            /* 0: Top of SRAM (MSP) */
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

    /* 2. Enable Power Interface Clock & Voltage Scaling */
    mov32 r0, 0x40023840        /* RCC_APB1ENR */
    ldr   r1, [r0]
    orr   r1, r1, #(1 << 28)    /* PWREN */
    str   r1, [r0]

    mov32 r0, 0x40007000        /* PWR_CR */
    ldr   r1, [r0]
    orr   r1, r1, #(1 << 14)    /* VOS = 1 (Scale 1 for <= 168 MHz) */
    str   r1, [r0]

    /* 3. Enable 12.0 MHz HSE Crystal Oscillator */
    mov32 r0, 0x40023800        /* RCC_CR */
    ldr   r1, [r0]
    orr   r1, r1, #(1 << 16)    /* HSEON */
    str   r1, [r0]

1:  ldr   r1, [r0]
    tst   r1, #(1 << 17)        /* Wait for HSERDY */
    beq   1b

    /* 4. Configure Flash Latency (5 WS @ 168 MHz) and Caches */
    mov32 r0, 0x40023C00        /* FLASH_ACR */
    mov32 r1, 0x00000705        /* Latency 5 + PRFTEN (bit 8) + ICEN (bit 9) + DCEN (bit 10) */
    str   r1, [r0]

    /* 5. Configure Bus Prescalers: AHB /1, APB1 /4 (42 MHz), APB2 /2 (84 MHz) */
    mov32 r0, 0x40023808        /* RCC_CFGR */
    ldr   r1, [r0]
    mov32 r2, 0xFFFF030C        /* Mask out PPRE2, PPRE1, HPRE, SW */
    and   r1, r1, r2
    mov32 r2, 0x00009400        /* PPRE2 = 4 (/2), PPRE1 = 5 (/4), HPRE = 0 (/1) */
    orr   r1, r1, r2
    str   r1, [r0]

    /* 6. Configure Main PLL:
     * Source = HSE (12 MHz)
     * PLLM = 12 (VCO in = 1 MHz)
     * PLLN = 336 (VCO out = 336 MHz)
     * PLLP = 2 (/2 -> SYSCLK = 168 MHz)
     * PLLQ = 7 (USB/SDIO = 48 MHz)
     * Value: (7 << 24) | (1 << 22) | (0 << 16) | (336 << 6) | 12 = 0x0740540C
     */
    mov32 r0, 0x40023804        /* RCC_PLLCFGR */
    mov32 r1, 0x0740540C
    str   r1, [r0]

    /* 7. Enable PLL and wait for lock */
    mov32 r0, 0x40023800        /* RCC_CR */
    ldr   r1, [r0]
    orr   r1, r1, #(1 << 24)    /* PLLON */
    str   r1, [r0]

2:  ldr   r1, [r0]
    tst   r1, #(1 << 25)        /* Wait for PLLRDY */
    beq   2b

    /* 8. Switch SYSCLK to PLL */
    mov32 r0, 0x40023808        /* RCC_CFGR */
    ldr   r1, [r0]
    bic   r1, r1, #0x03
    orr   r1, r1, #0x02         /* SW = 2 (PLL) */
    str   r1, [r0]

3:  ldr   r1, [r0]
    and   r2, r1, #0x0C         /* SWS bits [3:2] */
    cmp   r2, #0x08             /* SWS == 0b10 (PLL)? */
    bne   3b

    /* 9. Enable Peripheral Clocks: GPIOC, GPIOD, USART3 */
    mov32 r0, 0x40023830        /* RCC_AHB1ENR */
    ldr   r1, [r0]
    orr   r1, r1, #((1 << 2) | (1 << 3)) /* GPIOCEN | GPIODEN */
    str   r1, [r0]

    mov32 r0, 0x40023840        /* RCC_APB1ENR */
    ldr   r1, [r0]
    orr   r1, r1, #(1 << 18)    /* USART3EN */
    str   r1, [r0]

    /* 10. Configure User Green LED on PC13 (Output PP, Active Low) */
    mov32 r0, 0x40020800        /* GPIOC_BASE */
    ldr   r1, [r0, #0x00]       /* GPIOC_MODER */
    bic   r1, r1, #(3 << 26)
    orr   r1, r1, #(1 << 26)    /* Pin 13: 01b (Output) */
    str   r1, [r0, #0x00]

    /* Turn on User LED (PC13 driven LOW) */
    mov32 r1, (1 << 29)         /* BR13 = Reset PC13 */
    str   r1, [r0, #0x18]       /* GPIOC_BSRR */

    /* 11. Configure USART3 Pins on PD8 (TX) and PD9 (RX) */
    mov32 r0, 0x40020C00        /* GPIOD_BASE */

    /* MODER: Pin 8 [17:16] = 10b (AF), Pin 9 [19:18] = 10b (AF) */
    ldr   r1, [r0, #0x00]       /* MODER */
    bic   r1, r1, #(0xF << 16)
    orr   r1, r1, #(0xA << 16)
    str   r1, [r0, #0x00]

    /* OSPEEDR: Pin 8/9 Very High Speed (11b) */
    ldr   r1, [r0, #0x08]       /* OSPEEDR */
    orr   r1, r1, #(0xF << 16)
    str   r1, [r0, #0x08]

    /* PUPDR: Pin 8/9 Pull-up (01b) */
    ldr   r1, [r0, #0x0C]       /* PUPDR */
    bic   r1, r1, #(0xF << 16)
    orr   r1, r1, #(0x5 << 16)
    str   r1, [r0, #0x0C]

    /* AFRH (offset 0x24): PD8=AF7 (bits 3:0), PD9=AF7 (bits 7:4) */
    ldr   r1, [r0, #0x24]       /* AFR[1] / AFRH */
    bic   r1, r1, #0xFF
    orr   r1, r1, #0x77         /* AF7 = USART3 */
    str   r1, [r0, #0x24]

    /* 12. Configure USART3 Baud Rate (115200 baud @ 42 MHz APB1) */
    mov32 r0, 0x40004800        /* USART3_BASE */
    mov32 r1, 0x016C            /* BRR: 42000000 / (16 * 115200) = 22.786 -> 22, 12 -> 0x16C */
    str   r1, [r0, #0x08]       /* USART_BRR */

    /* Enable USART3, Transmitter, and Receiver */
    mov32 r1, 0x200C            /* UE (bit 13) | TE (bit 3) | RE (bit 2) */
    str   r1, [r0, #0x0C]       /* USART_CR1 */

    /* 13. Jump directly to CamelForth COLD start in forth_m4_prim.inc */
    b     forth_cold

    .size Reset_Handler, .-Reset_Handler

/* ===================================================================
 * Native Assembly Console I/O Routines for STM32F407 USART3
 * ===================================================================
 */

    .text

/* forth_tx_char ( uint8_t c [R0] -- void )
 * Transmit a single character over USART3
 */
    .global forth_tx_char
    .thumb_func
forth_tx_char:
    mov32 r1, 0x40004800        /* USART3_BASE */
1:  ldr   r2, [r1, #0x00]       /* USART_SR */
    tst   r2, #(1 << 7)         /* TXE (bit 7) */
    beq   1b
    strb  r0, [r1, #0x04]       /* USART_DR = c */
    bx    lr

/* forth_rx_ready ( void -- int32_t flag [R0] )
 * Returns -1 (TRUE) if character available in USART3 RX buffer, 0 (FALSE) otherwise.
 * Clears overrun/framing errors on STM32F4 (read SR followed by read DR).
 */
    .global forth_rx_ready
    .thumb_func
forth_rx_ready:
    mov32 r1, 0x40004800        /* USART3_BASE */
    ldr   r0, [r1, #0x00]       /* USART_SR */
    tst   r0, #0x0F             /* ORE, NE, FE, PE error flags */
    beq   1f
    ldrb  r2, [r1, #0x04]       /* Reading DR clears error flags */
1:  ldr   r0, [r1, #0x00]       /* Re-read SR */
    tst   r0, #(1 << 5)         /* RXNE (bit 5) */
    ite   ne
    mvnne r0, #0                /* R0 = -1 (TRUE) */
    moveq r0, #0                /* R0 =  0 (FALSE) */
    bx    lr

/* forth_rx_char ( void -- uint8_t c [R0] )
 * Blocking receive of next character from USART3
 */
    .global forth_rx_char
    .thumb_func
forth_rx_char:
    mov32 r1, 0x40004800        /* USART3_BASE */
1:  ldr   r0, [r1, #0x00]       /* USART_SR */
    tst   r0, #0x0F             /* Check error flags */
    beq   2f
    ldrb  r2, [r1, #0x04]       /* Clear error */
2:  ldr   r0, [r1, #0x00]       /* Check RXNE */
    tst   r0, #(1 << 5)
    beq   1b
    ldrb  r0, [r1, #0x04]       /* Read byte from USART_DR */
    bx    lr
