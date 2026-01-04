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
    gpio_num_t gpio;                 // GPIO pin connected to the limit switch
    limit_switch_active_t active;     // Defines whether switch is active HIGH or LOW
    void (*limit_switch_init) (limit_switch_t *sw); // Initializes the limit switch GPIO configuration
    bool (*limit_switch_is_pressed) (limit_switch_t *sw); // Returns true if the limit switch is currently pressed
} limit_switch_t;


#endif /* Limit Switch Library*/