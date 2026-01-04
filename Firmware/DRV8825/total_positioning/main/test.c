
#include <stdio.h>
#include <stdlib.h>
#include "driver/uart.h"
#include "esp_rom_sys.h"

#include "drv8825.h"
#include "limit.h"
#include "combined.h"

limit_switch_t x_l = {
    .gpio = GPIO_NUM_18,
    .active = LIMIT_SWITCH_ACTIVE_LOW
};

limit_switch_t y_l = {
    .gpio = GPIO_NUM_19,
    .active = LIMIT_SWITCH_ACTIVE_LOW
};

motor_t m_x = {
    .step = GPIO_NUM_3,
    .dir  = GPIO_NUM_4
};

motor_t m_y = {
    .step = GPIO_NUM_6,
    .dir  = GPIO_NUM_8
};

homing_ctx_t x_axis = {
    .motor = &m_x,
    .limit = &x_l,
    .home_dir_level=0
};
homing_ctx_t y_axis = {
    .motor = &m_y,
    .limit = &y_l,
    .home_dir_level=0
};

void app_main(){
    x_l.limit_switch_init(&x_l);
    y_l.limit_switch_init(&y_l);
    m_x.setup_stepper_pins(&m_x);
    m_y.setup_stepper_pins(&m_y);
    m_x.step_H(&m_x, 40);

    x_axis.home(&x_axis);
    y_axis.home(&y_axis);
    line_move_mm_blocking(&x_axis, &y_axis, 25, 30); 



}