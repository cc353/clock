/******************************************************************************
** 
 * \file        st77xx.c
 * \author      IOsetting | iosetting@outlook.com
 * \date        
 * \brief       Library of ST77XX TFT LCD on W806
 * \note        
 * \version     v0.1
 * \ingroup     demo
 * \remarks     test-board: HLK-W806-KIT-V1.0
 *              
 *              B10   -> RES, RESET
 *              B11   -> DC, CD
 *              B14   -> CS, Chip Select
 *              B15   -> SCK, SCL, CLK, Clock
 *              B16   -> BL, Back Light
 *              B17   -> MOSI, SDA
 *              GND   -> GND
 *              3.3V  -> VCC
 * 
 *              ST77XX_HARDWARE_SPI - 0: Software SPI, 1: Hardware SPI
 * 
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "st77xx.h"

#define LCD_HOST    SPI2_HOST

#define HAL_MAX_DELAY 0xffff

//To speed up transfers, every SPI transfer sends a bunch of lines. This define specifies how many. More means more memory use,
//but less overhead for setting up / finishing transfers. Make sure 240 is dividable by this.
#define PARALLEL_LINES 16

static uint8_t st77xx_buf[ST77XX_BUF_SIZE];
static uint16_t st77xx_buf_pt = 0;


#if ST77XX_HARDWARE_SPI
spi_device_handle_t spiHander;

//This function is called (in irq context!) just before a transmission starts. It will
//set the D/C line to the value indicated in the user field.
void lcd_spi_pre_transfer_callback(spi_transaction_t *trans)
{
    int dc= (int)trans->user;
    gpio_set_level(ST77XX_DC_PIN, dc);
}

static void initSpi()
{
    esp_err_t ret;
    spi_bus_config_t buscfg = {
        .miso_io_num = -1,
        .mosi_io_num = ST77XX_MOSI_PIN,
        .sclk_io_num = ST77XX_SCK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = PARALLEL_LINES * ST77XX_WIDTH * 2 + 8
    };
    spi_device_interface_config_t devcfg={
        .flags = SPI_DEVICE_HALFDUPLEX, // TX only
        .clock_speed_hz = 40*1000*1000,           //Clock
        .mode = 0,                                //SPI mode 0
        .spics_io_num = ST77XX_CS_PIN,               //CS pin
        .queue_size = 10,                          //We want to be able to queue 7 transactions at a time
        .pre_cb = lcd_spi_pre_transfer_callback,  //Specify pre-transfer callback to handle D/C line
        .post_cb = NULL,
    };
    //Initialize the SPI bus
    ret = spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);
    //Attach the LCD to the SPI bus
    ret = spi_bus_add_device(LCD_HOST, &devcfg, &spiHander);
    ESP_ERROR_CHECK(ret);
    printf("spi hander: %x\n", (void*)spiHander);
}
#endif

void ST77XX_Init(void)
{
    gpio_config_t io_config = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = 0,
        .pull_up_en = 0,
    };

#if !ST77XX_HARDWARE_SPI
    printf("init gpio cs sck mosi\n");
    io_config.pin_bit_mask = (1ULL << ST77XX_CS_PIN)
                            | (1ULL << ST77XX_SCK_PIN)
                            | (1ULL << ST77XX_MOSI_PIN)
                            | (1ULL << ST77XX_DC_PIN)
                            | (1ULL << ST77XX_RES_PIN)
                            | (1ULL << ST77XX_BL_PIN);
#else
    printf("init spi\n");
    initSpi();
    io_config.pin_bit_mask = (1ULL << ST77XX_DC_PIN)
                            | (1ULL << ST77XX_RES_PIN)
                            | (1ULL << ST77XX_BL_PIN);
#endif

    gpio_config(&io_config);
}


void ST77XX_Reset(void)
{
    ST77XX_RESET_LOW;
    vTaskDelay(10 / portTICK_RATE_MS);
    ST77XX_RESET_HIGH;
}

static void ST77XX_TransmitByte(uint8_t dat)
{
#if ST77XX_HARDWARE_SPI
    // ST77XX_CS_LOW;
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &dat,
        .user = (void*)0
    };
    ESP_ERROR_CHECK(spi_device_polling_transmit(spiHander, &t));
    // ST77XX_CS_HIGH;
#else
    uint8_t i;
    ST77XX_CS_LOW;
    for (i = 0; i < 8; i++)
    {
        ST77XX_SCK_LOW;
        if (dat & 0x80)
        {
            ST77XX_MOSI_HIGH;
        }
        else
        {
            ST77XX_MOSI_LOW;
        }
        ST77XX_SCK_HIGH;
        dat <<= 1;
    }
    ST77XX_CS_HIGH;
#endif
}

static void ST77XX_Transmit(const uint8_t *pData, uint32_t Size, uint32_t Timeout)
{
#if ST77XX_HARDWARE_SPI
    // ST77XX_CS_LOW;
    spi_transaction_t t = {
        .length = Size * 8,
        .tx_buffer = pData,
        .user = (void*)1
    };
    ESP_ERROR_CHECK(spi_device_polling_transmit(spiHander, &t));
    // ST77XX_CS_HIGH;
#else
    while (Size-- > 0)
    {
        ST77XX_TransmitByte(*(pData++));
    }
#endif
}

static void ST77XX_WriteCommand(uint8_t dat)
{
    ST77XX_DC_LOW;
    ST77XX_TransmitByte(dat);
    ST77XX_DC_HIGH;
}

static void ST77XX_WriteData(const uint8_t* buff, size_t buff_size)
{
    ST77XX_Transmit(buff, buff_size, HAL_MAX_DELAY);
}

static void ST77XX_WriteBuff(uint8_t* buff, size_t buff_size)
{
    while (buff_size--)
    {
        st77xx_buf[st77xx_buf_pt++] = *buff++;
        if (st77xx_buf_pt == ST77XX_BUF_SIZE)
        {
            ST77XX_Transmit(st77xx_buf, st77xx_buf_pt, HAL_MAX_DELAY);
            st77xx_buf_pt = 0;
        }
    }
}

static void ST77XX_FlushBuff(void)
{
    if (st77xx_buf_pt > 0)
    {
        ST77XX_Transmit(st77xx_buf, st77xx_buf_pt, HAL_MAX_DELAY);
        st77xx_buf_pt = 0;
    }
}

void ST77XX_ExecuteCommandList(const uint8_t *addr)
{
    uint8_t numCommands, numArgs;
    uint16_t ms;

    numCommands = *addr++;
    while (numCommands--)
    {
        uint8_t cmd = *addr++;
        ST77XX_WriteCommand(cmd);

        numArgs = *addr++;
        // If high bit set, delay follows args
        ms = numArgs & ST77XX_CMD_DELAY;
        numArgs &= ~ST77XX_CMD_DELAY;
        if (numArgs)
        {
            ST77XX_WriteData(addr, numArgs);
            addr += numArgs;
        }

        if (ms)
        {
            ms = *addr++;
            if (ms == 255)
                ms = 500;
            vTaskDelay(ms / portTICK_RATE_MS);
        }
    }
}

static void ST77XX_SetAddrWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    // column address set
    ST77XX_WriteCommand(ST77XX_CASET);
    x1 = x1 + ST77XX_XSTART;
    x2 = x2 + ST77XX_XSTART;
    uint8_t data[] = { x1 >> 8, x1 & 0xFF, x2 >> 8, x2 % 0xFF };
    ST77XX_WriteData(data, sizeof(data));

    // row address set
    ST77XX_WriteCommand(ST77XX_RASET);
    y1 = y1 + ST77XX_YSTART;
    y2 = y2 + ST77XX_YSTART;
    data[0] = y1 >> 8;
    data[1] = y1 & 0xFF;
    data[2] = y2 >> 8;
    data[3] = y2 & 0xFF;
    ST77XX_WriteData(data, sizeof(data));

    // write to RAM
    ST77XX_WriteCommand(ST77XX_RAMWR);
}

void ST77XX_BackLight_On(void)
{
    ST77XX_BL_HIGH;
}

void ST77XX_BackLight_Off(void)
{
    ST77XX_BL_LOW;
}

void ST77XX_Fill(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end, uint16_t color)
{
    uint16_t i,j;
    ST77XX_SetAddrWindow(x_start, y_start, x_end - 1, y_end - 1);
    for(i = y_start; i < y_end; i++)
    {
        for( j = x_start; j < x_end; j++)
        {
            ST77XX_WriteBuff((uint8_t *)&color, 2);
        }
    }
    ST77XX_FlushBuff();
}

void ST77XX_DrawPoint(uint16_t x, uint16_t y, uint16_t color)
{
    ST77XX_SetAddrWindow( x, y, x, y);
    ST77XX_WriteData((uint8_t *)&color, 2);
}

void ST77XX_DrawLine(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end, uint16_t color)
{
    uint16_t t;
    int xerr = 0, yerr=0, delta_x, delta_y, distance;
    int incx, incy, uRow, uCol;
    delta_x = x_end - x_start; //计算坐标增量 
    delta_y = y_end - y_start;
    uRow = x_start;//画线起点坐标
    uCol = y_start;

    if (delta_x > 0)
        incx = 1; //设置单步方向 
    else if (delta_x == 0)
        incx=0; //垂直线
    else
    {
        incx = -1;
        delta_x = -delta_x;
    }

    if (delta_y > 0)
        incy = 1;
    else if (delta_y == 0)
        incy=0; //水平线 
    else
    {
        incy = -1;
        delta_y = -delta_y;
    }
    
    if (delta_x > delta_y)
        distance = delta_x; //选取基本增量坐标轴 
    else
        distance = delta_y;

    for (t = 0; t < distance + 1; t++)
    {
        ST77XX_DrawPoint(uRow, uCol, color);
        xerr += delta_x;
        yerr += delta_y;
        if(xerr > distance)
        {
            xerr -= distance;
            uRow += incx;
        }
        if(yerr>distance)
        {
            yerr -= distance;
            uCol += incy;
        }
    }
}

void ST77XX_DrawRectangle(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end, uint16_t color)
{
    ST77XX_DrawLine(x_start, y_start, x_end,   y_start, color);
    ST77XX_DrawLine(x_start, y_start, x_start, y_end,   color);
    ST77XX_DrawLine(x_start, y_end,   x_end,   y_end,   color);
    ST77XX_DrawLine(x_end,   y_start, x_end,   y_end,   color);
}

void ST77XX_DrawCircle(uint16_t x, uint16_t y, uint8_t radius, uint16_t color)
{
    int a = 0, b = radius;
    while (a <= b)
    {
        ST77XX_DrawPoint(x - b, y - a, color);
        ST77XX_DrawPoint(x - b, y + a, color);
        ST77XX_DrawPoint(x + b, y - a, color);
        ST77XX_DrawPoint(x + b, y + a, color);
        ST77XX_DrawPoint(x + a, y - b, color);
        ST77XX_DrawPoint(x - a, y - b, color);
        ST77XX_DrawPoint(x + a, y + b, color);
        ST77XX_DrawPoint(x - a, y + b, color);
        a++;
        if ((a * a + b * b) > (radius * radius))
        {
            b--;
        }
    }
}

void ST77XX_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data)
{
    if ((x >= ST77XX_WIDTH) || (y >= ST77XX_HEIGHT))
        return;
    if ((x + w - 1) >= ST77XX_WIDTH)
        return;
    if ((y + h - 1) >= ST77XX_HEIGHT)
        return;

    ST77XX_SetAddrWindow(x, y, x + w - 1, y + h - 1);
    ST77XX_WriteData((uint8_t *)data, sizeof(uint16_t) * w * h);
}

void ST77XX_DrawChar(uint16_t x, uint16_t y, char ch, FontDef_t* font, uint16_t color, uint16_t bgcolor)
{
    uint8_t b, i, j, k, bytes;

    ST77XX_SetAddrWindow(x, y, x + font->width - 1, y + font->height - 1);
    bytes = font->width / 8 + ((font->width % 8)? 1 : 0);

    for (i = 0; i < font->height; i++)
    {
        for (j = 0; j < bytes; j++)
        {
            b = font->data[((ch - 32) * font->height + i) * bytes + j];
            if (font->order == 0)
            {
                for (k = 0; k < 8 && k < font->width - j * 8; k++)
                {
                    if ((b << k) & 0x80)
                    {
                        ST77XX_WriteBuff((uint8_t *)&color, 2);
                    }
                    else
                    {
                        ST77XX_WriteBuff((uint8_t *)&bgcolor, 2);
                    }
                }
            }
            else
            {
                for (k = 0; k < 8 && k < font->width - j * 8; k++)
                {
                    if (b & (0x0001 << k))
                    {
                        ST77XX_WriteBuff((uint8_t *)&color, 2);
                    }
                    else
                    {
                        ST77XX_WriteBuff((uint8_t *)&bgcolor, 2);
                    }
                }
            }
        }

    }
    ST77XX_FlushBuff();
}

void ST77XX_DrawString(uint16_t x, uint16_t y, const char *p, FontDef_t* font, uint16_t color, uint16_t bgcolor)
{
    while (*p != '\0')
    {
        ST77XX_DrawChar(x, y, *p, font, color, bgcolor);
        x += font->width;
        p++;
    }
}
