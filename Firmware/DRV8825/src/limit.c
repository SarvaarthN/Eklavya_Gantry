#include "limit.h"

esp_err_t limit_switch_init(limit_switch_t *sw)
{
    if (!sw) return ESP_ERR_INVALID_ARG;

    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << sw->gpio,
        .mode = GPIO_MODE_INPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pull_down_en = 0,
        .pull_up_en = 1 // Use pull-up if switch connects to GND when pressed
    };

    /* Configure pull resistors */
    if (sw->active == LIMIT_SWITCH_ACTIVE_LOW) {
        io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    } else {
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    }

    return gpio_config(&io_conf);
}





bool limit_switch_is_pressed(limit_switch_t *sw)
{
    int level = gpio_get_level(sw->gpio);

    if (sw->active == LIMIT_SWITCH_ACTIVE_LOW)
        return (level == 0);
    else
        return (level == 1);
}
