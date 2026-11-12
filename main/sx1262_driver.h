#pragma once
#include <stdint.h>
#include <stdio.h>
#include "driver/spi_common.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "hal/gpio_types.h"
#include "hal/spi_types.h"

#define PIN_RESET 42
#define PIN_BUSY 40

// sx1262 commands
#define NO_OP 0x00
#define GET_STATUS 0xC0
#define GET_DEVICE_ERRORS 0x17
#define CLEAR_DEVICE_ERRORS 0x07
#define SET_DIO3_AS_TCXO_CTRL 0x97
#define CALIBRATE 0x89
#define READ_REGISTER 0x1D
#define SET_STANDBY 0x80
#define SET_PACKET_TYPE 0x8A
#define SET_RF_FREQUENCY 0x86
#define CALIBRATE_IMAGE 0x98
#define SET_PA_CONFIG 0x95
#define SET_TX_PARAMS 0x8E
#define SET_MODULATION_PARAMS 0x8B
#define SET_PACKET_PARAMS 0x8C
#define SET_DIO2_AS_RF_SWITCH_CTRL 0x9D

#define TCXO_TIMETOUT_US 5000 // tcxo

esp_err_t radio_wait_ready(int32_t timeout);

esp_err_t send_command(spi_device_handle_t device_handle,
                       const char *command_name,
                       const uint8_t *tx,
                       size_t tx_len,
                       uint8_t *rx,
                       int32_t timeout);

// sx1262 driver commands

void sx1262_cmd_get_status(spi_device_handle_t radio);

void sx1262_cmd_get_device_errors(spi_device_handle_t radio);

void sx1262_cmd_clear_device_errors(spi_device_handle_t radio);

void sx1262_cmd_set_dio3_as_tcxo_ctrl(spi_device_handle_t radio, uint8_t tcxo_voltage);

void sx1262_cmd_calibrate(spi_device_handle_t radio);

esp_err_t sx1262_cmd_read_register(spi_device_handle_t radio, uint16_t register_addr,  uint8_t *rx, size_t rx_len);

void sx1262_cmd_set_standby(spi_device_handle_t radio);

void sx1262_cmd_set_packet_type(spi_device_handle_t radio);

void sx1262_cmd_set_rf_frequency(spi_device_handle_t radio, uint64_t frequency_mhz);

void sx1262_cmd_calibrate_image(spi_device_handle_t radio);

void sx1262_cmd_pa_config(spi_device_handle_t radio, uint8_t pa_duty_cycle, uint8_t hp_max);

void sx1262_cmd_set_tx_params(spi_device_handle_t radio, int8_t power, uint8_t ramp_time);

void sx1262_cmd_set_modulation_params(spi_device_handle_t radio, uint8_t spread_factor, uint8_t bandwidth, uint8_t coding_rate, uint8_t low_data_rate_optimize_enabled);

void sx1262_cmd_set_packet_params(spi_device_handle_t radio, uint16_t preamble_length, uint8_t header_type, uint8_t payload_length, uint8_t crc_type, uint8_t invert_iq);

void sx1262_cmd_set_dio2_as_rf_switch_ctrl(spi_device_handle_t radio);