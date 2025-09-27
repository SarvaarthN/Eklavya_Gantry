#include "driver/uart.h"
#include "driver/gpio.h"
#include <stdio.h>

#define UART_PORT_NUM      UART_NUM_1
#define UART_TX_PIN        26
#define UART_RX_PIN        25
#define BUF_SIZE           1024
#define BAUD_RATE          115200

void app_main(void)
{
    uart_config_t uart_config = {
        .baud_rate = BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };

    // Configure UART
    uart_driver_install(UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_PORT_NUM, &uart_config);
    uart_set_pin(UART_PORT_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    printf("UART Initialized\n");

    // Example: send a TMC2208 command (read DRV_STATUS register)
    uint8_t cmd[] = {0x05, 0x10, 0xA3, 0x00};  // Example: actual command may vary
    uart_write_bytes(UART_PORT_NUM, (const char*)cmd, sizeof(cmd));

    uint8_t data[BUF_SIZE];
    while (1) {
        int len = uart_read_bytes(UART_PORT_NUM, data, BUF_SIZE, pdMS_TO_TICKS(1000));
        if (len > 0) {
            printf("Received %d bytes: ", len);
            for (int i = 0; i < len; i++) {
                printf("%02X ", data[i]);
            }
            printf("\n");
        }
    }
}
