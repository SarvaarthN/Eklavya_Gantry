#include <stdio.h>
#include <stdint.h>
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


static const i2c_port_num_t i2c_port = 0;
static const gpio_num_t i2c_sda=21;
static const gpio_num_t i2c_scl=5;
static const uint8_t i2c_glitch_ignore_cnt=7;
static const uint16_t deviceaddr=0b0100111;
static const uint32_t clkSpeed=100e3;
static const uint32_t sleep_time_ms=1000;
static const uint8_t buf=0x0A; //OLATA
static const uint8_t bufferHigh= 0x80; // GPIO A7 High
static const uint8_t bufferLow= 0x00; // GPIO A7 High
static const uint8_t dataH[2]= {buf,bufferHigh};
static const uint8_t dataL[2]= {buf,bufferLow};

void app_main(void)
{
    i2c_master_bus_handle_t bus_handle;
    i2c_master_dev_handle_t dev_handle;

    i2c_master_bus_config_t i2c_mst_config = {
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .i2c_port = i2c_port,
    .scl_io_num = i2c_scl,
    .sda_io_num = i2c_sda,
    .glitch_ignore_cnt = i2c_glitch_ignore_cnt,
    .flags.enable_internal_pullup = true,
};


ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));

i2c_device_config_t dev_cfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = deviceaddr,
    .scl_speed_hz = clkSpeed,
};


ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));
    uint8_t pin_config[2] = {0x00, 0x00}; // IODIRA, all outputs
    ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, pin_config, 2, -1));

    while (true){
        vTaskDelay(sleep_time_ms/portTICK_PERIOD_MS);
        ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, dataH, 2, -1));
        vTaskDelay(sleep_time_ms/portTICK_PERIOD_MS);
        ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, dataL, 2, -1));

    }

}
