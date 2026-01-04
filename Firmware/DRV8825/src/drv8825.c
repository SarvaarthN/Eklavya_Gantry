#include "drv8825.h"
#include "limit.h"
#include "combined.h"
static const char *TAG = "MOTOR";

static volatile int current_x_mm = 0;
static volatile int current_y_mm = 0;

static inline int mm_to_steps(double mm) {
    double s = mm * STEPS_PER_MM;
    return (int)llround(s);
}

static inline int read_limit(gpio_num_t pin) {
    return gpio_get_level(pin);
}
static inline void step_pulse(gpio_num_t step_pin) {
    gpio_set_level(step_pin, 1);
    esp_rom_delay_us(STEP_HIGH_TIME_uS);
    gpio_set_level(step_pin, 0);
}

void setup_stepper_pins(motor_handle_t mtr) {
    // Configure stepper driver output pins
    gpio_config_t io_conf_out = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << mtr->step) | (1ULL << mtr->dir) | 
                        (1ULL << mtr->m0) | (1ULL << mtr->m1) | (1ULL << mtr->m2),
        .pull_down_en = 0,
        .pull_up_en = 0
    };
    gpio_config(&io_conf_out);
}

void step_H(motor_handle_t mtr, int no_steps) {
    int accel_steps = 6400;
    int decel_steps = accel_steps;
    if (no_steps<accel_steps){
        accel_steps=no_steps/2;
        decel_steps=accel_steps;
    }
    int const_steps = no_steps - accel_steps - decel_steps;
    if (const_steps<0){
        const_steps=0;
    }
    // Define min and max total step time (in microseconds)

    // Acceleration Phase
    for (int i = 0; i < accel_steps; i++) {
        int step_time = MAX_STEP_INTERVAL_US - ((MAX_STEP_INTERVAL_US - MIN_STEP_INTERVAL_US) * i / accel_steps);
        gpio_set_level( mtr->step, 1);
        esp_rom_delay_us(STEP_HIGH_TIME_uS);
        gpio_set_level( mtr->step, 0);
        esp_rom_delay_us(step_time - STEP_HIGH_TIME_uS);
    }

    // Constant Speed Phase
    for (int i = 0; i < const_steps; i++) {
        gpio_set_level( mtr->step, 1);
        esp_rom_delay_us(STEP_HIGH_TIME_uS);
        gpio_set_level( mtr->step, 0);
        esp_rom_delay_us(MIN_STEP_INTERVAL_US - STEP_HIGH_TIME_uS);
    }

    // Deceleration Phase
    for (int i = 0; i < decel_steps; i++) {
        int step_time = MIN_STEP_INTERVAL_US + ((MAX_STEP_INTERVAL_US - MIN_STEP_INTERVAL_US) * i / decel_steps);
        gpio_set_level( mtr->step, 1);
        esp_rom_delay_us(STEP_HIGH_TIME_uS);
        gpio_set_level( mtr->step, 0);
        esp_rom_delay_us(step_time - STEP_HIGH_TIME_uS);
    }
}
void step_V(motor_handle_t mtr, int no_steps) {
    int accel_steps = 6400;
    int decel_steps = accel_steps;
    if (no_steps<accel_steps){
        accel_steps=no_steps/2;
        decel_steps=accel_steps;
    }
    int const_steps = no_steps - accel_steps - decel_steps;
    if (const_steps<0){
        const_steps=0;
        accel_steps=no_steps/2;
        decel_steps=accel_steps;
    }

    int total_steps=0;

    // Acceleration Phase
    for (int i = 0; i < accel_steps; i++) {
        int step_time = MAX_STEP_INTERVAL_US - ((MAX_STEP_INTERVAL_US - MIN_STEP_INTERVAL_US) * i / accel_steps);
        gpio_set_level(mtr->step, 1);
        esp_rom_delay_us(STEP_HIGH_TIME_uS);
        gpio_set_level(mtr->step, 0);
        esp_rom_delay_us(step_time - STEP_HIGH_TIME_uS);
        total_steps++;
    }

    // Constant Speed Phase
    for (int i = 0; i < const_steps; i++) {
        gpio_set_level(mtr->step, 1);
        esp_rom_delay_us(STEP_HIGH_TIME_uS);
        gpio_set_level(mtr->step, 0);
        esp_rom_delay_us(MIN_STEP_INTERVAL_US - STEP_HIGH_TIME_uS);
        total_steps++;
    }

    // Deceleration Phase
    for (int i = 0; i < decel_steps; i++) {
        int step_time = MIN_STEP_INTERVAL_US + ((MAX_STEP_INTERVAL_US - MIN_STEP_INTERVAL_US) * i / decel_steps);
        gpio_set_level(mtr->step, 1);
        esp_rom_delay_us(STEP_HIGH_TIME_uS);
        gpio_set_level(mtr->step, 0);
        esp_rom_delay_us(step_time - STEP_HIGH_TIME_uS);
        total_steps++;
    }

    printf("Total steps taken = %d\n" ,total_steps);
}



void line_move_mm_blocking(axis_handle_t x_axis, axis_handle_t y_axis, int target_x_mm, int target_y_mm) {
    if (target_x_mm < 0) target_x_mm = 0;
    if (target_y_mm < 0) target_y_mm = 0;
    if (target_x_mm > X_MAX_MM) target_x_mm = X_MAX_MM;
    if (target_y_mm > Y_MAX_MM) target_y_mm = Y_MAX_MM;

    int dx_mm = target_x_mm - current_x_mm;
    int dy_mm = target_y_mm - current_y_mm;

    int sx = (dx_mm >= 0) ? 1 : -1;
    int sy = (dy_mm >= 0) ? 1 : -1;

    int dx_steps = abs(mm_to_steps((double)dx_mm));
    int dy_steps = abs(mm_to_steps((double)dy_mm));

    if (dx_steps == 0 && dy_steps == 0) return;

    gpio_set_level(x_axis->motor->dir, (sx > 0) ? 1 : 0);
    gpio_set_level(y_axis->motor->dir, (sy > 0) ? 1 : 0);

    int primary = (dx_steps >= dy_steps) ? dx_steps : dy_steps;
    int accel_steps = (int)llround(0.2 * (double)primary);
    if (accel_steps < 1) accel_steps = (primary >= 3) ? 1 : 0;
    int decel_steps = accel_steps;
    int const_steps = primary - accel_steps - decel_steps;
    if (const_steps < 0) { const_steps = 0; accel_steps = primary / 2; decel_steps = primary - accel_steps; }

    int err = (dx_steps >= dy_steps) ? (dx_steps / 2) : (dy_steps / 2);
    int x_cnt = 0, y_cnt = 0;

    for (int i = 0; i < primary; i++) {
        if (dx_steps >= dy_steps) {
            err -= dy_steps;
            if (err < 0) { err += dx_steps; step_pulse(y_axis->motor->step); y_cnt++; }
            step_pulse(x_axis->motor->step); x_cnt++;
        } else {
            err -= dx_steps;
            if (err < 0) { err += dy_steps; step_pulse(x_axis->motor->step); x_cnt++; }
            step_pulse(y_axis->motor->step); y_cnt++;
        }

        if ((read_limit(x_axis->limit->gpio) == x_axis->limit->active && sx < 0) ||
            (read_limit(y_axis->limit->gpio) == y_axis->limit->active && sy < 0)) {
            printf("\n[HALT] Hit limit during move.\n");
            break;
        }

        int interval_us = MAX_STEP_INTERVAL_US;
        if (i < accel_steps && accel_steps > 0) {
            interval_us = MAX_STEP_INTERVAL_US - (int)((double)(MAX_STEP_INTERVAL_US - MIN_STEP_INTERVAL_US) * (double)i / (double)accel_steps);
        } else if (i >= accel_steps + const_steps && decel_steps > 0) {
            int di = i - (accel_steps + const_steps);
            interval_us = MIN_STEP_INTERVAL_US + (int)((double)(MAX_STEP_INTERVAL_US - MIN_STEP_INTERVAL_US) * (double)di / (double)decel_steps);
        } else {
            interval_us = MIN_STEP_INTERVAL_US;
        }
        if (interval_us < STEP_HIGH_TIME_uS + 1) interval_us = STEP_HIGH_TIME_uS + 1;
        esp_rom_delay_us(interval_us - STEP_HIGH_TIME_uS);
    }

    current_x_mm += sx * (int)llround((double)x_cnt / STEPS_PER_MM);
    current_y_mm += sy * (int)llround((double)y_cnt / STEPS_PER_MM);
    if (current_x_mm < 0) current_x_mm = 0;
    if (current_y_mm < 0) current_y_mm = 0;
    if (current_x_mm > X_MAX_MM) current_x_mm = X_MAX_MM;
    if (current_y_mm > Y_MAX_MM) current_y_mm = Y_MAX_MM;
}

void setmicrostep(motor_handle_t mtr, int m0,int m1, int m2){
    gpio_set_level(mtr->m0, m0);
    gpio_set_level(mtr->m1, m1);
    gpio_set_level(mtr->m2, m2);
}

