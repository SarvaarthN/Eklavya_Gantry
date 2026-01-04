#include "limit.h"

limit_switch_t x_min = {
    .gpio = GPIO_NUM_18,
    .active = LIMIT_SWITCH_ACTIVE_LOW
};

limit_switch_t y_min = {
    .gpio = GPIO_NUM_19,
    .active = LIMIT_SWITCH_ACTIVE_LOW
};

void app_main(void)
{
    limit_switch_init(&x_min);
    limit_switch_init(&y_min);

    while (1) {
        if (limit_switch_is_pressed(&x_min)) {
            printf("X MIN pressed\n");
        }
        if (limit_switch_is_pressed(&y_min)) {
            printf("Y MIN pressed\n");
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
