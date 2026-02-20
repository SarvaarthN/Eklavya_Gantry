#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "driver/gpio.h"

// --- PIN MAPPING FOR WROVER & HW-504 ---
// ADC1_CHANNEL_6 → GPIO 34 (VRx)
// ADC1_CHANNEL_7 → GPIO 35 (VRy)  
// GPIO 25        → SW (Button)
#define JOY_X_CHAN  ADC1_CHANNEL_6
#define JOY_Y_CHAN  ADC1_CHANNEL_7
#define JOY_SW_PIN  GPIO_NUM_25

bool buttonState = false;  // Tracks toggle state (false=DOWN, true=UP)
bool lastButtonReading = 1;  // For edge detection (1=not pressed)

void app_main(void) {
    // ── 1. ADC Configuration ─────────────────────────────────────────────────
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(JOY_X_CHAN, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(JOY_Y_CHAN, ADC_ATTEN_DB_11);

    // ── 2. Button Configuration ──────────────────────────────────────────────
    gpio_config_t btn_cfg = {
        .pin_bit_mask = (1ULL << JOY_SW_PIN),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&btn_cfg);

    // ── 3. Calibration ───────────────────────────────────────────────────────
    printf("Calibrating... Keep joystick centered.\n");
    vTaskDelay(pdMS_TO_TICKS(2000));

    int centerX = 0, centerY = 0;
    for (int i = 0; i < 16; i++) {
        centerX += adc1_get_raw(JOY_X_CHAN);
        centerY += adc1_get_raw(JOY_Y_CHAN);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    centerX /= 16;
    centerY /= 16;
    printf("Center locked → X: %d  Y: %d\n\n", centerX, centerY);

    // ── 4. Main Loop ─────────────────────────────────────────────────────────
    while (1) {
        // Read raw ADC
        int xRaw = adc1_get_raw(JOY_X_CHAN);
        int yRaw = adc1_get_raw(JOY_Y_CHAN);
        int btnRaw = gpio_get_level(JOY_SW_PIN);  // 0 = pressed

        // Map to –100 … +100
        int xVal = (xRaw - centerX) / 20;
        int yVal = (centerY - yRaw) / 20;

        // Clamp to ±100
        if (xVal >  100) xVal =  100;
        if (xVal < -100) xVal = -100;
        if (yVal >  100) yVal =  100;
        if (yVal < -100) yVal = -100;

        // Deadzone
        if (xVal > -10 && xVal < 10) xVal = 0;
        if (yVal > -10 && yVal < 10) yVal = 0;

        // Joystick direction (Y priority)
        const char *dir = "CENTER  ";
        if (yVal >  30) dir = "FORWARD ";
        else if (yVal < -30) dir = "BACKWARD";
        else if (xVal >  30) dir = "RIGHT   ";
        else if (xVal < -30) dir = "LEFT    ";

        // **Live dashboard** - shows current button toggle state
        printf(" X: %+4d | Y: %+4d | JOY: %s | BTN: %d  \n ",
               xVal, yVal, dir, btnRaw);

        fflush(stdout);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
