#include <stdio.h>
#include <stdint.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "hal/gpio_types.h"

#define LED_PIN GPIO_NUM_21
#define BUTTON_PIN GPIO_NUM_0
#define DEBOUNCE_US 50000 // 50ms

static QueueHandle_t button_queue;

// ISR: runs an interrupt.
static void IRAM_ATTR button_isr(void *arg) {
    uint32_t pin = (uint32_t)arg;
    BaseType_t woke = pdFALSE; // what is this
    xQueueSendFromISR(button_queue, &pin, &woke); // why does it have to be xQueueSendFROmISR and not just xQueueSend?
    portYIELD_FROM_ISR(woke); // what is this? "woke" from what?
}

static void button_task(void *arg) {
    uint32_t pin;
    int64_t last_press_us = 0; // is us micro seconds?
    int led_on = 0; // why all the different integer types here?

    for (;;) {
        if (xQueueReceive(button_queue, &pin, portMAX_DELAY)) {

            int64_t now = esp_timer_get_time(); // what is this
            if (now - last_press_us < DEBOUNCE_US) { // explain this logic
                continue; 
            }
            printf("timestamp: %lld, ", now);
            if (now - last_press_us < 400000) {
                printf("(Short press), ");
            } else {
                printf("(Long press), ");
            }
            last_press_us = now;
            
            led_on = !led_on;
            gpio_set_level(LED_PIN, led_on ? 0 : 1);
            printf("button pressed: LED %s\n", led_on ? "ON" : "OFF");
        }
    }
}

void app_main(void)
{
    gpio_config_t led = {
        .pin_bit_mask = 1ULL << LED_PIN, // if this is here, what is the point of gpio_set_level?
        .mode = GPIO_MODE_OUTPUT
    };
    ESP_ERROR_CHECK(gpio_config(&led)); // what does this do
    gpio_set_level(LED_PIN, 1);

    gpio_config_t btn = {
        .pin_bit_mask = 1ULL << BUTTON_PIN, 
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_ANYEDGE, // falling edge? what does that mean?
    };
    ESP_ERROR_CHECK(gpio_config(&btn)); // does gpio_config() apply the configuration of the gpio_config_t struct?

    button_queue = xQueueCreate(10, sizeof(uint32_t));
    xTaskCreate(button_task, "button", 4096, NULL, 10, NULL);

    ESP_ERROR_CHECK(gpio_install_isr_service(0)); // explain
    ESP_ERROR_CHECK(gpio_isr_handler_add(BUTTON_PIN, button_isr, (void *)BUTTON_PIN));
}

// void app_main(void)
// {
//     const gpio_config_t cfg = {
//         .pin_bit_mask = 1ULL << LED_PIN,
//         .mode         = GPIO_MODE_OUTPUT,
//         .pull_up_en   = GPIO_PULLUP_DISABLE,
//         .pull_down_en = GPIO_PULLDOWN_DISABLE,
//         .intr_type    = GPIO_INTR_DISABLE,
//     };
//     ESP_ERROR_CHECK(gpio_config(&cfg));
    
//     for (;;) {
//         gpio_set_level(LED_PIN, 0);
//         vTaskDelay(pdMS_TO_TICKS(500));
//         gpio_set_level(LED_PIN, 1);
//         vTaskDelay(pdMS_TO_TICKS(500));
//     }
// }