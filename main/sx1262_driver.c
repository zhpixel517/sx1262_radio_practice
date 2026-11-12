#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "driver/spi_common.h"
#include "driver/spi_master.h"
#include "hal/gpio_types.h"
#include "driver/gpio.h"
#include "hal/spi_types.h"
#include "esp_err.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"


#include "sx1262_driver.h"

esp_err_t radio_wait_ready(int32_t timeout)
{
    int64_t t0 = esp_timer_get_time();

    while (gpio_get_level(PIN_BUSY) != 0)
    {
        int64_t elapsed = esp_timer_get_time() - t0;
        if (elapsed > timeout)
        {
            return ESP_ERR_TIMEOUT;
        }
    }

    printf("BUSY low after %lld us\n", esp_timer_get_time() - t0);
    return ESP_OK;
}

esp_err_t send_command(spi_device_handle_t device_handle,
                       const char *command_name,
                       const uint8_t *tx,
                       size_t tx_len,
                       uint8_t *rx,
                       int32_t timeout)
{
    printf("Sending SPI command: %s\n", command_name);

    spi_transaction_t transaction = {
        .length = tx_len * 8,
        .tx_buffer = tx,
        .rx_buffer = rx};

    ESP_ERROR_CHECK(radio_wait_ready(timeout)); // always wait for ready before sending
    ESP_ERROR_CHECK(spi_device_transmit(device_handle, &transaction));

    for (int i = 0; i < tx_len; i++)
    {
        printf("tx ");
        printf("%02X", tx[i]);
        printf("   rx ");
        printf("%02X", rx[i]);
        printf("\n");
    }

    printf("\n");
    ESP_ERROR_CHECK(radio_wait_ready(timeout)); // always wait for ready before sending

    return ESP_OK;
}

// sx1262 driver commands

void sx1262_cmd_get_status(spi_device_handle_t radio)
{
    uint8_t tx[2] = {GET_STATUS, NO_OP};
    uint8_t rx[2] = {0x00, 0x00};

    ESP_ERROR_CHECK(send_command(radio, "GetStatus", tx, sizeof(tx), rx, 10000));
}

void sx1262_cmd_get_device_errors(spi_device_handle_t radio)
{
    uint8_t tx[4] = {GET_DEVICE_ERRORS, NO_OP, NO_OP, NO_OP};
    uint8_t rx[4] = {0x00, 0x00, 0x00, 0x00};

    ESP_ERROR_CHECK(send_command(radio, "GET DEVICE ERRORS", tx, sizeof(tx), rx, 10000));
}

void sx1262_cmd_clear_device_errors(spi_device_handle_t radio)
{
    uint8_t tx[3] = {CLEAR_DEVICE_ERRORS, NO_OP, NO_OP};
    uint8_t rx[3] = {0x00, 0x00, 0x00};

    ESP_ERROR_CHECK(send_command(radio, "clear device errors", tx, sizeof(tx), rx, 10000));
}

void sx1262_cmd_set_dio3_as_tcxo_ctrl(spi_device_handle_t radio, uint8_t tcxo_voltage)
{
    uint32_t delay = TCXO_TIMETOUT_US * 64 / 1000; // 5000 us -> 320 ticks. we have 64khz oscillator. 1/64 of a millisecond is 1 ticke
    // i need to split this number across 3 different bytes to fit inside the spi transaction
    uint8_t delay_high = (delay >> 16) & 0xFF;
    uint8_t delay_mid = (delay >> 8) & 0xFF;
    uint8_t delay_low = delay & 0xFF;

    uint8_t tx[5] = {SET_DIO3_AS_TCXO_CTRL, tcxo_voltage, delay_high, delay_mid, delay_low};
    uint8_t rx[5] = {0x00, 0x00, 0x00, 0x00, 0x00};

    ESP_ERROR_CHECK(send_command(radio, "SetDIO3AsTCXOCtrl", tx, sizeof(tx), rx, 10000));
}

void sx1262_cmd_calibrate(spi_device_handle_t radio)
{
    uint8_t tx[2] = {CALIBRATE, 0b01111111}; // calibrate everything. leave the last as 0
    uint8_t rx[2] = {0x00, 0x00};

    ESP_ERROR_CHECK(send_command(radio, "CALIBRATE", tx, sizeof(tx), rx, 10000));
}

esp_err_t sx1262_cmd_read_register(spi_device_handle_t radio, uint16_t register_addr, uint8_t *rx, size_t rx_len)
{
    printf("Read register 0x%04X\n", register_addr);
    uint8_t addr_high = (register_addr >> 8) & 0xFF;
    uint8_t addr_low = register_addr & 0xFF;

    // start reading from 4 because this function returns the data after a bunch of status bytes.
    // 13.2.2

    uint8_t tx[rx_len + 4];
    tx[0] = READ_REGISTER; // only send the starting address once
    tx[1] = addr_high;
    tx[2] = addr_low;
    for (size_t i = 3; i < rx_len + 1; i++) { tx[i] = NO_OP; } // fill the rest of the buffer with NO_OP up to rx_len + 1 (plus 1 instead of plus 4 because 3 were already filled)
    uint8_t rx_temp[rx_len + 4];

    esp_err_t if_err = send_command(radio, "ReadRegister", tx, sizeof(tx), rx_temp, 10000);

    // dest, source, len
    memcpy(rx, &rx_temp[4], rx_len); // notice the index 4 here!

    return if_err;

}

void sx1262_cmd_set_standby(spi_device_handle_t radio)
{
    // set standby
    uint8_t tx[2] = {SET_STANDBY, 0}; // 0 here means STDBY_RC mode, using the RC13M clock, 1 uses TCXO
    uint8_t rx[2] = {0x00, 0x00};

    ESP_ERROR_CHECK(send_command(radio, "SetStandBy", tx, sizeof(tx), rx, 10000));
}

void sx1262_cmd_set_packet_type(spi_device_handle_t radio)
{
    // set packet type
    uint8_t tx[2] = {SET_PACKET_TYPE, 0x01}; // 0x01 means LoRa, see table 13-38
    uint8_t rx[2] = {0x00, 0x00};

    ESP_ERROR_CHECK(send_command(radio, "set packet type", tx, sizeof(tx), rx, 10000));
}

void sx1262_cmd_set_rf_frequency(spi_device_handle_t radio, uint64_t frequency_mhz)
{
    // set rf frequency
    // frequency = RFfreq * F_XTAL / 2^25
    // RFfreq = frequency * 2^25 / F_XTAL
    // F_XTAL is the frequency of the chip's crystal reference.
    uint32_t fxosc_mhz = 32;                                     // * 1000 * 1000
    uint64_t rf_freq = frequency_mhz * (1ULL << 25) / fxosc_mhz; // 1 << 25 is the same as 2^25

    // ^^ this only works for whole MHZ. for 915.125 mhz we have to switch to hertz later

    // new thing I learned: C works out the right hand side using the types on the right hand side, and only converts to
    // uint64_t when it stores the result.
    // printf("%02llX\n", rf_freq);

    uint8_t rf_freq_first_byte = (rf_freq >> 24) & 0xFF;
    uint8_t rf_freq_second_byte = (rf_freq >> 16) & 0xFF;
    uint8_t rf_freq_third_byte = (rf_freq >> 8) & 0xFF;
    uint8_t rf_freq_last_byte = (rf_freq) & 0xFF;

    uint8_t tx[5] = {SET_RF_FREQUENCY, rf_freq_first_byte, rf_freq_second_byte, rf_freq_third_byte, rf_freq_last_byte};
    uint8_t rx[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
    ESP_ERROR_CHECK(send_command(radio, "set rf frequency", tx, sizeof(tx), rx, 10000));
}

void sx1262_cmd_calibrate_image(spi_device_handle_t radio)
{
    // calibrate image
    // see 9.2.1

    uint8_t tx[3] = {CALIBRATE_IMAGE, 0xE1, 0xE9}; // image calibration defaults for the 902-928 band
    uint8_t rx[3] = {0x00, 0x00, 0x00};

    ESP_ERROR_CHECK(send_command(radio, "Calibrate Image", tx, sizeof(tx), rx, 10000));
}

void sx1262_cmd_pa_config(spi_device_handle_t radio, uint8_t pa_duty_cycle, uint8_t hp_max)
{
    const uint8_t device_sel = 0x00; // set to 0 for sx1262

    // set pa config. (power amplifier)
    // 0: Opcode, 1: paDutyCycle, 2: hpMax, 3: deviceSel (set to 0 for sx1262), 4: paLut (reserved and always 0x01)

    // paDutyCycle set to 0x02 is +14 dBm. !!! SHOULD NEVER BE HIGHER THAN 0x04
    uint8_t tx[5] = {SET_PA_CONFIG, pa_duty_cycle, hp_max, device_sel, 0x01};
    uint8_t rx[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
    // see table 13-21: PA Operating Modes with Optimal Settings

    ESP_ERROR_CHECK(send_command(radio, "Set Power amplifier config", tx, sizeof(tx), rx, 10000));
}

void sx1262_cmd_set_tx_params(spi_device_handle_t radio, int8_t power, uint8_t ramp_time)
{
    // set tx params, 13.4.4
    // dBm here needs to be +22 according to table 13-21, this pa config configuration I am using requires it. (value is 0x16)
    // rampTime 0x04 is 200 micro seconds
    uint8_t tx[3] = {SET_TX_PARAMS, power, ramp_time}; // I set power to 0x00 to make it quieter, less power
    uint8_t rx[3] = {0x00, 0x00, 0x00};

    ESP_ERROR_CHECK(send_command(radio, "set tx params", tx, sizeof(tx), rx, 10000));
}

void sx1262_cmd_set_modulation_params(spi_device_handle_t radio, uint8_t spread_factor, uint8_t bandwidth, uint8_t coding_rate, uint8_t low_data_rate_optimize_enabled)
{
    // set modulation params 13.4.5
    // both boards must use the same values to hear each other
    // spreading factor, bandwidth, coding rate, LowDataRateOptimize

    // see tables 13-47 to 13-50 for hex definitions needed here

    uint8_t tx[8] = {SET_MODULATION_PARAMS, spread_factor, bandwidth, coding_rate, low_data_rate_optimize_enabled, 0x00, 0x00, 0x00};
    uint8_t rx[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    ESP_ERROR_CHECK(send_command(radio, "set modulation params", tx, sizeof(tx), rx, 10000));
}

void sx1262_cmd_set_packet_params(spi_device_handle_t radio, uint16_t preamble_length, uint8_t header_type, uint8_t payload_length, uint8_t crc_type, uint8_t invert_iq)
{
    // set packet params 13.4.6
    // preamble length, header type, payload length, CRC, IQ
    // tables 13-66 to 13-70

    uint8_t pr_len_high = (preamble_length >> 8) & 0xFF;
    uint8_t pr_len_low = (preamble_length) & 0xFF;

    uint8_t tx[10] = {SET_PACKET_PARAMS, pr_len_high, pr_len_low, header_type, payload_length, crc_type, invert_iq, 0x00, 0x00, 0x00};
    uint8_t rx[10] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    ESP_ERROR_CHECK(send_command(radio, "set packet params", tx, sizeof(tx), rx, 10000));
}

void sx1262_cmd_set_dio2_as_rf_switch_ctrl(spi_device_handle_t radio)
{
    uint8_t tx[2] = {SET_DIO2_AS_RF_SWITCH_CTRL, 0x01}; // enable
    uint8_t rx[2] = {0x00, 0x00};

    ESP_ERROR_CHECK(send_command(radio, "set dio2 as rf switch ctrl", tx, sizeof(tx), rx, 10000));
}