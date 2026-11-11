/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c on Olimex STM32-E407 board
  ******************************************************************************
  */

#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/* Board hardware definitions: OLIMEX STM32-E407 */

/* User LED (Green): PC13 */
#define LED_PIN                    GPIO_PIN_13
#define LED_GPIO_PORT              GPIOC
#define LED_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOC_CLK_ENABLE()

/* User Button (WKUP): PA0 */
#define USER_BTN_PIN               GPIO_PIN_0
#define USER_BTN_GPIO_PORT         GPIOA
#define USER_BTN_GPIO_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE()

/* Console UART: USART3 on Connector PD (PD8 TX, PD9 RX) */
#define CONSOLE_UART               USART3
#define CONSOLE_UART_CLK_ENABLE()  __HAL_RCC_USART3_CLK_ENABLE()
#define CONSOLE_UART_GPIO_CLK_ENABLE() __HAL_RCC_GPIOD_CLK_ENABLE()
#define CONSOLE_UART_TX_PIN        GPIO_PIN_8
#define CONSOLE_UART_TX_PORT       GPIOD
#define CONSOLE_UART_RX_PIN        GPIO_PIN_9
#define CONSOLE_UART_RX_PORT       GPIOD
#define CONSOLE_UART_AF            GPIO_AF7_USART3
#define CONSOLE_BAUDRATE           115200

extern UART_HandleTypeDef huart_console;

void SystemClock_Config(void);
void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
