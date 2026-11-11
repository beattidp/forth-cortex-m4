/**
  ******************************************************************************
  * @file           : console_io.c
  * @brief          : Serial console driver implementation for STM32-E407 (USART3 on PD8/PD9)
  ******************************************************************************
  */

#include "console_io.h"
#include "main.h"

UART_HandleTypeDef huart_console;

void console_init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 1. Enable GPIO and UART clocks */
    CONSOLE_UART_GPIO_CLK_ENABLE();
    CONSOLE_UART_CLK_ENABLE();

    /* 2. Configure GPIO TX (PD8) and RX (PD9) pins as AF7 (USART3) */
    GPIO_InitStruct.Pin = CONSOLE_UART_TX_PIN | CONSOLE_UART_RX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = CONSOLE_UART_AF;
    HAL_GPIO_Init(CONSOLE_UART_TX_PORT, &GPIO_InitStruct);

    /* 3. Configure UART peripheral (115200 8N1) */
    huart_console.Instance = CONSOLE_UART;
    huart_console.Init.BaudRate = CONSOLE_BAUDRATE;
    huart_console.Init.WordLength = UART_WORDLENGTH_8B;
    huart_console.Init.StopBits = UART_STOPBITS_1;
    huart_console.Init.Parity = UART_PARITY_NONE;
    huart_console.Init.Mode = UART_MODE_TX_RX;
    huart_console.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart_console.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart_console) != HAL_OK) {
        Error_Handler();
    }
}

void forth_tx_char(uint8_t c) {
    /* Wait until transmit data register is empty */
    while ((CONSOLE_UART->SR & USART_SR_TXE) == 0U) {
    }
    CONSOLE_UART->DR = (uint8_t)c;
}

int32_t forth_rx_ready(void) {
    /* Clear overrun, noise, framing, parity errors if present (STM32F4: read SR then DR) */
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
