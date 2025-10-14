#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_rom_sys.h"

#define STEP_PIN    GPIO_NUM_26
#define DIR_PIN     GPIO_NUM_25

#define UART_PORT   UART_NUM_1
#define UART_TX_PIN GPIO_NUM_13
#define UART_RX_PIN GPIO_NUM_27

#define STEP_DELAY_US 500
static const char *TAG = "TMC2208";

// ---------------- CRC Calculation ----------------
uint8_t tmc_crc(uint8_t *datagram, int len) {
    uint8_t crc = 0;
    for (int i = 0; i < len; i++) {
        crc ^= datagram[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x01) crc = (crc >> 1) ^ 0x8C;
            else crc >>= 1;
        }
    }
    return crc;
}

// ---------------- Write Register ----------------
void tmc2208_write(uint8_t reg, uint32_t data) {
    uint8_t datagram[8];
    datagram[0] = 0x05;          
    datagram[1] = 0x00;          
    datagram[2] = reg | 0x80;    
    datagram[3] = (data >> 24) & 0xFF;
    datagram[4] = (data >> 16) & 0xFF;
    datagram[5] = (data >> 8) & 0xFF;
    datagram[6] = data & 0xFF;
    datagram[7] = tmc_crc(datagram, 7);
    uart_write_bytes(UART_PORT, (const char *)datagram, 8);
    ESP_LOGI(TAG, "Write reg 0x%02X = 0x%08" PRIX32, reg, data);
}

// ---------------- Read Register ----------------
int tmc2208_read(uint8_t reg, uint8_t *resp, int max_len) {
    uint8_t datagram[4];
    datagram[0] = 0x05;
    datagram[1] = 0x00;
    datagram[2] = reg;
    datagram[3] = tmc_crc(datagram, 3);

    uart_flush(UART_PORT);
    uart_write_bytes(UART_PORT, (const char *)datagram, 4);

    int len = uart_read_bytes(UART_PORT, resp, max_len, pdMS_TO_TICKS(100));
    if (len > 0) {
        ESP_LOGI(TAG, "Read reg 0x%02X response (%d bytes):", reg, len);
        for (int i = 0; i < len; i++) {
            printf("%02X ", resp[i]);
        }
        printf("\n");
    } else {
        ESP_LOGW(TAG, "No response from reg 0x%02X", reg);
    }
    return len;
}

// ---------------- Stepper Setup ----------------
bool stepper_setup(uint8_t microstep) {
    // UART init
    const uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(UART_PORT, &uart_config);
    uart_set_pin(UART_PORT, UART_TX_PIN, UART_RX_PIN,
                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_PORT, 256, 256, 0, NULL, 0);

    // STEP/DIR GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << STEP_PIN) | (1ULL << DIR_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 0,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    gpio_set_level(DIR_PIN, 1);

    // ---- Set CHOPCONF microstepping ----
    uint32_t chopconf = 0x000100C3;  // base config
    chopconf &= ~(0x0F << 24);       // clear MRES
    chopconf |= ((microstep & 0x0F) << 24); // set MRES
    tmc2208_write(0x6C, chopconf);

    vTaskDelay(pdMS_TO_TICKS(50));

    // Verify UART
    uint8_t resp[16];
    int len = tmc2208_read(0x6C, resp, sizeof(resp));
    if (len <= 0) {
        ESP_LOGE(TAG, "UART read failed. Check wiring and PDN_UART jumper!");
        return false;
    }

    ESP_LOGI(TAG, "Stepper setup complete.");
    return true;
}

// ---------------- Stepper Move ----------------
void stepper_move(int steps) {
    for (int i = 0; i < steps; i++) {
        gpio_set_level(STEP_PIN, 1);
        esp_rom_delay_us(STEP_DELAY_US);
        gpio_set_level(STEP_PIN, 0);
        esp_rom_delay_us(STEP_DELAY_US);
    }
}

// ---------------- Main ----------------
void app_main(void) {
    uint8_t microstep = 7; // 1/32 microstepping
    if (!stepper_setup(microstep)) {
        ESP_LOGE(TAG, "Stepper setup failed. Exiting...");
        return;
    }

    ESP_LOGI(TAG, "Moving 6400 steps...");
    stepper_move(6400);
    ESP_LOGI(TAG, "Done.");
    uint8_t resp[16];
    tmc2208_read(0x6C, resp, sizeof(resp));
}

