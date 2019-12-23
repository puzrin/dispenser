#ifndef __ST7735_H__
#define __ST7735_H__

#include "lvgl.h"

void ST7735_Init(SPI_HandleTypeDef *p_hspi, lv_disp_drv_t *p_disp_drv);
void ST7735_flush_cb(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p);

#endif // __ST7735_H__
