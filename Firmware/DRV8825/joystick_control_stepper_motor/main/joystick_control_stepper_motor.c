#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "esp_adc/adc_oneshot.h"


// --- JOYSTICK PINS ---
// ADC1_CHANNEL_6 → GPIO 34 (VRx)
// ADC1_CHANNEL_7 → GPIO 35 (VRy)
// GPIO 25 → SW (Button)
#define JOY_X_CHAN      ADC_CHANNEL_6
#define JOY_Y_CHAN      ADC_CHANNEL_7
#define JOY_SW_PIN      GPIO_NUM_32


// --- STEPPER MOTOR 1 (X-axis: LEFT/RIGHT) ---
//#define STEP_PIN_1      GPIO_NUM_22
// #define DIR_PIN_1       GPIO_NUM_15

#define STEP_PIN_1    GPIO_NUM_18
#define DIR_PIN_1     GPIO_NUM_19

// --- STEPPER MOTOR 2 (Y-axis: FORWARD/BACKWARD) ---
#define STEP_PIN_2      GPIO_NUM_4
#define DIR_PIN_2       GPIO_NUM_25

// --- STEPPER TIMING ---
//#define  microsteps 32
#define STEP_HIGH_US   2 // 1.9ms minimum HIGH pulse (in microseconds)
#define BASE_PERIOD_US  (50000 /32   ) // Base step period at full speed (50ms)
#define MIN_PERIOD_US   (5000/32)  // Minimum step period at max joystick (5ms = fastest)

// Global variables
static int centerX = 0;
static int centerY = 0;
static adc_oneshot_unit_handle_t adc1_handle = NULL;

// ──────────────────────────────────────────────
// Setup all GPIO pins
// ──────────────────────────────────────────────
void setup_pins(void) {
    // Stepper motor output pins
    gpio_config_t io_conf = {
        .intr_type    = GPIO_INTR_DISABLE,
        .mode         = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << STEP_PIN_1) | (1ULL << DIR_PIN_1) |
                        (1ULL << STEP_PIN_2) | (1ULL << DIR_PIN_2),
        .pull_down_en = 0,
        .pull_up_en   = 0
    };
    gpio_config(&io_conf);

    // Ensure all stepper pins start LOW
    gpio_set_level(STEP_PIN_1, 0);
    gpio_set_level(DIR_PIN_1,  0);
    gpio_set_level(STEP_PIN_2, 0);
    gpio_set_level(DIR_PIN_2,  0);

    // Button input pin with internal pull-up
    gpio_config_t btn_cfg = {
        .pin_bit_mask = (1ULL << JOY_SW_PIN),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&btn_cfg);
}

// ──────────────────────────────────────────────
// Initialize ADC
// ──────────────────────────────────────────────
void adc_init(void) {
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    adc_oneshot_new_unit(&init_config, &adc1_handle);

    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_11,
        .bitwidth = ADC_BITWIDTH_12,
    };
    adc_oneshot_config_channel(adc1_handle, JOY_X_CHAN, &config);
    adc_oneshot_config_channel(adc1_handle, JOY_Y_CHAN, &config);
}

// ──────────────────────────────────────────────
// Calibrate joystick center position
// ──────────────────────────────────────────────
void calibrate_joystick(void) {
    printf("Calibrating... Keep joystick centered.\n");
    vTaskDelay(pdMS_TO_TICKS(2000));

    int sumX = 0, sumY = 0;
    int adc_reading = 0;
    for (int i = 0; i < 16; i++) {
        adc_oneshot_read(adc1_handle, JOY_X_CHAN, &adc_reading);
        sumX += adc_reading;
        adc_oneshot_read(adc1_handle, JOY_Y_CHAN, &adc_reading);
        sumY += adc_reading;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    centerX = sumX / 16;
    centerY = sumY / 16;
    printf("Center locked → X: %d  Y: %d\n\n", centerX, centerY);
}

// ──────────────────────────────────────────────
// Send ONE step pulse to a motor
// ──────────────────────────────────────────────
static inline void do_step(gpio_num_t step_pin) {
    gpio_set_level(step_pin, 1);
    esp_rom_delay_us(STEP_HIGH_US);  // Modern replacement for ets_delay_us
    gpio_set_level(step_pin, 0);
}

// ──────────────────────────────────────────────
// Map joystick magnitude (0–100) → step period
// Higher joystick deflection = shorter period = faster motor
// Returns period in microseconds, or 0 if below deadzone
// ──────────────────────────────────────────────
static int magnitude_to_period_us(int magnitude) {
    if (magnitude < 10) return 0; // deadzone → stop

    // Clamp magnitude to 10–100
    if (magnitude > 100) magnitude = 100;

    // Linear interpolation:
    // magnitude=10  → BASE_PERIOD_US (slow)
    // magnitude=100 → MIN_PERIOD_US  (fast)
    int period = BASE_PERIOD_US - ((BASE_PERIOD_US - MIN_PERIOD_US) * (magnitude - 10)) / 90;
    return period;
}

// ──────────────────────────────────────────────
// app_main — main entry point
// ──────────────────────────────────────────────
void app_main(void) {
    setup_pins();
    adc_init();
    calibrate_joystick();

    printf("Controls:\n");
    printf("  LEFT / RIGHT  → Motor 1 (STEP_PIN_1 / DIR_PIN_1)\n");
    printf("  FWD  / BACK   → Motor 2 (STEP_PIN_2 / DIR_PIN_2)\n\n");

    while (1) {
        // ── Read joystick ───────────────────────────────
        int adc_reading = 0;
        adc_oneshot_read(adc1_handle, JOY_X_CHAN, &adc_reading);
        int xRaw = adc_reading;
        adc_oneshot_read(adc1_handle, JOY_Y_CHAN, &adc_reading);
        int yRaw = adc_reading;
        int btnRaw = gpio_get_level(JOY_SW_PIN); // 0 = pressed

        // Map to –100 … +100
        int xVal = (xRaw - centerX) / 20;
        int yVal = (centerY - yRaw) / 20;

        // Clamp
        if (xVal >  100) xVal =  100;
        if (xVal < -100) xVal = -100;
        if (yVal >  100) yVal =  100;
        if (yVal < -100) yVal = -100;

        // ── Determine direction & magnitude ─────────────
        // Motor 1: controlled by X-axis (LEFT / RIGHT)
        int xMag = (xVal < 0) ? -xVal : xVal;  // absolute value
        int xDir = (xVal >= 0) ? 1 : 0;         // 1=RIGHT, 0=LEFT

        // Motor 2: controlled by Y-axis (FORWARD / BACKWARD)
        int yMag = (yVal < 0) ? -yVal : yVal;
        int yDir = (yVal >= 0) ? 1 : 0;          // 1=FORWARD, 0=BACKWARD

        // ── Set direction pins ──────────────────────────
        gpio_set_level(DIR_PIN_1, xDir);
        gpio_set_level(DIR_PIN_2, yDir);

        // ── Convert magnitude to step period ────────────
        int periodX_us = magnitude_to_period_us(xMag); // 0 = stop
        int periodY_us = magnitude_to_period_us(yMag);

        // ── Direction label for dashboard ───────────────
        const char *xLabel = (xMag < 10) ? "STOP " : (xDir ? "RIGHT" : "LEFT ");
        const char *yLabel = (yMag < 10) ? "STOP " : (yDir ? "FWD  " : "BACK ");

        printf("X:%+4d[%s @ %5dus] | Y:%+4d[%s @ %5dus] | BTN:%d\n",
               xVal, xLabel, periodX_us,
               yVal, yLabel, periodY_us,
               btnRaw);
        fflush(stdout);

        // ── Step both motors for ~50ms window ───────────
        int window_us  = 50000; // 50 ms control loop window
        int elapsed_us = 0;
        int nextStep1  = periodX_us; // time until next step for motor 1
        int nextStep2  = periodY_us; // time until next step for motor 2

        // If both motors are stopped, just delay and continue
        if (periodX_us == 0 && periodY_us == 0) {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        // Initialize next-step timers; if motor is stopped set to max
        if (periodX_us == 0) nextStep1 = window_us + 1;
        if (periodY_us == 0) nextStep2 = window_us + 1;

        while (elapsed_us < window_us) {
            // Find the nearest upcoming event
            int next_event = (nextStep1 < nextStep2) ? nextStep1 : nextStep2;

            // Cap at remaining window time
            if (next_event > (window_us - elapsed_us))
                next_event = window_us - elapsed_us;

            // Wait until next event
            if (next_event > 0)
                esp_rom_delay_us(next_event);

            elapsed_us += next_event;
            nextStep1  -= next_event;
            nextStep2  -= next_event;

            // Fire motor 1 step if due
            if (nextStep1 <= 0 && periodX_us > 0) {
                do_step(STEP_PIN_1);
                nextStep1 += periodX_us;
            }

            // Fire motor 2 step if due
            if (nextStep2 <= 0 && periodY_us > 0) {
                do_step(STEP_PIN_2);
                nextStep2 += periodY_us;
            }
        }
    }
}
