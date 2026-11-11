/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : C startup harness for Forth on Olimex STM32-E407
  ******************************************************************************
  */

#include "main.h"
#include "console_io.h"
#include "forth_config.h"

extern void forth_cold(void);

int main(void) {
    /* Reset of all peripherals, Initializes Flash interface and SysTick */
    HAL_Init();

    /* Configure System Clock to 168 MHz (12 MHz HSE -> PLL) */
    SystemClock_Config();

    /* Configure User Green LED (PC13) */
    LED_GPIO_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = LED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_GPIO_PORT, &GPIO_InitStruct);

    /* Turn on User LED to indicate active firmware */
    HAL_GPIO_WritePin(LED_GPIO_PORT, LED_PIN, GPIO_PIN_RESET);

    /* Initialize Console USART3 on PD8/PD9 @ 115200 8N1 */
    console_init();

    /* Jump to CamelForth COLD start */
    forth_cold();

    /* Should never return */
    while (1) {
    }
}

/**
  * @brief System Clock Configuration (Configures MCU to 168 MHz using 12 MHz HSE + PLL)
  */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* Enable Power Control clock */
    __HAL_RCC_PWR_CLK_ENABLE();

    /* Voltage scaling scale 1 for 168 MHz operation */
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /* Configure HSE and PLL:
     * HSE = 12 MHz
     * PLL_M = 12 (VCO input = 1 MHz)
     * PLL_N = 336 (VCO output = 336 MHz)
     * PLL_P = 2 (SYSCLK = 168 MHz)
     * PLL_Q = 7 (USB/SDIO = 48 MHz)
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 12;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    /* Select PLL as SYSCLK and configure bus dividers:
     * HCLK = SYSCLK / 1 = 168 MHz
     * APB1 = HCLK / 4 = 42 MHz
     * APB2 = HCLK / 2 = 84 MHz
     */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
        Error_Handler();
    }

    /* Enable flash prefetch and caches for maximum execution speed */
    __HAL_FLASH_INSTRUCTION_CACHE_ENABLE();
    __HAL_FLASH_DATA_CACHE_ENABLE();
    __HAL_FLASH_PREFETCH_BUFFER_ENABLE();
}

void Error_Handler(void) {
    __disable_irq();
    while (1) {
    }
}

void SysTick_Handler(void) {
    HAL_IncTick();
}
