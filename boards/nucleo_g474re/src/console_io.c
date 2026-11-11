#include "console_io.h"
#include "main.h"

UART_HandleTypeDef hlpuart1;

void console_init(void) {
    /* 1. Enable GPIOA and LPUART1 peripheral clocks */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_LPUART1_CLK_ENABLE();

    /* 2. Configure PA2 (TX) and PA3 (RX) as Alternate Function AF12 (LPUART1) */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = CONSOLE_TX_PIN | CONSOLE_RX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = CONSOLE_TX_AF;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* 3. Configure LPUART1 @ 115200 baud, 8-N-1 */
    hlpuart1.Instance = LPUART1;
    hlpuart1.Init.BaudRate = 115200;
    hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
    hlpuart1.Init.StopBits = UART_STOPBITS_1;
    hlpuart1.Init.Parity = UART_PARITY_NONE;
    hlpuart1.Init.Mode = UART_MODE_TX_RX;
    hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

    if (HAL_UART_Init(&hlpuart1) != HAL_OK) {
        Error_Handler();
    }
}

void forth_tx_char(uint8_t c) {
    /* Wait until transmit data register is ready */
    while ((LPUART1->ISR & USART_ISR_TXE_TXFNF) == 0U) {
    }
    LPUART1->TDR = c;
}

int32_t forth_rx_ready(void) {
    if (LPUART1->ISR & (USART_ISR_ORE | USART_ISR_NE | USART_ISR_FE | USART_ISR_PE)) {
        LPUART1->ICR = USART_ICR_ORECF | USART_ICR_NECF | USART_ICR_FECF | USART_ICR_PECF;
    }
    return (LPUART1->ISR & USART_ISR_RXNE_RXFNE) ? -1 : 0;
}

uint8_t forth_rx_char(void) {
    while (!forth_rx_ready()) {
    }
    return (uint8_t)(LPUART1->RDR & 0xFFU);
}
