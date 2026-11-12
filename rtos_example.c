#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"

TaskHandle_t myTaskHandleA = NULL;
TaskHandle_t myTaskHandleB = NULL;

void Demo_Task_A(void *arg) {
    int count = 0;
    while (1) {
        count++;
        printf("Demo_Task_A printing....%d\n", count);
        if (count == 10)
        {
            printf("Demo_Task_A resumed Demo_Task_B\n");
            vTaskResume(myTaskHandleB);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void Demo_Task_B(void *arg)
{
    int count = 0;
    while (1)
    {
        count++;
        printf("Demo_Task_B printing...%d\n", count);
        if (count == 5)
        {
            // Suspend itself: The scheduler will no longer schedule this task until it is resumed by another task's vTaskResume
            printf("Demo_Task_B suspended itself\n");
            vTaskSuspend(NULL);
        }
        if (count == 10)
        {
            printf("Demo_Task_B deleted itself\n");
            vTaskDelete(NULL);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void)
{
    // Create tasks and pin them to specific cores; stack size 4096 bytes; both tasks share priority 10
    // Task A pinned to Core 0, Task B pinned to Core 1 (can run in parallel on dual-core)
    xTaskCreatePinnedToCore(Demo_Task_A, "Demo_Task_A", 4096, NULL, 10, &myTaskHandleA, 0);
    xTaskCreatePinnedToCore(Demo_Task_B, "Demo_Task_B", 4096, NULL, 10, &myTaskHandleB, 1);
}