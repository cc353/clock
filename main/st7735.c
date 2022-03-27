/******************************************************************************
** 
 * \file        st7735.c
 * \author      IOsetting | iosetting@outlook.com
 * \date        
 * \brief       Library of ST7735/ILI9163 TFT LCD on W806
 * \note        
 * \version     v0.1
 * \ingroup     demo
 * \remarks     test-board: HLK-W806-KIT-V1.0
 *              
 * 
******************************************************************************/

#include "st7735.h"

static const uint8_t
    init_cmds_r[] = {                      // Init for 7735R, part 1 (red or green tab)
        15,                               // 15 commands in list:
        ST77XX_SWRESET, ST77XX_CMD_DELAY, //  1: Software reset, 0 args, w/delay
        150,                              //     150 ms delay
        ST77XX_SLPOUT, ST77XX_CMD_DELAY,  //  2: Out of sleep mode, 0 args, w/delay
        255,                              //     500 ms delay
        ST7735_FRMCTR1, 3,                //  3: Frame rate ctrl - normal mode, 3 args:
        0x05, 0x3C, 0x3C,                 //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
        ST7735_FRMCTR2, 3,                //  4: Frame rate control - idle mode, 3 args:
        0x05, 0x3C, 0x3C,                 //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
        ST7735_FRMCTR3, 6,                //  5: Frame rate ctrl - partial mode, 6 args:
        0x05, 0x3C, 0x3C,                 //     Dot inversion mode
        0x05, 0x3C, 0x3C,                 //     Line inversion mode
        ST7735_INVCTR , 1,                //  6: Display inversion ctrl, 1 arg, no delay:
        0x03,                             //     
        ST7735_PWCTR1 , 3,                //  7: Power control, 3 args, no delay:
        0x28,
        0x08,                             //     
        0x04,                             //     
        ST7735_PWCTR2 , 1,                //  8: Power control, 1 arg, no delay:
        0xC0,                             //     
        ST7735_PWCTR3 , 2,                //  9: Power control, 2 args, no delay:
        0x0D,                             //     
        0x00,                             //     
        ST7735_PWCTR4 , 2,                // 10: Power control, 2 args, no delay:
        0x8D,                             //
        0x2A,  
        ST7735_PWCTR5 , 2,                // 11: Power control, 2 args, no delay:
        0x8D, 0xEE,
        ST7735_VMCTR1 , 1,                // 12: Power control, 1 arg, no delay:
        0x1A,
        ST77XX_INVOFF , 0,                // 13: Don't invert display, no args, no delay
        ST77XX_MADCTL , 1,                // 14: Memory access control (directions), 1 arg:
        ST77XX_ROTATION,                  //     
        ST77XX_COLMOD , 1,                // 15: set color mode, 1 arg, no delay:
        0x05                              //     16-bit color
    },

    init_cmds2[] = {
        2,                                //  2 commands in list:
        ST77XX_CASET, 4,                  //  1: Column addr set, 4 args, no delay:
        0x00, ST77XX_XSTART,              //     XSTART
        0x00, ST77XX_XSTART + ST77XX_WIDTH - 1,  // XEND
        ST77XX_RASET  , 4,                //  2: Row addr set, 4 args, no delay:
        0x00, ST77XX_YSTART,              //     YSTART
        0x00, ST77XX_YSTART + ST77XX_HEIGHT - 1, // YEND
    },

    init_cmds3[] = {
        4,                                //  4 commands in list:
        ST7735_GMCTRP1, 16,               //  1: Magical unicorn dust, 16 args, no delay:
        0x04, 0x22, 0x07, 0x0A,
        0x2E, 0x30, 0x25, 0x2A,
        0x28, 0x26, 0x2E, 0x3A,
        0x00, 0x01, 0x03, 0x13,
        ST7735_GMCTRN1, 16,             //  2: Sparkles and rainbows, 16 args, no delay:
        0x04, 0x16, 0x06, 0x0D,
        0x2D, 0x26, 0x23, 0x27,
        0x27, 0x25, 0x2D, 0x3B,
        0x00, 0x01, 0x04, 0x13,
        ST77XX_NORON, ST77XX_CMD_DELAY, //  3: Normal display on, no args, w/delay
        10,                             //     10 ms delay
        ST77XX_DISPON, ST77XX_CMD_DELAY, // 4: Main screen turn on, no args w/delay
        100                             //     100 ms delay
    };

void ST7735_Init(void)
{
    ST77XX_Init();
    ST77XX_Reset();
    printf("ST77XX_Init\n");
    ST77XX_ExecuteCommandList(init_cmds_r);
    ST77XX_ExecuteCommandList(init_cmds2);
    ST77XX_ExecuteCommandList(init_cmds3);
    printf("ST77XX_ExecuteCommandList\n");
    ST77XX_BackLight_On();
}
