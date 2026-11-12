#include <stdint.h>
#include <stdio.h>
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"

#include "sx1262_driver.h"

#define PIN_SCK 7  // GPIO 7, xiao d8
#define PIN_MISO 8 // GPIO 8, xiao d9
#define PIN_MOSI 9 // GPIO 9, xiao d10
#define PIN_NSS 41 // radio chip select

static void print_bin8(uint8_t x)
{
    for (int i = 7; i >= 0; i--)
    {
        printf("%d", (x >> i) & 1);
        if (i == 4)
        {
            printf(" ");
        }
    }
}

static void get_status_byte(uint8_t x)
{
}

static void gpio_init(void)
{
    // gpio_config_t reset_pin_cfg = {
    //     .pin_bit_mask = (1U << PIN_RESET),
    //     .mode         = GPIO_MODE_OUTPUT,
    //     .pull_up_en

    // };
    gpio_set_direction(PIN_RESET, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_BUSY, GPIO_MODE_INPUT);
}


void app_main(void)
{
    spi_bus_config_t bus = {
        .sclk_io_num = PIN_SCK,
        .mosi_io_num = PIN_MOSI,
        .miso_io_num = PIN_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO)); // what is DMA?

    // // gpio_set_pull_mode(PIN_MISO, GPIO_PULLDOWN_ONLY);

    spi_device_interface_config_t dev = {
        .clock_speed_hz = 1 * 1000 * 1000, // 1 mhz
        .mode = 0,                         // CPOL, CPHA
        .spics_io_num = PIN_NSS,           // NSS vs CS? what are these?
        .queue_size = 1};
    spi_device_handle_t radio;
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &dev, &radio));

    gpio_init();
    gpio_set_level(PIN_RESET, 1); // reset off. high makes it go off, since NRESET is active-low

    vTaskDelay(pdMS_TO_TICKS(10));
    printf("BUSY pin level on sx1262: %d\n", gpio_get_level(PIN_BUSY));

    gpio_set_level(PIN_RESET, 0); // reset line on
    // vTaskDelay(pdMS_TO_TICKS(2));
    esp_rom_delay_us(200);
    gpio_set_level(PIN_RESET, 1); // reset line off
    // int64_t t0 = esp_timer_get_time();

    // while (gpio_get_level(PIN_BUSY) != 0) { // wait..
    // }
    // int64_t elapsed = esp_timer_get_time() - t0;
    ESP_ERROR_CHECK(radio_wait_ready(10000));


    sx1262_cmd_get_device_errors(radio);
    sx1262_cmd_clear_device_errors(radio);

    uint8_t tcxo_voltage = 0x02; // 1.8v, table 13-35
    sx1262_cmd_set_dio3_as_tcxo_ctrl(radio, tcxo_voltage);

    sx1262_cmd_calibrate(radio);

    sx1262_cmd_get_device_errors(radio);

    // reading the sync word

    uint8_t copy_rx_len = 2;
    uint8_t read_register_rx[copy_rx_len];

    uint16_t sync_word_addr_start = 0x0740;
    sx1262_cmd_read_register(radio, sync_word_addr_start, read_register_rx, sizeof(read_register_rx));

    sx1262_cmd_set_standby(radio);
    
    sx1262_cmd_get_device_errors(radio);

    sx1262_cmd_set_packet_type(radio);


    uint64_t frequency_mhz = 915; // * 1000 * 1000;
    sx1262_cmd_set_rf_frequency(radio, frequency_mhz);
    
    sx1262_cmd_get_device_errors(radio);

    sx1262_cmd_calibrate_image(radio);
    
    uint8_t paDutyCycle = 0x02;
    uint8_t hpMax = 0x02;
    sx1262_cmd_pa_config(radio, paDutyCycle, hpMax);

    uint8_t power = 0x00;
    uint8_t rampTime = 0x04;
    sx1262_cmd_set_tx_params(radio, power, rampTime);

    

    sx1262_cmd_get_device_errors(radio);



    // SF: 7 (short airtime, quick feedback)
    // BW: 125 khz
    // CR: 4/5 (good starting point)
    // LDRO: off for now
    uint8_t spread_factor = 0x07;
    uint8_t bandwidth = 0x04;
    uint8_t coding_rate = 0x01;
    uint8_t low_data_rate_optimize_enabled = 0x00;

    sx1262_cmd_set_modulation_params(radio, spread_factor, bandwidth, coding_rate, low_data_rate_optimize_enabled);


    uint16_t preamble_length = 12;
    uint8_t header_type = 0x00; // explicit, because we have variable length packets that we send on lora
    uint8_t payload_length = 255;
    uint8_t crc_type = 0x1; // on
    uint8_t invert_iq = 0x00; // standard

    sx1262_cmd_set_packet_params(radio, preamble_length, header_type, payload_length, crc_type, invert_iq);

    
    sx1262_cmd_set_dio2_as_rf_switch_ctrl(radio);
    
    sx1262_cmd_get_status(radio);

    sx1262_cmd_get_device_errors(radio);

}
