#include "combined.h"
#include "drv8825.h"
#include "limit.h"
void home_task(void *pvParameters) {
    axis_handle_t axis = (axis_handle_t) pvParameters;

    gpio_set_level(axis->motor->dir, axis->home_dir_level);

    while (!axis->limit->limit_switch_is_pressed(axis->limit)) {
        axis->motor->step_H(axis->motor,1);
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    vTaskDelete(NULL);
}



void home(axis_handle_t axis){
    xTaskCreate(home_task, "Home Axis", 2048, axis, 1, NULL);
}