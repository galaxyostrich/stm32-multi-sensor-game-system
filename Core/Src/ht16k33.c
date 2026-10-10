#include "ht16k33.h"
#include "string.h"
#include <math.h>

const uint8_t HT16K33_Font[36][8] = {
    // A–Z
    {0x3E,0x09,0x09,0x3E,0x00,0x00,0x00,0x00}, // A
    {0x3F,0x25,0x25,0x1A,0x00,0x00,0x00,0x00}, // B
    {0x1E,0x21,0x21,0x12,0x00,0x00,0x00,0x00}, // C
    {0x3F,0x21,0x21,0x1E,0x00,0x00,0x00,0x00}, // D
    {0x3F,0x25,0x25,0x21,0x00,0x00,0x00,0x00}, // E
    {0x3F,0x05,0x05,0x01,0x00,0x00,0x00,0x00}, // F
    {0x1E,0x21,0x25,0x3D,0x00,0x00,0x00,0x00}, // G
    {0x3F,0x04,0x04,0x3F,0x00,0x00,0x00,0x00}, // H
    {0x21,0x3F,0x21,0x00,0x00,0x00,0x00,0x00}, // I
    {0x10,0x20,0x21,0x1F,0x00,0x00,0x00,0x00}, // J
    {0x3F,0x04,0x0A,0x31,0x00,0x00,0x00,0x00}, // K
    {0x3F,0x20,0x20,0x20,0x00,0x00,0x00,0x00}, // L
    {0x3F,0x02,0x04,0x02,0x3F,0x00,0x00,0x00}, // M
    {0x3F,0x02,0x04,0x3F,0x00,0x00,0x00,0x00}, // N
    {0x1E,0x21,0x21,0x1E,0x00,0x00,0x00,0x00}, // O
    {0x3F,0x09,0x09,0x06,0x00,0x00,0x00,0x00}, // P
    {0x1E,0x21,0x31,0x5E,0x00,0x00,0x00,0x00}, // Q
    {0x3F,0x09,0x19,0x26,0x00,0x00,0x00,0x00}, // R
    {0x26,0x25,0x25,0x19,0x00,0x00,0x00,0x00}, // S
    {0x01,0x01,0x3F,0x01,0x01,0x00,0x00,0x00}, // T
    {0x1F,0x20,0x20,0x1F,0x00,0x00,0x00,0x00}, // U
    {0x0F,0x10,0x20,0x10,0x0F,0x00,0x00,0x00}, // V
    {0x3F,0x10,0x08,0x10,0x3F,0x00,0x00,0x00}, // W
    {0x33,0x0C,0x0C,0x33,0x00,0x00,0x00,0x00}, // X
    {0x03,0x04,0x38,0x04,0x03,0x00,0x00,0x00}, // Y
    {0x31,0x29,0x25,0x23,0x00,0x00,0x00,0x00}, // Z
    // 0–9
    {0x1E,0x21,0x21,0x1E,0x00,0x00,0x00,0x00}, // 0
    {0x22,0x3F,0x20,0x00,0x00,0x00,0x00,0x00}, // 1
    {0x32,0x29,0x29,0x26,0x00,0x00,0x00,0x00}, // 2
    {0x21,0x25,0x25,0x1A,0x00,0x00,0x00,0x00}, // 3
    {0x0C,0x0A,0x3F,0x08,0x00,0x00,0x00,0x00}, // 4
    {0x27,0x25,0x25,0x19,0x00,0x00,0x00,0x00}, // 5
    {0x1E,0x25,0x25,0x18,0x00,0x00,0x00,0x00}, // 6
    {0x01,0x01,0x3D,0x03,0x00,0x00,0x00,0x00}, // 7
    {0x1A,0x25,0x25,0x1A,0x00,0x00,0x00,0x00}, // 8
    {0x06,0x29,0x29,0x1E,0x00,0x00,0x00,0x00}  // 9
};

void HT16K33_Init(I2C_HandleTypeDef *hi2c)
{
    uint8_t cmd;

    cmd = 0x21; // turn on oscillator
    HAL_I2C_Master_Transmit(hi2c, HT16K33_I2C_ADDR, &cmd, 1, HAL_MAX_DELAY);

    cmd = 0x81; // display on, blink off
    HAL_I2C_Master_Transmit(hi2c, HT16K33_I2C_ADDR, &cmd, 1, HAL_MAX_DELAY);

    cmd = 0xE0 | 8; // medium brightness
    HAL_I2C_Master_Transmit(hi2c, HT16K33_I2C_ADDR, &cmd, 1, HAL_MAX_DELAY);
}

void HT16K33_SetBrightness(I2C_HandleTypeDef *hi2c, uint8_t brightness)
{
    uint8_t cmd = 0xE0 | (brightness & 0x0F);
    HAL_I2C_Master_Transmit(hi2c, HT16K33_I2C_ADDR, &cmd, 1, HAL_MAX_DELAY);
}

void HT16K33_Clear(I2C_HandleTypeDef *hi2c)
{
    uint8_t buffer[17];
    memset(buffer, 0, sizeof(buffer));
    buffer[0] = 0x00;
    HAL_I2C_Master_Transmit(hi2c, HT16K33_I2C_ADDR, buffer, sizeof(buffer), HAL_MAX_DELAY);
}

void HT16K33_DisplayRaw(I2C_HandleTypeDef *hi2c, const uint8_t *data)
{
    uint8_t buffer[17];
    buffer[0] = 0x00;
    for (int i = 0; i < 8; i++) {
        buffer[1 + i * 2] = data[i];
        buffer[2 + i * 2] = 0x00;
    }
    HAL_I2C_Master_Transmit(hi2c, HT16K33_I2C_ADDR, buffer, sizeof(buffer), HAL_MAX_DELAY);
}

void HT16K33_DisplayChar(I2C_HandleTypeDef *hi2c, char c)
{
    uint8_t index;
    if (c >= 'A' && c <= 'Z') index = c - 'A';
    else if (c >= '0' && c <= '9') index = 26 + (c - '0');
    else { HT16K33_Clear(hi2c); return; }

    HT16K33_DisplayRaw(hi2c, HT16K33_Font[index]);
}

uint8_t HT16K33_RotateBorder_NonBlocking(I2C_HandleTypeDef *hi2c, uint8_t loops)
{
    static uint8_t loop = 0, i = 0;
    static uint32_t last_update = 0;
    const uint8_t tail_len = 3;

    const uint8_t path[28][2] = {
        {0,0},{1,0},{2,0},{3,0},{4,0},{5,0},{6,0},{7,0},
        {7,1},{7,2},{7,3},{7,4},{7,5},{7,6},{7,7},
        {6,7},{5,7},{4,7},{3,7},{2,7},{1,7},{0,7},
        {0,6},{0,5},{0,4},{0,3},{0,2},{0,1}
    };

    uint32_t now = HAL_GetTick();
    if (now - last_update < 30) return 0;
    last_update = now;

    uint8_t frame[8] = {0};
    for (int t = 0; t < tail_len; t++) {
        int idx = i - t;
        if (idx < 0) continue;
        if (idx >= 28) continue;
        uint8_t x = path[idx][0];
        uint8_t y = path[idx][1];
        frame[x] |= 1 << y;
    }

    HT16K33_DisplayRaw(hi2c, frame);

    i++;
    if (i >= 28 + tail_len) {
        i = 0;
        loop++;
        if (loop >= loops) {
            loop = 0;
            HT16K33_Clear(hi2c);
            return 1;
        }
    }

    return 0;
}

void HT16K33_RadarAlert(I2C_HandleTypeDef *hi2c, float mag_strength)
{
    static uint8_t phase = 0;
    static uint32_t last_update = 0;
    uint32_t now = HAL_GetTick();
    if (now - last_update < 60) return;
    last_update = now;

    uint8_t frame[8] = {0};
    uint8_t brightness = 5 + (uint8_t)(4.0f * sinf(phase * 3.14159f / 32.0f));
    phase = (phase + 1) % 64;

    const uint8_t ring_outer[8] = {0xFF,0x81,0x81,0x81,0x81,0x81,0x81,0xFF};
    const uint8_t ring_middle[8] = {0x7E,0x42,0x42,0x42,0x42,0x42,0x42,0x7E};
    const uint8_t ring_inner[8] = {0x3C,0x24,0x24,0x24,0x24,0x24,0x24,0x3C};
    const uint8_t center_dot[8] = {0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00};

    if (mag_strength < 2200) {
        HT16K33_Clear(hi2c);
        return;
    }
    else if (mag_strength < 2400) {
        memcpy(frame, ring_outer, sizeof(frame));
    }
    else if (mag_strength < 2500) {
        for (int i=0;i<8;i++) frame[i] = ring_outer[i] | ring_middle[i];
    }
    else if (mag_strength < 2800) {
        for (int i=0;i<8;i++) frame[i] = ring_outer[i] | ring_middle[i] | ring_inner[i];
    }
    else {
        for (int i=0;i<8;i++) frame[i] = ring_outer[i] | ring_middle[i] | ring_inner[i] | center_dot[i];
        brightness = (phase % 8 < 4) ? 15 : 2;
    }

    HT16K33_DisplayRaw(hi2c, frame);
    HT16K33_SetBrightness(hi2c, brightness);
}

uint8_t HT16K33_EscapeCountdown_NonBlocking(
    I2C_HandleTypeDef *hi2c,
    uint32_t start_time,
    uint32_t duration_ms,
    volatile uint8_t *escaped_flag
)
{
    static uint32_t last_update = 0;
    static uint8_t phase = 0;

    uint32_t now = HAL_GetTick();
    uint32_t elapsed = now - start_time;

    if (elapsed >= duration_ms) {
        HT16K33_DisplayChar(hi2c, 'X');
        uint8_t brightness = (phase % 4 < 2) ? 15 : 2;
        HT16K33_SetBrightness(hi2c, brightness);
        phase++;
        return 1;
    }

    if (*escaped_flag) {
        HT16K33_DisplayChar(hi2c, 'E');
        HT16K33_SetBrightness(hi2c, 15);
        *escaped_flag = 0;
        return 0;
    }

    if (now - last_update >= 80U) {
        last_update = now;

        uint8_t frame[8];
        uint32_t step_time = duration_ms / 8;
        uint8_t cols_to_off = elapsed / step_time;
        if (cols_to_off > 8) cols_to_off = 8;

        for (int col = 0; col < 8; col++) {
            frame[col] = (col >= 8 - cols_to_off) ? 0x00 : 0xFF;
        }

        HT16K33_DisplayRaw(hi2c, frame);

        uint8_t brightness =
            6 + (uint8_t)(5.0f * sinf(phase * 3.14159f / 16.0f));
        HT16K33_SetBrightness(hi2c, brightness);
        phase = (phase + 1) % 32;
    }

    return 2;
}
