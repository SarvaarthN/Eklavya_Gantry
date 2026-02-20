#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "rom/ets_sys.h"
#include <stdio.h>

// ─── I2C Config (YOUR WIRING) ─────────────────────────────────────────────────
#define I2C_MASTER_NUM      I2C_NUM_0
#define I2C_MASTER_SDA_IO   GPIO_NUM_19  // SDA → GPIO19 ✓
#define I2C_MASTER_SCL_IO   GPIO_NUM_18  // SCL → GPIO18 ✓
#define I2C_MASTER_FREQ_HZ  400000
#define MCP23017_ADDR       0x20

// ─── MCP23017 Registers ───────────────────────────────────────────────────────
#define MCP_IODIRA          0x00         // Port A direction  
#define MCP_OLATA           0x14         // Port A output latch

// ─── Stepper Pins ─────────────────────────────────────────────────────────────
#define STEP_PIN            GPIO_NUM_4   // Direct STEP ✓
#define DIR_BIT             (1 << 6)     // A6 = GPA6 on 7Semi

#define STEP_HIGH_US        3000         // 1.9ms high
#define STEP_PERIOD_US      25000        // 25ms total (40 RPM)

static uint8_t porta_state = 0x00;

esp_err_t mcp_write(uint8_t reg, uint8_t value) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MCP23017_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, value, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return ret;
}

void i2c_mcp_init() {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,  // GPIO19
        .scl_io_num = I2C_MASTER_SCL_IO,  // GPIO18
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
    
    vTaskDelay(pdMS_TO_TICKS(100));
    mcp_write(MCP_IODIRA, 0x00);  // Port A all outputs
    mcp_write(MCP_OLATA, 0x00);   // Clear outputs
}

void setup_stepper_pins() {
    // STEP on GPIO4 (direct)
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << STEP_PIN),
        .pull_down_en = 0,
        .pull_up_en = 0
    };
    gpio_config(&io_conf);
    gpio_set_level(STEP_PIN, 0);
    
    // DIR on MCP A6 (GPA6)
    porta_state |= DIR_BIT;  // DIR high
    mcp_write(MCP_OLATA, porta_state);
}

void stepper_step() {
    gpio_set_level(STEP_PIN, 1);
    ets_delay_us(STEP_HIGH_US);
    gpio_set_level(STEP_PIN, 0);
    ets_delay_us(STEP_PERIOD_US - STEP_HIGH_US);
}

void app_main() {
    i2c_mcp_init();
    setup_stepper_pins();
    
    printf("Stepper Control:\n");
    printf("  STEP → GPIO4\n");
    printf("  DIR  → MCP A6 (GPA6)\n");
    printf("  I2C: SDA=GPIO19, SCL=GPIO18\n");
    
    int steps = 0;
    while (1) {
        stepper_step();
        steps++;
        if (steps % 50 == 0) {
            printf("Steps: %d (40 RPM)\n", steps);
        }
    }
}
