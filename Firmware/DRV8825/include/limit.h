#ifndef LIMIT_H
#define LIMIT_H

#include "esp_err.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

typedef struct limit_switch limit_switch_t;
/* Logical state of switch */
typedef enum {
    LIMIT_SWITCH_ACTIVE_LOW,
    LIMIT_SWITCH_ACTIVE_HIGH
} limit_switch_active_t;

/* Limit switch object */
typedef struct limit_switch  {
    gpio_num_t gpio;                  // GPIO connected
    limit_switch_active_t active;     // Active high / low
    void (*limit_switch_init) (limit_switch_t *sw);
    bool (*limit_switch_is_pressed) (limit_switch_t *sw);
} limit_switch_t;

/* API */




#endif /* Limit Switch Library*/