/*	lcd_7735.h - 8 November 2015
 *	Name & E-mail:  - Igi Chorazewicz (ichor001@ucr.edu)
 *	CS Login: ichor001
 *	Partner(s) Name & E-mail:  - 
 *	Lab Section: 
 *	Assignment: Lab Final Project
 *	Exercise Description:
 *
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */


#ifndef LCD_ST7735_H_
#define LCD_ST7735_H_

// most of these data values can be found in the Adafruit ST7735 LCD library https://github.com/adafruit/Adafruit-ST7735-Library
#define LCD_SWRESET 0x01
#define LCD_SLPOUT 0x11
#define LCD_DISPOFF 0x28
#define LCD_DISPON 0x29
#define LCD_CASET 0x2A
#define LCD_RASET 0x2B
#define LCD_RAMWR 0x2C
#define LCD_MADCTL 0x36
#define LCD_COLMOD 0x3A
#define LCD_RGB565 0x05

#define LCD_INVOFF  0x20
#define LCD_INVON   0x21

#define LCD_XSIZE 128
#define LCD_YSIZE 160
#define LCD_SIZE 20480 // 128 x 160
#define LCD_XMAX LCD_XSIZE-1
#define LCD_YMAX LCD_YSIZE-1
#define LCD_BLACK 0x0000
#define LCD_BLUE 0x001F
#define LCD_RED 0xF800
#define LCD_GREEN 0x04C0
#define LCD_CYAN    0x07FF
#define LCD_MAGENTA 0xF81F
#define LCD_YELLOW  0xFFE0
#define LCD_WHITE 0xFFFF


#define LCD_COLOR565(r,g,b) ((((r) << 11) & LCD_RED) | (((g) << 5) & LCD_GREEN) | ((b) & LCD_BLUE))

void LCD_init();
void LCD_hardReset();
void LCD_writeCommand(uint8_t cmd);
void LCD_writeData(uint8_t data);
void LCD_writeColor565(uint16_t data, uint16_t count);

void LCD_clear();
void LCD_clearColor(uint16_t color);
void LCD_setAddrWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);

void LCD_drawRect(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint16_t color);
void LCD_fillRect(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint16_t color);
void LCD_drawRectOffset(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint16_t color, uint16_t o);
void LCD_fillRectOffset(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint16_t color, uint16_t o);
void LCD_drawPixel(uint8_t x, uint8_t y, uint16_t color);
void LCD_drawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint16_t color);
void LCD_drawFastLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint16_t color);


void LCD_setTextSpacing(uint8_t spacing);
void LCD_setTextHSpacing(uint8_t spacing);
void LCD_setTextVSpacing(uint8_t spacing);
uint8_t LCD_getTextHSpacing();
uint8_t LCD_getTextVSpacing();

void LCD_setTextPosition(int16_t x, int16_t y);

void LCD_setTextSize(uint8_t size);
uint8_t LCD_getTextSize();

void LCD_setForeground(uint16_t color);
uint16_t LCD_getForeground();

void LCD_setBackground(uint16_t color);
uint16_t LCD_getBackground();

void LCD_setTextWrap(uint8_t flag);
uint8_t LCD_getTextWrap();

void LCD_drawChar(int16_t x, int16_t y, unsigned char c, uint16_t fg, uint16_t bg, uint8_t size);
void LCD_drawString(const char *string);

uint16_t LCD_getStringPixelWidth(const char* string);
uint16_t LCD_getStringPixelHeight(const char* string);

void LCD_invert();

#endif /* LCD_7735_H_ */