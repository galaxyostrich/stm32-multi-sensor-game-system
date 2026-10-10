#include "main.h"
#include "stm32l4xx_hal_rcc_ex.h"
#include "stdio.h"
#include "string.h"
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include "i2c.h"
#include "ht16k33.h"
#include "stm32l4xx_hal_adc.h"
#include "../../Drivers/BSP/B-L4S5I-IOT01/stm32l4s5i_iot01_accelero.h"
#include "../../Drivers/BSP/B-L4S5I-IOT01/stm32l4s5i_iot01_tsensor.h"
#include "../../Drivers/BSP/B-L4S5I-IOT01/stm32l4s5i_iot01_psensor.h"
#include "../../Drivers/BSP/B-L4S5I-IOT01/stm32l4s5i_iot01_hsensor.h"
#include "../../Drivers/BSP/B-L4S5I-IOT01/stm32l4s5i_iot01_magneto.h"
#include "../../Drivers/BSP/B-L4S5I-IOT01/stm32l4s5i_iot01_gyro.h"

volatile uint8_t escape_pressed = 0;
volatile uint8_t paused = 0;
volatile uint8_t count = 0;
volatile uint32_t last_press_time = 0;
volatile uint8_t game_over = 0;
volatile uint8_t mode_switched = 0;
volatile uint8_t btn_event = 0;   // 0:no event, 2:double click detected

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) { Error_Handler(); }

    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
    PeriphClkInit.PeriphClockSelection     = RCC_PERIPHCLK_USART1 | RCC_PERIPHCLK_I2C1 | RCC_PERIPHCLK_ADC;
    PeriphClkInit.Usart1ClockSelection     = RCC_USART1CLKSOURCE_PCLK2;
    PeriphClkInit.I2c1ClockSelection       = RCC_I2C1CLKSOURCE_PCLK1;
    PeriphClkInit.AdcClockSelection        = RCC_ADCCLKSOURCE_SYSCLK;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) { Error_Handler(); }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) { Error_Handler(); }
}
void Error_Handler(void)
{
    __disable_irq();
    while (1);
}

//  Define and initialize GPIO ports
#define BUZZER_PORT   GPIOB
#define BUZZER_PIN    GPIO_PIN_4
ADC_HandleTypeDef hadc1;
#define SOUND_ADC_GPIO_PORT   GPIOA
#define SOUND_ADC_PIN         GPIO_PIN_0
#define SOUND_ADC_CHANNEL     ADC_CHANNEL_14
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    HAL_GPIO_WritePin(GPIOB, LED2_Pin, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = LED2_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = BUTTON_EXTI13_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = BUZZER_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BUZZER_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin  = SOUND_ADC_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG_ADC_CONTROL;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(SOUND_ADC_GPIO_PORT, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

static void MX_ADC1_Init(void)
{
    __HAL_RCC_ADC_CLK_ENABLE();

    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler        = ADC_CLOCK_ASYNC_DIV1;
    hadc1.Init.Resolution            = ADC_RESOLUTION_12B;
    hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc1.Init.ScanConvMode          = ADC_SCAN_DISABLE;
    hadc1.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    hadc1.Init.LowPowerAutoWait      = DISABLE;
    hadc1.Init.ContinuousConvMode    = DISABLE;
    hadc1.Init.NbrOfConversion       = 1;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    hadc1.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.Overrun               = ADC_OVR_DATA_PRESERVED;
    if (HAL_ADC_Init(&hadc1) != HAL_OK) { Error_Handler(); }

    HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);

    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel      = SOUND_ADC_CHANNEL;
    sConfig.Rank         = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_47CYCLES_5;
    sConfig.SingleDiff   = ADC_SINGLE_ENDED;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset       = 0;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) { Error_Handler(); }
}

UART_HandleTypeDef huart1;
char uart_buf[128];
static void UART1_Init(void)
{
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    GPIO_InitStruct.Pin = GPIO_PIN_7 | GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&huart1) != HAL_OK)
    {
        while (1);
    }
}

static void Sensors_Init(void)
{
    BSP_ACCELERO_Init();
    BSP_GYRO_Init();
    BSP_MAGNETO_Init();
    BSP_TSENSOR_Init();
    BSP_PSENSOR_Init();
    BSP_HSENSOR_Init();
}

//  This is the block for configuring the user push button
#define BTN_DEBOUNCE_MS    40U
#define DOUBLE_CLICK_MS    500U
#define SINGLE_CLICK_MS    (DOUBLE_CLICK_MS + 50U)
void loop_check_press(void){
	if (count == 1){
		uint32_t now = HAL_GetTick();
		if (now - last_press_time > SINGLE_CLICK_MS){
			count = 0;
			sprintf(uart_buf, ">> Single press detected.\r\n");
			HAL_UART_Transmit(&huart1, (uint8_t*)uart_buf, strlen(uart_buf), 0xFFFF);
			escape_pressed = 1;
		}
	}
}
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if (GPIO_Pin == BUTTON_EXTI13_Pin)
	{
		uint32_t now = HAL_GetTick();

		if (now - last_press_time < BTN_DEBOUNCE_MS) return;

		if (count == 0)
		{
			count = 1;
			last_press_time = now;
		}

		else if (count == 1) {
			if (now - last_press_time <= DOUBLE_CLICK_MS)
			{
				paused ^= 1;
				count = 0;
				game_over = 0;
				mode_switched = 1;
				btn_event = 2;
			} else
			{
				last_press_time = now;
			}
		}
	}
}

//  This is the block for configuring the buzzer
volatile uint8_t buzzer_on = 0;
volatile uint32_t buzzer_start_time = 0;
void Buzzer_On(void){
    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_SET);
}
void Buzzer_Off(void){
    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
}
void Buzzer_Update(void){
    if (buzzer_on && (HAL_GetTick() - buzzer_start_time >= 1000U)){
        Buzzer_Off();
        buzzer_on = 0;
    }
}

//  This is the block for configuring the sound sensor
static uint8_t Sound_Poll(uint32_t threshold, uint32_t refractory_ms)
{
    const uint32_t SAMPLE_INTERVAL_MS = 8;
    static uint32_t last_sample_ms  = 0;
    static uint32_t last_trigger_ms = 0;
    static uint32_t base            = 0;
    static uint8_t  inited          = 0;
    enum { ADC_IDLE, ADC_STARTED };
    static uint8_t adc_state = ADC_IDLE;

    uint32_t now = HAL_GetTick();
    if (now - last_sample_ms < SAMPLE_INTERVAL_MS) return 0;
    last_sample_ms = now;

    if (adc_state == ADC_IDLE) {
        if (HAL_ADC_Start(&hadc1) == HAL_OK) adc_state = ADC_STARTED;
        return 0;
    }

    if (__HAL_ADC_GET_FLAG(&hadc1, ADC_FLAG_EOC)) {
        uint32_t v = HAL_ADC_GetValue(&hadc1);
        HAL_ADC_Stop(&hadc1);
        adc_state = ADC_IDLE;

        if (!inited) { base = v; inited = 1; }
        else         { base = (base * 15 + v) / 16; }

        if (v > base + threshold && (now - last_trigger_ms) > refractory_ms) {
            last_trigger_ms = now;
            return 1;
        }
    }
    return 0;
}

//  The main logic for Game 1
static void Game_1(void)
{
    static const uint32_t GREEN_DURATION     = 10000U;
    static const uint32_t RED_DURATION       = 10000U;
    static const int32_t  MOVE_THRESHOLD     = 2000;
    static const float    GYRO_LIMIT_XZ      = 1500.0f;
    static const float    GYRO_LIMIT_Y       = 2500.0f;
    static const uint32_t ENV_POLL_INTERVAL  = 2000U;
    static const uint32_t LED_BLINK_INTERVAL = 500U;

    static uint8_t  inited           = 0;
    static uint8_t  red_light        = 0;
    static uint32_t last_change_time = 0;
    static uint32_t env_last_time    = 0;
    static uint32_t led_last_time    = 0;
    static uint32_t move_last_time   = 0;
    static uint8_t  state            = 0;

    if (!inited || mode_switched)
    {
        inited = 1;
        mode_switched = 0;
        red_light = 0;
        last_change_time = HAL_GetTick();
        state = 0;

        HT16K33_Init(&hi2c1);
        HT16K33_Clear(&hi2c1);

        sprintf(uart_buf, "Entering Red Light, Green Light as Player\r\n");
        HAL_UART_Transmit(&huart1, (uint8_t*)uart_buf, strlen(uart_buf), 0xFFFF);
    }

    if (state == 0) {
        if (HT16K33_RotateBorder_NonBlocking(&hi2c1, 2)) {
            HT16K33_DisplayChar(&hi2c1, 'G');
            sprintf(uart_buf, "Green Light!\r\n");
            HAL_UART_Transmit(&huart1, (uint8_t*)uart_buf, strlen(uart_buf), 0xFFFF);
            state = 1;
        }
        return;
    }

    // This part detects any sound during Game 1
    uint32_t now = HAL_GetTick();
    if (Sound_Poll(1500, 300)) {
        if (!red_light) {
            // If green light now, then switch to red light
            red_light = 1;
            last_change_time = now;

            sprintf(uart_buf, "Red Light! Because of your sound!\r\n");
            HAL_UART_Transmit(&huart1, (uint8_t*)uart_buf, strlen(uart_buf), 0xFFFF);
            HT16K33_DisplayChar(&hi2c1, 'R');
        } else {
            // If red light now, then game over
            sprintf(uart_buf, "Game Over! We detected your sound during Red Light!\r\n");
            HAL_UART_Transmit(&huart1, (uint8_t*)uart_buf, strlen(uart_buf), 0xFFFF);

            HT16K33_DisplayChar(&hi2c1, 'X');
            HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);

            game_over = 1;
            Buzzer_On(); buzzer_on = 1; buzzer_start_time = HAL_GetTick();
            return;
        }
    }

    if(!red_light && now - last_change_time >= GREEN_DURATION) {
        red_light = 1;
        last_change_time = now;
        sprintf(uart_buf, "Red Light!\r\n");
        HAL_UART_Transmit(&huart1, (uint8_t*)uart_buf, strlen(uart_buf), 0xFFFF);
        HT16K33_DisplayChar(&hi2c1, 'R');

    }
    else if(red_light && now - last_change_time >= RED_DURATION) {
        red_light = 0;
        last_change_time = now;
        sprintf(uart_buf, "Green Light!\r\n");
        HAL_UART_Transmit(&huart1, (uint8_t*)uart_buf, strlen(uart_buf), 0xFFFF);
        HT16K33_DisplayChar(&hi2c1, 'G');
    }

    if(!red_light) {
        HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);

        if(now - env_last_time >= ENV_POLL_INTERVAL) {
            float temp = BSP_TSENSOR_ReadTemp();
            float hum  = BSP_HSENSOR_ReadHumidity();
            float pres = BSP_PSENSOR_ReadPressure();
            sprintf(uart_buf, "Temp: %.1fC Hum: %.1f%% Press: %.1fhPa\r\n", temp, hum, pres);
            HAL_UART_Transmit(&huart1, (uint8_t*)uart_buf, strlen(uart_buf), 0xFFFF);
            env_last_time = now;
        }
    }
    else {
        if (now - led_last_time >= LED_BLINK_INTERVAL) {
            HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
            led_last_time = now;
        }

        if (now - move_last_time >= ENV_POLL_INTERVAL) {
            int16_t acc_data[3];
            float gyro_data[3];
            BSP_ACCELERO_AccGetXYZ(acc_data);
            BSP_GYRO_GetXYZ(gyro_data);

            sprintf(uart_buf, "Acc: X=%d Y=%d Z=%d | Gyro: X=%.1f Y=%.1f Z=%.1f\r\n",
                    acc_data[0], acc_data[1], acc_data[2],
                    gyro_data[0], gyro_data[1], gyro_data[2]);
            HAL_UART_Transmit(&huart1, (uint8_t*)uart_buf, strlen(uart_buf), 0xFFFF);

            if (abs(acc_data[0])   >  MOVE_THRESHOLD ||
                abs(acc_data[1])   >  MOVE_THRESHOLD ||
                abs(acc_data[2])   >  MOVE_THRESHOLD ||
                fabs(gyro_data[0]) >  GYRO_LIMIT_XZ  ||
                fabs(gyro_data[1]) >  GYRO_LIMIT_Y   ||
                fabs(gyro_data[2]) >  GYRO_LIMIT_XZ)
            {
                sprintf(uart_buf, "Game Over! You moved during Red Light!\r\n");
                HAL_UART_Transmit(&huart1, (uint8_t*)uart_buf, strlen(uart_buf), 0xFFFF);

                HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
                HT16K33_DisplayChar(&hi2c1, 'X');
                game_over = 1;

                Buzzer_On();
                buzzer_on = 1;
                buzzer_start_time = HAL_GetTick();

                return;
            }
            move_last_time = now;
        }
    }
}

//  The main logic for Game 2
void Game_2(void)
{
	//  Environment Sensor Thresholds (unit: Degree Celsius, %, hPa)
    static const float TEMP_SPIKE   = 31.0f;
    static const float HUM_SPIKE    = 65.0f;
    static const float PRES_SPIKE   = 1000.0f;

	//  Magnetic Field Strength Levels (unit: raw ADC sum)
	static const float MAG_OFF_TH_F   = 2200.0f;
	static const float MAG_LOW_TH_F   = 2400.0f;
	static const float MAG_MED_TH_F   = 2500.0f;
	static const float MAG_ALERT_TH_F = 2800.0f;

	//  LED Blink Periods (unit: ms)
    static const uint32_t LED_LOW  = 1000U;
    static const uint32_t LED_MED  = 500U;
    static const uint32_t LED_HIGH = 250U;

	//  Game Timing Windows (unit: ms)
    static const uint32_t ENV_POLL_INTERVAL_MS  = 1000U;   // sensor read frequency
    static const uint32_t ALERT_WINDOW_MS       = 3000U;   // escape window duration

	//  Runtime Variables (persistent between calls)
    static uint8_t  inited            = 0;
    static uint32_t env_last_time     = 0;
    static uint32_t led_last_time     = 0;
	static uint8_t  alert_triggered   = 0;

	extern ADC_HandleTypeDef hadc1;

	//  Initialization Block
    if (!inited || mode_switched) {
        inited = 1;
        mode_switched = 0;
		env_last_time = led_last_time = 0;
		alert_triggered = 0;
		escape_pressed = 0;

        HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
        sprintf(uart_buf, "Entering Catch And Run as Player\r\n");
        HAL_UART_Transmit(&huart1, (uint8_t*)uart_buf, strlen(uart_buf), 0xFFFF);
    }

	uint32_t current_time = HAL_GetTick();

	//  Environment Monitoring
	if(current_time - env_last_time >= ENV_POLL_INTERVAL_MS) {
		float temp = BSP_TSENSOR_ReadTemp();
	    float hum  = BSP_HSENSOR_ReadHumidity();
	    float pres = BSP_PSENSOR_ReadPressure();

		if (temp > TEMP_SPIKE) {
			sprintf(uart_buf, "Temperature spike detected! T: %.1fC. Dangerous environment!\r\n", temp);
			HAL_UART_Transmit(&huart1, (uint8_t*)uart_buf, strlen(uart_buf), 0xFFFF);
		}
		if (hum > HUM_SPIKE) {
			sprintf(uart_buf, "Humidity spike detected! H: %.1f%%. Dangerous environment!\r\n", hum);
			HAL_UART_Transmit(&huart1, (uint8_t*)uart_buf, strlen(uart_buf), 0xFFFF);
		}
        if (pres > PRES_SPIKE) {
			sprintf(uart_buf, "Pressure spike detected! P: %.1fhPa. Dangerous environment!\r\n", pres);
			HAL_UART_Transmit(&huart1, (uint8_t*)uart_buf, strlen(uart_buf), 0xFFFF);
		}
		env_last_time = current_time;
	}

	//  Magnetic Field and LED Behaviour
	int16_t mag[3];
	BSP_MAGNETO_GetXYZ(mag);
	float mx = (float)mag[0], my = (float)mag[1], mz = (float)mag[2];
	float mag_strength = sqrtf(mx*mx + my*my + mz*mz);

	//  Real-time radar strength
    if(!alert_triggered){
        HT16K33_RadarAlert(&hi2c1, mag_strength);
    }

	uint32_t blink_period = 0;
	if (mag_strength < MAG_OFF_TH_F) {
		HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
	}
	else if (mag_strength < MAG_LOW_TH_F) {
		blink_period = LED_LOW;
	}
	else if (mag_strength < MAG_MED_TH_F) {
		blink_period = LED_MED;
	}
	else {
		blink_period = LED_HIGH;
	}

	if (blink_period > 0 && !alert_triggered && current_time - led_last_time >= blink_period) {
		HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
		led_last_time = current_time;
	}

	//  Alert Trigger and Escape Mechanism
	static uint8_t countdown_active = 0;
	static uint32_t countdown_start_time = 0;
    static volatile uint8_t escape_count = 0;
    static uint8_t escape_reason = 0;

	if (!alert_triggered) {
	    if (mag_strength > MAG_ALERT_TH_F) {
	        alert_triggered = 1;
	        countdown_active = 1;
	        countdown_start_time = HAL_GetTick();
            escape_count = 0;
            escape_reason = 0;

	        sprintf(uart_buf, "Enforcer nearby! Be careful.\r\n");
	        HAL_UART_Transmit(&huart1, (uint8_t*)uart_buf, strlen(uart_buf), 0xFFFF);
	    }
	}

	if (countdown_active) {
	    if (escape_pressed) {
	        escape_count = 1;
	        escape_pressed = 0;
            if (escape_reason == 0) escape_reason = 1;
	    } else if (Sound_Poll(1800, 300)) {
	        escape_count = 1;
            if (escape_reason == 0) escape_reason = 2;
	    }

		uint8_t result = HT16K33_EscapeCountdown_NonBlocking(
		    &hi2c1,
		    countdown_start_time,
		    ALERT_WINDOW_MS,
		    &escape_count
		);

	    if (result == 0) {
            if (escape_reason == 1){
                sprintf(uart_buf, "Player escaped by button! Good job!\r\n");
            }
            else if (escape_reason == 2){
                sprintf(uart_buf, "The hero heard your voice and saved you!\r\n");
            }
            else {
                sprintf(uart_buf, "Player escaped, good job!\r\n");
            }
            HAL_UART_Transmit(&huart1, (uint8_t*)uart_buf, strlen(uart_buf), 0xFFFF);

	        countdown_active = 0;
	        alert_triggered = 0;
	        HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
            escape_reason = 0;
	    }
	    else if (result == 1) {
	        sprintf(uart_buf, "Game Over! You failed to escape.\r\n");
	        HAL_UART_Transmit(&huart1, (uint8_t*)uart_buf, strlen(uart_buf), 0xFFFF);
	        countdown_active = 0;
	        alert_triggered = 0;
            escape_reason = 0;

	        game_over = 1;
	        Buzzer_On();
	        buzzer_on = 1;
	        buzzer_start_time = HAL_GetTick();
	    }
	}
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    UART1_Init();
    MX_ADC1_Init();
    MX_I2C1_Init();
    Sensors_Init();

    while (1)
    {
    	Buzzer_Update();
        loop_check_press();

        if (btn_event == 2) {
            sprintf(uart_buf, ">> Double press detected.\r\n");
            HAL_UART_Transmit(&huart1, (uint8_t*)uart_buf, strlen(uart_buf), 0xFFFF);
            btn_event = 0;
        }

        if (game_over) {
            static uint32_t last_blink = 0;
            uint32_t t = HAL_GetTick();
            if (t - last_blink >= 1000U) {
                HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
                last_blink = t;
            }
            continue;
        }

        if (paused == 0)
            Game_1();
        else
            Game_2();
    }
}
