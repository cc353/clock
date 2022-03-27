/* GPIO Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "input.h"

#define delayMs(ms) vTaskDelay((ms) / portTICK_PERIOD_MS)

/**
 * Brief:
 * This test code shows how to configure gpio and how to use gpio interrupt.
 *
 * GPIO status:
 * GPIO18: output
 * GPIO19: output
 * GPIO4:  input, pulled up, interrupt from rising edge and falling edge
 * GPIO5:  input, pulled up, interrupt from rising edge.
 *
 * Test:
 * Connect GPIO18 with GPIO4
 * Connect GPIO19 with GPIO5
 * Generate pulses on GPIO18/19, that triggers interrupt on GPIO4/5
 *
 */

#define GPIO_OUTPUT_IO_0    13
#define GPIO_OUTPUT_IO_1    13
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_OUTPUT_IO_0) | (1ULL<<GPIO_OUTPUT_IO_1))
#define GPIO_INPUT_IO_UP    1
#define GPIO_INPUT_IO_DOWN  12
#define GPIO_INPUT_IO_LEFT  0
#define GPIO_INPUT_IO_RIGHT  18
#define GPIO_INPUT_IO_ENTER  19
#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INPUT_IO_UP) | (1ULL<<GPIO_INPUT_IO_DOWN) | (1ULL<<GPIO_INPUT_IO_LEFT) | (1ULL<<GPIO_INPUT_IO_RIGHT) | (1ULL<<GPIO_INPUT_IO_ENTER))
#define ESP_INTR_FLAG_DEFAULT 0

static int blinkDelay = 1000;

static xQueueHandle gpio_evt_queue = NULL;

static InputCallback input_callback = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void gpio_task_example(void* arg)
{
    static int kUp = 1, kDown = 1, kLeft = 1, kRight = 1, kEnter = 1;

    uint32_t io_num;
    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            // printf("GPIO[%d] intr, val: %d\n", io_num, gpio_get_level(io_num));
            int level = gpio_get_level(io_num);
            switch (io_num)
            {
            case GPIO_INPUT_IO_UP:
                if (level != kUp)
                {
                    kUp = level;
                    // printf("key up %d\n", kUp);
                    input_callback(Up, kUp == 0);
                }
                break;
            case GPIO_INPUT_IO_DOWN:
                if (level != kDown)
                {
                    kDown = level;
                    // printf("key down %d\n", kDown);
                    input_callback(Down, kDown == 0);
                }
                break;
            case GPIO_INPUT_IO_LEFT:
                if (level != kLeft)
                {
                    kLeft = level;
                    // printf("key left %d\n", kLeft);
                    input_callback(Left, kLeft == 0);
                }
                break;
            case GPIO_INPUT_IO_RIGHT:
                if (level != kRight)
                {
                    kRight = level;
                    // printf("key right %d\n", kRight);
                    input_callback(Right, kRight == 0);
                }
                break;
            case GPIO_INPUT_IO_ENTER:
                if (level != kEnter)
                {
                    kEnter = level;
                    // printf("key enter %d\n", kEnter);
                    input_callback(Enter, kEnter == 0);
                }
                break;
            }
        }
    }
}

void input_init(InputCallback callback)
{
    input_callback = callback;

    //zero-initialize the config structure.
    gpio_config_t io_conf = {};
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

    //interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    //change gpio intrrupt type for one pin
    // gpio_set_intr_type(GPIO_INPUT_IO_UP, GPIO_INTR_ANYEDGE);

    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    //start gpio task
    xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_IO_UP, gpio_isr_handler, (void*) GPIO_INPUT_IO_UP);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_IO_DOWN, gpio_isr_handler, (void*) GPIO_INPUT_IO_DOWN);
    gpio_isr_handler_add(GPIO_INPUT_IO_LEFT, gpio_isr_handler, (void*) GPIO_INPUT_IO_LEFT);
    gpio_isr_handler_add(GPIO_INPUT_IO_RIGHT, gpio_isr_handler, (void*) GPIO_INPUT_IO_RIGHT);
    gpio_isr_handler_add(GPIO_INPUT_IO_ENTER, gpio_isr_handler, (void*) GPIO_INPUT_IO_ENTER);

    //remove isr handler for gpio number.
    // gpio_isr_handler_remove(GPIO_INPUT_IO_UP);
    //hook isr handler for specific gpio pin again
    // gpio_isr_handler_add(GPIO_INPUT_IO_UP, gpio_isr_handler, (void*) GPIO_INPUT_IO_UP);

    printf("Minimum free heap size: %d bytes\n", esp_get_minimum_free_heap_size());

    // int cnt = 0;
    // while(1) {
    //     cnt++;
    //     vTaskDelay(blinkDelay / portTICK_RATE_MS);
    //     gpio_set_level(GPIO_OUTPUT_IO_0, cnt % 2);
    //     gpio_set_level(GPIO_OUTPUT_IO_1, cnt % 2);
    // }
}
