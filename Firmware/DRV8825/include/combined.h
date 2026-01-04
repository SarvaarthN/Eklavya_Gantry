
#ifndef COMBINED_H
#define COMBINED_H

#include "limit.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// Forward declare motor types (defined in drv8825.h)
struct motor_t; 
typedef struct motor_t* motor_handle_t; // Motor handle is a pointer to a motor object

typedef struct homing_ctx_t homing_ctx_t; // Forward declaration of homing context structure
typedef struct homing_ctx_t* axis_handle_t; // Axis handle is a pointer to a homing context
/* Homing context for a single axis */
struct homing_ctx_t {
    motor_handle_t motor;
    limit_switch_t *limit;
    int home_dir_level;

    /**
     * @brief Delete motor object
     * @param motor Motor object
     * @return
     *     - ESP_OK: Success
     *    - ESP_ERR_INVALID_ARG: Invalid argument
     *   - ESP_FAIL: Failed
     */
    void (*home)(axis_handle_t axis);
};

void home_task(void *pvParameters); // FreeRTOS task that performs homing (axis passed via pvParameters)
void home(axis_handle_t axis); // Public API to start homing for an axis

#endif