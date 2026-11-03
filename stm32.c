
/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : HC-SR04 Ultrasonic Distance Measurement
  *                   STM32 Nucleo-F446RE
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdio.h>
#include <string.h>

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
void Error_Handler(void);

/* USER CODE BEGIN 0 */

/* HC-SR04 Pin Configuration */
#define TRIG_PORT GPIOB
#define TRIG_PIN  GPIO_PIN_10

#define ECHO_PORT GPIOC
#define ECHO_PIN  GPIO_PIN_7

#define ECHO_TIMEOUT_US 30000U

/* Initialize DWT cycle counter */
static void DWT_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    DWT->CYCCNT = 0;

    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

/* Microsecond delay */
static void delay_us(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;

    uint32_t cycles =
        (SystemCoreClock / 1000000U) * us;

    while ((uint32_t)(DWT->CYCCNT - start) < cycles)
    {
    }
}

/* Wait for ECHO pin state with timeout */
static uint8_t wait_for_echo(GPIO_PinState state,
                             uint32_t timeout_us)
{
    uint32_t start = DWT->CYCCNT;

    uint32_t timeout_cycles =
        (SystemCoreClock / 1000000U) * timeout_us;

    while (HAL_GPIO_ReadPin(ECHO_PORT, ECHO_PIN) != state)
    {
        if ((uint32_t)(DWT->CYCCNT - start) >= timeout_cycles)
        {
            return 0;
        }
    }

    return 1;
}

/* Measure ultrasonic distance */
static int32_t measure_distance_cm_x100(void)
{
    uint32_t start;
    uint32_t duration_cycles;
    uint32_t cycles_per_us;
    uint32_t duration_us;

    /* Ensure TRIG is LOW */
    HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_RESET);

    delay_us(3);

    /* Send 10 microsecond trigger pulse */
    HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_SET);

    delay_us(10);

    HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_RESET);

    /* Wait for ECHO HIGH */
    if (!wait_for_echo(GPIO_PIN_SET, ECHO_TIMEOUT_US))
    {
        return -1;
    }

    /* Start timing */
    start = DWT->CYCCNT;

    /* Wait for ECHO LOW */
    if (!wait_for_echo(GPIO_PIN_RESET, ECHO_TIMEOUT_US))
    {
        return -1;
    }

    /* Calculate pulse duration */
    duration_cycles = DWT->CYCCNT - start;

    cycles_per_us = SystemCoreClock / 1000000U;

    duration_us =
        (duration_cycles + cycles_per_us / 2U) / cycles_per_us;

    /* Convert time into distance in 0.01 cm units */
    return (int32_t)((duration_us * 1715U + 500U) / 1000U);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  */
int main(void)
{
    HAL_Init();

    SystemClock_Config();

    MX_GPIO_Init();

    MX_USART2_UART_Init();

    /* Initialize DWT timer */
    DWT_Init();

    /* USER CODE BEGIN 2 */

    char message[80];

    const char startup[] =
        "\r\nSTM32 NUCLEO-F446RE\r\n"
        "HC-SR04 Distance Measurement Started\r\n";

    HAL_UART_Transmit(&huart2,
                      (uint8_t *)startup,
                      strlen(startup),
                      HAL_MAX_DELAY);

    /* USER CODE END 2 */

    /* Infinite loop */
    while (1)
    {
        /* USER CODE BEGIN WHILE */

        int32_t distance = measure_distance_cm_x100();

        if (distance < 0)
        {
            snprintf(message,
                     sizeof(message),
                     "No Echo Detected\r\n");
        }
        else
        {
            snprintf(message,
                     sizeof(message),
                     "Distance: %ld.%02ld cm\r\n",
                     (long)(distance / 100),
                     (long)(distance % 100));
        }

        HAL_UART_Transmit(&huart2,
                          (uint8_t *)message,
                          strlen(message),
                          HAL_MAX_DELAY);

        HAL_Delay(500);

        /* USER CODE END WHILE */
    }
}

/**
  * @brief System Clock Configuration
  */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();

    __HAL_PWR_VOLTAGESCALING_CONFIG(
        PWR_REGULATOR_VOLTAGE_SCALE3);

    RCC_OscInitStruct.OscillatorType =
        RCC_OSCILLATORTYPE_HSI;

    RCC_OscInitStruct.HSIState = RCC_HSI_ON;

    RCC_OscInitStruct.HSICalibrationValue =
        RCC_HSICALIBRATION_DEFAULT;

    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;

    RCC_OscInitStruct.PLL.PLLSource =
        RCC_PLLSOURCE_HSI;

    RCC_OscInitStruct.PLL.PLLM = 16;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
    RCC_OscInitStruct.PLL.PLLQ = 2;
    RCC_OscInitStruct.PLL.PLLR = 2;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_HCLK |
        RCC_CLOCKTYPE_SYSCLK |
        RCC_CLOCKTYPE_PCLK1 |
        RCC_CLOCKTYPE_PCLK2;

    RCC_ClkInitStruct.SYSCLKSource =
        RCC_SYSCLKSOURCE_PLLCLK;

    RCC_ClkInitStruct.AHBCLKDivider =
        RCC_SYSCLK_DIV1;

    RCC_ClkInitStruct.APB1CLKDivider =
        RCC_HCLK_DIV2;

    RCC_ClkInitStruct.APB2CLKDivider =
        RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct,
                            FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief USART2 Initialization Function
  */
static void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2;

    huart2.Init.BaudRate = 115200;

    huart2.Init.WordLength = UART_WORDLENGTH_8B;

    huart2.Init.StopBits = UART_STOPBITS_1;

    huart2.Init.Parity = UART_PARITY_NONE;

    huart2.Init.Mode = UART_MODE_TX_RX;

    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;

    huart2.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart2) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief GPIO Initialization Function
  */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Enable GPIO clocks */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* Set TRIG LOW initially */
    HAL_GPIO_WritePin(GPIOB,
                      GPIO_PIN_10,
                      GPIO_PIN_RESET);

    /* Configure PB10 - TRIG */
    GPIO_InitStruct.Pin = GPIO_PIN_10;

    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;

    GPIO_InitStruct.Pull = GPIO_NOPULL;

    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* Configure PC7 - ECHO */
    GPIO_InitStruct.Pin = GPIO_PIN_7;

    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;

    GPIO_InitStruct.Pull = GPIO_NOPULL;

    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

/**
  * @brief Error Handler
  */
void Error_Handler(void)
{
    __disable_irq();

    while (1)
    {
    }
}

#ifdef USE_FULL_ASSERT

void assert_failed(uint8_t *file, uint32_t line)
{
}

#endif
