#ifndef __HT16K33_H__
#define __HT16K33_H__

#include "main.h"

#define HT16K33_I2C_ADDR   (0x70 << 1)


void HT16K33_Init(I2C_HandleTypeDef *hi2c);
void HT16K33_SetBrightness(I2C_HandleTypeDef *hi2c, uint8_t brightness);
void HT16K33_Clear(I2C_HandleTypeDef *hi2c);
void HT16K33_DisplayRaw(I2C_HandleTypeDef *hi2c, const uint8_t *data);
void HT16K33_DisplayChar(I2C_HandleTypeDef *hi2c, char c);
uint8_t HT16K33_RotateBorder_NonBlocking(I2C_HandleTypeDef *hi2c, uint8_t loops);
void HT16K33_RadarAlert(I2C_HandleTypeDef *hi2c, float mag_strength);
uint8_t HT16K33_EscapeCountdown_NonBlocking(
    I2C_HandleTypeDef *hi2c,
    uint32_t start_time,
    uint32_t duration_ms,
    volatile uint8_t *escaped_flag
);

extern const uint8_t HT16K33_Font[36][8];

#endif
