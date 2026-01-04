#ifndef DRV8825_H
#define DRV8825_H
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include <math.h>
#include <stddef.h>
// Forward declare that this type exists (defined in combined.h)
struct homing_ctx_t;
typedef struct homing_ctx_t* axis_handle_t;

typedef struct motor_t motor_t;
typedef struct motor_t* motor_handle_t;

#define STEP_HIGH_TIME_MS 1.9
#define STEP_TOTAL_TIME_MS 25

#define STEP_HIGH_TIME_uS 5
#define STEP_TOTAL_TIME_uS 30

#define MAX_STEP_INTERVAL_US 150
#define MIN_STEP_INTERVAL_US 30

#define X_MAX_MM 540
#define Y_MAX_MM 390

#define PULLEY_RADIUS_MM 6.36
#define STEPS_PER_REV 3200
#define PI 6.28318530717958647692/2
#define STEPS_PER_MM ( (double)STEPS_PER_REV / (PI * (double)PULLEY_RADIUS_MM) )

struct motor_t {
    /**
     * @brief Enable motor
     * @param motor_handle_t Motor object handle
     */
    void (*setup_stepper_pins)(motor_handle_t motor);

    /**
     * @brief Sets Microstepping motor
        +-----------+-----+-----+-----+
        | Microstep | MS0 | MS1 | MS2 |
        +-----------+-----+-----+-----+
        | Full      |  0  |  0  |  0  |
        | 1/2       |  1  |  0  |  0  |
        | 1/4       |  0  |  1  |  0  |
        | 1/8       |  1  |  1  |  0  |
        | 1/16      |  0  |  0  |  1  |
        | 1/32      |  1  |  0  |  1  |
        | 1/32      |  0  |  1  |  1  |
        | 1/32      |  1  |  1  |  1  |
        +-----------+-----+-----+-----+

     * @param motor Motor object
     * @return
     */
    void (*setmicrostep)(motor_handle_t motor, int m0, int m1, int m2);

    
    /**
     * @brief Moves Specified Number of Steps Horizontal
     * @param motor Motor object
     * @return
     *  - ESP_OK: Success
     * - ESP_ERR_INVALID_ARG: Invalid argument
     * - ESP_FAIL: Failed
     */
    void (*step_H)(motor_handle_t motor, int no_steps);
    /**
     * @brief Moves Specified Number of Steps vertically
     * @param motor Motor object
     * @return
     *  - ESP_OK: Success
     * - ESP_ERR_INVALID_ARG: Invalid argument
     * - ESP_FAIL: Failed
     */
    void (*step_V)(motor_handle_t motor, int no_steps);

    


    /**
     * @brief Delete motor object
     * @param motor Motor object
     * @return
     *     - ESP_OK: Success
     *    - ESP_ERR_INVALID_ARG: Invalid argument
     *   - ESP_FAIL: Failed
     */
    void (*del)(motor_handle_t motor);

    gpio_num_t step;
    gpio_num_t dir;
    gpio_num_t m0;
    gpio_num_t m1;
    gpio_num_t m2;
};
void line_move_mm_blocking(axis_handle_t x_axis, axis_handle_t y_axis, int target_x_mm, int target_y_mm);
static inline int mm_to_steps(double mm);
static inline int read_limit(gpio_num_t pin);
static inline void step_pulse(gpio_num_t step_pin);
#endif /* DRV8825 Library*/