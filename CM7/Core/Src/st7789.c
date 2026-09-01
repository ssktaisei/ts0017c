/*
 * st7789.c
 *
 *  Created on: 2026/09/01
 *      Author: sata
 */

#include "st7789.h"
#include "main.h"

extern SPI_HandleTypeDef hspi1;

/*
 * Change these to match your CubeMX configuration.
 */
#define ST7789_CS_GPIO_Port    GPIOD
#define ST7789_CS_Pin          GPIO_PIN_14

#define ST7789_DC_GPIO_Port    GPIOD
#define ST7789_DC_Pin          GPIO_PIN_3

#define ST7789_RST_GPIO_Port   GPIOD
#define ST7789_RST_Pin         GPIO_PIN_4


#define CS_LOW()    HAL_GPIO_WritePin(ST7789_CS_GPIO_Port, \
                                      ST7789_CS_Pin, \
                                      GPIO_PIN_RESET)

#define CS_HIGH()   HAL_GPIO_WritePin(ST7789_CS_GPIO_Port, \
                                      ST7789_CS_Pin, \
                                      GPIO_PIN_SET)

#define DC_COMMAND() HAL_GPIO_WritePin(ST7789_DC_GPIO_Port, \
                                        ST7789_DC_Pin, \
                                        GPIO_PIN_RESET)

#define DC_DATA()    HAL_GPIO_WritePin(ST7789_DC_GPIO_Port, \
                                       ST7789_DC_Pin, \
                                       GPIO_PIN_SET)

#define RST_LOW()   HAL_GPIO_WritePin(ST7789_RST_GPIO_Port, \
                                      ST7789_RST_Pin, \
                                      GPIO_PIN_RESET)

#define RST_HIGH()  HAL_GPIO_WritePin(ST7789_RST_GPIO_Port, \
                                      ST7789_RST_Pin, \
                                      GPIO_PIN_SET)


/* ST7789 commands */
#define ST7789_SWRESET   0x01
#define ST7789_SLPIN     0x10
#define ST7789_SLPOUT    0x11
#define ST7789_NORON     0x13

#define ST7789_INVOFF    0x20
#define ST7789_INVON     0x21

#define ST7789_DISPOFF   0x28
#define ST7789_DISPON    0x29

#define ST7789_CASET     0x2A
#define ST7789_RASET     0x2B
#define ST7789_RAMWR    0x2C

#define ST7789_MADCTL    0x36
#define ST7789_COLMOD    0x3A


static void ST7789_WriteByte(uint8_t data)
{
    HAL_SPI_Transmit(&hspi1, &data, 1, HAL_MAX_DELAY);
}


void ST7789_WriteCommand(uint8_t cmd)
{
    CS_LOW();

    DC_COMMAND();
    ST7789_WriteByte(cmd);

    CS_HIGH();
}


void ST7789_WriteData(uint8_t *data, uint16_t size)
{
    CS_LOW();

    DC_DATA();

    HAL_SPI_Transmit(&hspi1,
                     data,
                     size,
                     HAL_MAX_DELAY);

    CS_HIGH();
}


static void ST7789_WriteDataByte(uint8_t data)
{
    DC_DATA();
    ST7789_WriteByte(data);
}


/*
 * Hardware reset
 */
static void ST7789_Reset(void)
{
    RST_HIGH();
    HAL_Delay(5);

    RST_LOW();
    HAL_Delay(20);

    RST_HIGH();
    HAL_Delay(120);
}


/*
 * ST7789 initialization for
 * Adafruit 2.0" 320x240 display.
 *
 * This follows the basic initialization used
 * by Adafruit's ST7789 driver.
 */
void ST7789_Init(void)
{
    CS_HIGH();
    DC_DATA();

    ST7789_Reset();

    ST7789_WriteCommand(ST7789_SWRESET);
    HAL_Delay(150);

    ST7789_WriteCommand(ST7789_SLPOUT);
    HAL_Delay(120);

    /*
     * RGB565
     */
    ST7789_WriteCommand(ST7789_COLMOD);

    {
        uint8_t data = 0x55;
        ST7789_WriteData(&data, 1);
    }

    HAL_Delay(10);

    /*
     * Landscape 320 x 240
     *
     * MX | MV
     * 0x40 | 0x20 = 0x60
     */
    ST7789_WriteCommand(ST7789_MADCTL);

    {
        uint8_t data = 0x60;
        ST7789_WriteData(&data, 1);
    }

    /*
     * 320 columns
     */
    ST7789_WriteCommand(ST7789_CASET);

    {
        uint8_t data[] =
        {
            0x00, 0x00,
            0x01, 0x3F
        };

        ST7789_WriteData(data, 4);
    }

    /*
     * 240 rows
     */
    ST7789_WriteCommand(ST7789_RASET);

    {
        uint8_t data[] =
        {
            0x00, 0x00,
            0x00, 0xEF
        };

        ST7789_WriteData(data, 4);
    }

    /*
     * Inversion
     */
    ST7789_WriteCommand(ST7789_INVON);
    HAL_Delay(10);

    /*
     * Normal display
     */
    ST7789_WriteCommand(ST7789_NORON);
    HAL_Delay(10);

    /*
     * Display ON
     */
    ST7789_WriteCommand(ST7789_DISPON);
    HAL_Delay(10);
}


/*
 * Set drawing window.
 */
void ST7789_SetAddressWindow(uint16_t x0,
                             uint16_t y0,
                             uint16_t x1,
                             uint16_t y1)
{
    uint8_t data[4];

    /*
     * Column address
     */
    ST7789_WriteCommand(ST7789_CASET);

    data[0] = x0 >> 8;
    data[1] = x0 & 0xFF;
    data[2] = x1 >> 8;
    data[3] = x1 & 0xFF;

    ST7789_WriteData(data, 4);

    /*
     * Row address
     */
    ST7789_WriteCommand(ST7789_RASET);

    data[0] = y0 >> 8;
    data[1] = y0 & 0xFF;
    data[2] = y1 >> 8;
    data[3] = y1 & 0xFF;

    ST7789_WriteData(data, 4);

    /*
     * Start memory write
     */
    ST7789_WriteCommand(ST7789_RAMWR);
}


/*
 * Draw one pixel.
 */
void ST7789_DrawPixel(uint16_t x,
                      uint16_t y,
                      uint16_t color)
{
    uint8_t data[2];

    if (x >= ST7789_WIDTH ||
        y >= ST7789_HEIGHT)
    {
        return;
    }

    ST7789_SetAddressWindow(x, y, x, y);

    data[0] = color >> 8;
    data[1] = color & 0xFF;

    ST7789_WriteData(data, 2);
}


/*
 * Fill rectangle.
 */
void ST7789_FillRect(uint16_t x,
                     uint16_t y,
                     uint16_t w,
                     uint16_t h,
                     uint16_t color)
{
    uint32_t pixels;
    uint32_t i;

    if (x >= ST7789_WIDTH ||
        y >= ST7789_HEIGHT)
    {
        return;
    }

    if ((x + w) > ST7789_WIDTH)
    {
        w = ST7789_WIDTH - x;
    }

    if ((y + h) > ST7789_HEIGHT)
    {
        h = ST7789_HEIGHT - y;
    }

    if (w == 0 || h == 0)
    {
        return;
    }

    ST7789_SetAddressWindow(
        x,
        y,
        x + w - 1,
        y + h - 1
    );

    /*
     * Send pixels.
     *
     * This version sends one pixel at a time.
     * It is simple but not particularly fast.
     */
    CS_LOW();
    DC_DATA();

    for (i = 0; i < ((uint32_t)w * h); i++)
    {
        ST7789_WriteByte(color >> 8);
        ST7789_WriteByte(color & 0xFF);
    }

    CS_HIGH();
}


/*
 * Fill entire display.
 */
void ST7789_FillScreen(uint16_t color)
{
    ST7789_FillRect(
        0,
        0,
        ST7789_WIDTH,
        ST7789_HEIGHT,
        color
    );
}


/*
 * Convert 24-bit RGB to RGB565.
 */
uint16_t ST7789_Color565(uint8_t r,
                         uint8_t g,
                         uint8_t b)
{
    return ((r & 0xF8) << 8) |
           ((g & 0xFC) << 3) |
           ((b & 0xF8) >> 3);
}
