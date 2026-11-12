#include <stdint.h>
#include <stdio.h>
#include "driver/spi_common.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "hal/gpio_types.h"
#include "hal/spi_types.h"

#define PIN_SCK 7 // GPIO 7, xiao d8
#define PIN_MISO 8 // GPIO 8, xiao d9
#define PIN_MOSI 9 // GPIO 9, xiao d10
#define PIN_NSS  41 // radio chip select, not connected at the moment. 

// q: why are all the numbners decremented by 1?

static void print_bin8(uint8_t x)
{
    for (int i = 7; i >= 0; i--) {
        printf("%d", (x >> i) & 1);
        if (i == 4) {
            printf(" ");
        }
    }
}


void app_main(void)
{
    spi_bus_config_t bus = {
        .sclk_io_num = PIN_SCK,
        .mosi_io_num = PIN_MOSI,
        .miso_io_num = PIN_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = 1,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO)); // what is DMA?

    gpio_set_pull_mode(PIN_MISO, GPIO_PULLDOWN_ONLY);

    spi_device_interface_config_t dev = {
        .clock_speed_hz = 40 * 1000 * 1000, // 1 mhz
        .mode = 0, // CPOL, CPHA
        .spics_io_num = PIN_NSS, // NSS vs CS? what are these?
        .queue_size = 1
    };
    spi_device_handle_t radio;
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &dev, &radio));

    uint8_t tx[3] = {0x3A, 0x07, 0x40};
    uint8_t rx[3] = {0xAA, 0xAA, 0xAA};

    while (1) {
        spi_transaction_t t = {
            .length = sizeof(tx) * 8, // in bits,
            .tx_buffer = tx,
            .rx_buffer = rx,
        };
        ESP_ERROR_CHECK(spi_device_transmit(radio, &t));

        for (int i = 0; i < 3; i++) {
            printf("tx ");
            print_bin8(tx[i]);
            printf("   rx ");
            print_bin8(rx[i]);
            printf("\n");
        }
        printf("\n");

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}