#include "stm32f0xx_hal.h"
#include "st7735.h"

// Scanning direction of frame memory,
// X-mirror and Y-mirror, set bits
// according to datasheet
#define MADCTL_MY  (1 << 6)
#define MADCTL_MX  (1 << 7)
// Select BGR color filter panel type
#define MADCTL_BGR (1 << 3)

// Code of 16-bit RGB 5-6-5 color format by datasheet
#define MODE_RGB_565 0b101

// Display hardware reset pin and port
#define RESET_Pin  GPIO_PIN_3
#define RESET_Port GPIOA
// Display Data/Command select pin and port
#define RS_Pin  GPIO_PIN_2
#define RS_Port GPIOA

// Srart cells in display memory
#define XSTART 26
#define YSTART 1
// Display resolution
#define WIDTH  80
#define HEIGHT 160

// Control commands codes by datasheet
#define CMD_SWRESET 0x01
#define CMD_SLPOUT  0x11
#define CMD_INVON   0x21
#define CMD_DISPON  0x29
#define CMD_CASET   0x2A
#define CMD_RASET   0x2B
#define CMD_RAMWR   0x2C
#define CMD_COLMOD  0x3A
#define CMD_MADCTL  0x36

static SPI_HandleTypeDef *hspi;
static lv_disp_drv_t *disp_drv;

// Send 1-byte command to display
static void ST7735_WriteCommand(uint8_t cmd)
{
    HAL_GPIO_WritePin(RS_Port, RS_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(hspi, &cmd, sizeof(cmd), HAL_MAX_DELAY);
}

// Send data to display (non-DMA)
static void ST7735_WriteData(uint8_t* buff, size_t buff_size)
{
    HAL_GPIO_WritePin(RS_Port, RS_Pin, GPIO_PIN_SET);
    HAL_SPI_Transmit(hspi, buff, buff_size, HAL_MAX_DELAY);
}

// Send 1 byte data to display
static void ST7735_WriteByteData(uint8_t byte)
{
  HAL_GPIO_WritePin(RS_Port, RS_Pin, GPIO_PIN_SET);
  HAL_SPI_Transmit(hspi, &byte, 1, HAL_MAX_DELAY);
}

// Set address window in display frame memory for given coordinates
static void ST7735_SetAddressWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    // Set column address
    uint8_t data[] = { 0x00, (uint8_t)(x0 + XSTART), 0x00, (uint8_t)(x1 + XSTART) };
    ST7735_WriteCommand(CMD_CASET);
    ST7735_WriteData(data, sizeof(data));

    // Set row address
    data[1] = y0 + YSTART;
    data[3] = y1 + YSTART;
    ST7735_WriteCommand(CMD_RASET);
    ST7735_WriteData(data, sizeof(data));

    // Enable write to RAM mode
    ST7735_WriteCommand(CMD_RAMWR);
}

// ST7735 display initialization sequence
void ST7735_Init(SPI_HandleTypeDef *p_hspi, lv_disp_drv_t *p_disp_drv)
{
    hspi = p_hspi;
    disp_drv = p_disp_drv;
    // Hardware reset display
    HAL_GPIO_WritePin(RESET_Port, RESET_Pin, GPIO_PIN_RESET);
    HAL_Delay(5);
    HAL_GPIO_WritePin(RESET_Port, RESET_Pin, GPIO_PIN_SET);
    // Software reset
    ST7735_WriteCommand(CMD_SWRESET);
    // Wait while display module loads all default values
    // to registers, 120 ms by datasheet
    HAL_Delay(120);
    // Turn off sleep mode
    ST7735_WriteCommand(CMD_SLPOUT);
    // Set scanning direction of frame memory
    // and color filter panel type
    ST7735_WriteCommand(CMD_MADCTL);
    ST7735_WriteByteData(MADCTL_MX | MADCTL_MY | MADCTL_BGR);
    // Set 16-bit 5-6-5 RGB format
    ST7735_WriteCommand(CMD_COLMOD);
    ST7735_WriteByteData(MODE_RGB_565);
    // Enable display color inversion mode,
    // (normal black background and white text)
    ST7735_WriteCommand(CMD_INVON);
    // Turn display ON
    ST7735_WriteCommand(CMD_DISPON);
}

// Flush lvgl buffer to display memory by DMA transfer
void ST7735_flush_cb(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    int area_size = (area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1);

    ST7735_SetAddressWindow(area->x1, area->y1, area->x2, area->y2);
    HAL_GPIO_WritePin(RS_Port, RS_Pin, GPIO_PIN_SET);
    HAL_SPI_Transmit_DMA(hspi, (uint8_t*)color_p, area_size * sizeof(uint16_t));
}

// DMA SPI transfer of lvgl buffer data to ST7735 dispay complete,
// inform lvgl about this
void HAL_SPI_TxCpltCallback (SPI_HandleTypeDef *hspi)
{
    lv_disp_flush_ready(disp_drv);
}

