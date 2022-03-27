/* SPI Master example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "lvgl.h"
#include "demos/lv_demos.h"
#include "examples/lv_examples.h"
#include "st7735.h"
#include "input.h"
#include "my_sntp.h"

#define LV_TICK_PERIOD_MS 1

#define SCREEN_W ST77XX_WIDTH
#define SCREEN_H ST77XX_HEIGHT

#define BUF_LINES 24

static lv_disp_draw_buf_t disp_buf;

static lv_color_t buf_1[SCREEN_W * BUF_LINES];
static lv_color_t buf_2[SCREEN_W * BUF_LINES];

static lv_disp_drv_t disp_drv;

static lv_key_t lastKey = 0;
static bool lastKeyPress = false;

static void input_callback(Key key, bool press)
{
    printf("input %d %d \n", key, press ? 1 : 0);
    lastKeyPress = press;
    if (press)
    {
        switch (key)
        {
        case Up:
            lastKey = LV_KEY_UP;
            break;

        case Down:
            lastKey = LV_KEY_DOWN;
            break;

        case Left:
            lastKey = LV_KEY_PREV;
            break;

        case Right:
            lastKey = LV_KEY_NEXT;
            break;

        case Enter:
            lastKey = LV_KEY_ENTER;
            break;
        }
    }
}

void my_flush_cb(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
    ST77XX_DrawImage(area->x1, area->y1, area->x2 - area->x1 + 1, area->y2 - area->y1 + 1, (uint16_t *)color_p);

    lv_disp_flush_ready(disp_drv);
}

static void lv_tick_task(void *arg)
{
    (void)arg;

    lv_tick_inc(LV_TICK_PERIOD_MS);
}

static void keyboard_read(lv_indev_drv_t * drv, lv_indev_data_t*data)
{
  data->key = lastKey;
  data->state = lastKeyPress ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

static void init()
{
    lv_init();

    input_init(&input_callback);

    ST7735_Init();
    printf("ST7735 Inited\n");

    lv_disp_draw_buf_init(&disp_buf, buf_1, buf_2, SCREEN_W * BUF_LINES);
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf = &disp_buf;
    disp_drv.flush_cb = my_flush_cb;
    disp_drv.hor_res = SCREEN_W;
    disp_drv.ver_res = SCREEN_H;
    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_KEYPAD;
    indev_drv.read_cb = keyboard_read;
    /*Register the driver in LVGL and save the created input device object*/
    lv_indev_t * my_indev = lv_indev_drv_register(&indev_drv);

    lv_group_t* group = lv_group_get_default();
    if (!group)
    {
        group = lv_group_create();
        if (group)
        {
            lv_group_set_default(group);
        }
    }

    lv_indev_set_group(my_indev, group);

    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lv_tick_task,
        .name = "periodic_gui"};
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, LV_TICK_PERIOD_MS * 1000));
}

static void update_label_timer(lv_timer_t * timer)
{
    static char week[7][5] = {"天", "一", "二", "三", "四", "五", "六"};

    lv_obj_t** p = (lv_obj_t**)timer->user_data;
    lv_obj_t* labelTime = *p;
    lv_obj_t* labelDate = *(p + 1);

    struct timeval tv;
    struct tm timeinfo;

    gettimeofday(&tv, NULL);
    localtime_r(&tv.tv_sec, &timeinfo);

    lv_label_set_text_fmt(
        labelTime,
        "%02d:%02d:%02d.%03ld",
        timeinfo.tm_hour,
        timeinfo.tm_min,
        timeinfo.tm_sec,
        tv.tv_usec / 1000
    );
    lv_label_set_text_fmt(
        labelDate,
        "%04d年%d月%02d 星期%s",
        timeinfo.tm_year + 1900,
        timeinfo.tm_mon + 1,
        timeinfo.tm_mday,
        week[timeinfo.tm_wday]
    );
}

void app_main(void)
{
    printf("hello clock\n");
    init();
    printf("init\n");

    my_sntp_init();

    static lv_style_t styleTime, styleDate;
    lv_style_init(&styleTime);
    lv_style_set_text_color(&styleTime, lv_color_make(0, 0xa0, 0));
    lv_style_set_text_font(&styleTime, &lv_font_montserrat_22);

    lv_style_init(&styleDate);
    lv_style_set_text_color(&styleDate, lv_color_make(0, 0xa0, 0));
    lv_style_set_text_font(&styleDate, &lv_font_simsun_16_cjk);

    lv_obj_t* labelTime = lv_label_create(lv_scr_act());
    lv_label_set_text(labelTime, "");
    lv_obj_center(labelTime);
    lv_obj_align(labelTime, LV_ALIGN_LEFT_MID, 5, -10);
    lv_obj_add_style(labelTime, &styleTime, 0);

    lv_obj_t* labelDate = lv_label_create(lv_scr_act());
    lv_label_set_text(labelDate, "");
    lv_obj_align_to(labelDate, labelTime, LV_ALIGN_BOTTOM_LEFT, 0, 15);
    lv_obj_add_style(labelDate, &styleDate, 0);

    lv_obj_t* labels[] = {labelTime, labelDate};
    lv_timer_t * timer = lv_timer_create(update_label_timer, 1, &labels);
    lv_timer_ready(timer);

    while (1)
    {
        lv_task_handler();
        vTaskDelay(pdMS_TO_TICKS(1));
    }

}
