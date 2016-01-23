/*	lcd_7735.c - 8 November 2015
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

#include <stdint.h>

#include "lcd_st7735.h"
#include "util.h"
#include "spi.h"
#include "pinmap.h"
#include "font5x7.h"


void delay_ms(int miliSec) //for 8 Mhz crystal
{
	int i,j;
	for(i=0;i<miliSec;i++)
	for(j=0;j<775;j++)
	{
		asm("nop");
	}
}


void LCD_init() {
	LCD_hardReset();
	LCD_writeCommand(LCD_SLPOUT);
	delay_ms(150);
	LCD_writeCommand(LCD_COLMOD);
	LCD_writeData(LCD_RGB565);
	LCD_writeCommand(LCD_DISPON);
}


void LCD_hardReset() {
	CLR_BIT(LCD_PORT_CONTROL, LCD_PIN_RST);
	delay_ms(1);
	SET_BIT(LCD_PORT_CONTROL, LCD_PIN_RST);
	delay_ms(200);
}


void LCD_writeCommand(unsigned char cmd) {
	CLR_BIT(LCD_PORT_CONTROL, LCD_PIN_DC); // 0 -> command
	LCD_writeData(cmd);
	SET_BIT(LCD_PORT_CONTROL, LCD_PIN_DC); // 1 -> data
}


void LCD_writeData(unsigned char data) {
	CLR_BIT(LCD_PORT_SELECT, LCD_PIN_SS);
	SPI_masterTransmit(data);
	SET_BIT(LCD_PORT_SELECT, LCD_PIN_SS);
}


void LCD_writeDataRepeat(unsigned int data, unsigned int count) {
	CLR_BIT(LCD_PORT_SELECT, LCD_PIN_SS);
	unsigned char hi = HI(data);
	unsigned char lo = LO(data);
	for(unsigned int i = 0; i < count; i++) {
		SPI_masterTransmit(hi);
		SPI_masterTransmit(lo);
	}
	SET_BIT(LCD_PORT_SELECT, LCD_PIN_SS);
}


void LCD_setAddrWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
	LCD_writeCommand(LCD_CASET);
	LCD_writeData(HI(x0));
	LCD_writeData(LO(x0));
	LCD_writeData(HI(x1));
	LCD_writeData(LO(x1));
	LCD_writeCommand(LCD_RASET);
	LCD_writeData(HI(y0));
	LCD_writeData(LO(y0));
	LCD_writeData(HI(y1));
	LCD_writeData(LO(y1));
}


void LCD_clear() {
	LCD_clearColor(0x00);
}

void LCD_clearColor(uint16_t color) {
	LCD_setAddrWindow(0, 0, LCD_XMAX, LCD_YMAX);
	LCD_writeColor565(color, LCD_XMAX * LCD_YMAX);
};


void LCD_writeColor565(uint16_t data, uint16_t count) {
	LCD_writeCommand(LCD_RAMWR);
	unsigned char hi = HI(data);
	unsigned char lo = LO(data);
	CLR_BIT(LCD_PORT_SELECT, LCD_PIN_SS);
	while(count > 0) {
		// SPI transmit code is inline for speed
		//	SPI_masterTransmit(hi);
		SPDR = hi;
		while(!GET_BIT(SPSR, SPIF));
		SPDR = lo;
		while(!GET_BIT(SPSR, SPIF));
		//	SPI_masterTransmit(lo);
		count--;
	}
	SET_BIT(LCD_PORT_SELECT, LCD_PIN_SS);
}


void LCD_drawRect(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint16_t color) {
	if(x0 > x1) SWAP(x0, x1);
	if(y0 > y1) SWAP(y0, y1);
	uint8_t width = x1 - x0 + 1;
	uint8_t height = y1 - y0 + 1;
	LCD_setAddrWindow(x0, y0, x1, y0);
	LCD_writeColor565(color, width);
	LCD_setAddrWindow(x0, y1, x1, y1);
	LCD_writeColor565(color, width);
	LCD_setAddrWindow(x0, y0, x0, y1);
	LCD_writeColor565(color, height);
	LCD_setAddrWindow(x1, y0, x1, y1);
	LCD_writeColor565(color, height);
}


void LCD_fillRect(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint16_t color) {
	if(x0 > x1) SWAP(x0, x1);
	if(y0 > y1) SWAP(y0, y1);
	uint8_t width = x1 - x0 + 1;
	uint8_t height = y1 - y0 + 1;
	LCD_setAddrWindow(x0, y0, x1, y1);
	LCD_writeColor565(color, width*height);
}


// draw rectangles enlarged by o value
void LCD_drawRectOffset(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint16_t color, uint16_t o) {
	LCD_drawRect(x0 - o, y0 - o, x1 + o, y1 + o, color);
}


void LCD_fillRectOffset(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint16_t color, uint16_t o) {
	LCD_fillRect(x0 - o, y0 - o, x1 + o, y1 + o, color);
}


void LCD_drawPixel(uint8_t x, uint8_t y, uint16_t color) {
	LCD_setAddrWindow(x, y, x, y);
	LCD_writeColor565(color, 1);
}



// helper function to draw a line (screen coordinates)
// slightly optimized with the help of code found at http://www.simppa.fi/blog/extremely-fast-line-algorithm-as3-optimized/
void LCD_drawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint16_t color) {

	//	if(debug) std::cout << "\drawing line segment from <" << x0 << ", " << y0 << "> to <" << x1 << ", " << y1 << ">" << std::endl;

	int8_t y_longer = 0;
	int16_t length_short = y1 - y0;
	int16_t length_long = x1 - x0;
	
	int16_t x = x0;
	int16_t y = y0;
	
	if(abs(length_short) > abs(length_long)) {
		int16_t temp = length_short;
		length_short = length_long;
		length_long = temp;
		y_longer = 1;
	}

	int16_t d;
	if(length_long == 0) 
		d = 0;
	else
		d = (length_short << 8) / length_long;

	if (y_longer) {
		if (length_long > 0) {
			length_long += y;
			for (int16_t j = 0x80 + (x << 8); y <= length_long; y++) {
				LCD_drawPixel(j >> 8, y, color);//(int)y * XMAX + x_i, current_color, 0.5, data);
				j += d;
			}
		} else {
			length_long += y;
			for (uint16_t j = 0x80 + (x << 8); y >= length_long; y--) {
				LCD_drawPixel(j >> 8, y, color);//(int)y * width + x_i, current_color, 0.5, data);
				j -= d;
			}
		}
	} else {
		if (length_long > 0) {
			length_long += x;
			for (int16_t j = 0x80 + (y << 8); x <= length_long; x++) {
				LCD_drawPixel(x, j >> 8, color);//(int)y * XMAX + x_i, current_color, 0.5, data);
				j += d;
			}
		} else {
			length_long += y;
			for (uint16_t j = 0x80 + (y << 8); x >= length_long; x--) {
				LCD_drawPixel(x, j >> 8, color);//(int)y * width + x_i, current_color, 0.5, data);
				j -= d;
			}
		}
	}
}

// DO NOT USE, needs optimization
// helper function to draw an optimized line (gets slower as line slope approaches 1; fastest when flat; decreases SPI time)
void LCD_drawFastLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint16_t color) {

	//	if(debug) std::cout << "\drawing line segment from <" << x0 << ", " << y0 << "> to <" << x1 << ", " << y1 << ">" << std::endl;

	int8_t y_longer = 0;
	int16_t length_short = y1 - y0;
	int16_t length_long = x1 - x0;
	
	int16_t x = x0;
	int16_t y = y0;
	
	if(abs(length_short) > abs(length_long)) {
		int16_t temp = length_short;
		length_short = length_long;
		length_long = temp;
		y_longer = 1;
	}

	int16_t d;
	if(length_long == 0) 
		d = 0;
	else
		d = (length_short << 8) / length_long;
		
	uint8_t flat_start = 0;
	uint8_t flat_count = 0;

	if (y_longer) {
		flat_start = y;
		if (length_long > 0) {
			length_long += y;
			for (int16_t j = 0x80 + (x << 8); y <= length_long; y++) {
				//LCD_drawPixel(j >> 8, y, color);//(int)y * XMAX + x_i, current_color, 0.5, data);
				//j += d;
				uint8_t j_shifted = j >> 8;
				uint8_t j_dped = j += d;
				if(j_dped >> 8 != j) {
					LCD_setAddrWindow(j_shifted, flat_start, j_shifted, y);
					LCD_writeColor565(color, flat_count);
					flat_count = 0;
					flat_start = y;
				}
				
				flat_count++;
				j = j_dped;
			}
		} else {
			length_long += y;
			for (uint16_t j = 0x80 + (x << 8); y >= length_long; y--) {
				//LCD_drawPixel(j >> 8, y, color);//(int)y * width + x_i, current_color, 0.5, data);
				//j -= d;
				if((j - d) >> 8 != j) {
					LCD_setAddrWindow(j >> 8, y, j >> 8, flat_start);
					LCD_writeColor565(color, flat_count);
					flat_count = 0;
					flat_start = y;
				}
				
				flat_count++;
				j -= d;
			}
		}
	} else {
		flat_start = x;
		if (length_long > 0) {
			length_long += x;
			for (int16_t j = 0x80 + (y << 8); x <= length_long; x++) {
				//LCD_drawPixel(x, j >> 8, color);//(int)y * XMAX + x_i, current_color, 0.5, data);
				//j += d;
				if((j + d) >> 8 != j) {
					LCD_setAddrWindow(flat_start, j >> 8, x, j >> 8);
					LCD_writeColor565(color, flat_count);
					flat_count = 0;
					flat_start = x;
				}
				
				flat_count++;
				j += d;
			}
		} else {
			length_long += y;
			for (uint16_t j = 0x80 + (y << 8); x >= length_long; x--) {
				//LCD_drawPixel(x, j >> 8, color);//(int)y * width + x_i, current_color, 0.5, data);
				//j -= d;
				if((j - d) >> 8 != j) {
					LCD_setAddrWindow(x, j >> 8, flat_start, j >> 8);
					LCD_writeColor565(color, flat_count);
					flat_count = 0;
					flat_start = x;
				}
				
				flat_count++;
				j -= d;
			}
		}
	}
}


// --- TEXT FUNCTIONS ---

// set and get text spacing in pixels
static uint8_t text_h_spacing = 0;
static uint8_t text_v_spacing = 0;
void LCD_setTextSpacing(uint8_t spacing) { text_h_spacing = spacing; text_v_spacing = spacing; }
void LCD_setTextHSpacing(uint8_t spacing) { text_h_spacing = spacing; }
void LCD_setTextVSpacing(uint8_t spacing) { text_v_spacing = spacing; }
uint8_t LCD_getTextHSpacing() { return text_h_spacing; }
uint8_t LCD_getTextVSpacing() { return text_v_spacing; }
	
// set text start position in pixel coordinates
static vec2i text_pos;
void LCD_setTextPosition(int16_t x, int16_t y) { text_pos.x = x, text_pos.y = y; }

// set and get text size (width of each text pixel)
static uint8_t text_size;
void LCD_setTextSize(uint8_t size) { text_size = size; }
uint8_t LCD_getTextSize() { return text_size; }

// set and get text foreground color
static uint16_t text_fg;
void LCD_setForeground(uint16_t color) { text_fg = color; }
uint16_t LCD_getForeground() { return text_fg; }

// set and get text background color
static uint16_t text_bg;
void LCD_setBackground(uint16_t color) { text_bg = color; }
uint16_t LCD_getBackground() { return text_bg; }

// set and get text wrapping flag
static uint8_t text_wrap;
void LCD_setTextWrap(uint8_t flag) { text_wrap = flag; }
uint8_t LCD_getTextWrap() { return text_wrap; }


// draw a single character
void LCD_drawChar(int16_t x, int16_t y, unsigned char c, uint16_t fg, uint16_t bg, uint8_t size) {
	for(uint8_t row = 0; row < 5; row++) {
		uint8_t char_byte = pgm_read_byte(font + (c * 5) + row);
		for(uint8_t col = 0; col < 8; col++) {
			uint16_t color = (char_byte & (1 << col)) ? fg : bg;
			if(text_size == 1) {
				LCD_drawPixel(x + row, y + col, color);
			} else {
				LCD_fillRect(x + row * size, y + col * size, x + row * size + size - 1, y + col * size + size - 1, color);
			}
		}
	}
}


// draw a string using text globals
void LCD_drawString(const char *string) {
	unsigned int length = strlen(string);
	
	vec2i char_pos = text_pos;
	
	for(unsigned int i = 0; i < length; i++) {
		// text wrap
		if((char_pos.x + text_size * 5) >= LCD_XMAX) {
			if(!text_wrap) return; // end string draw if no wrap
			char_pos.x = text_pos.x;
			char_pos.y += text_size * 7 + text_v_spacing;
		}
		
		if(string[i] == '\n') {
			char_pos.x = text_pos.x;
			char_pos.y += text_size * 7 + text_v_spacing;
			continue;
		}
		
		char_pos.x += text_h_spacing; // horiz. text spacing in pixels
		LCD_drawChar(char_pos.x, char_pos.y + text_v_spacing, string[i], text_fg, text_bg, text_size);
		char_pos.x += text_size * 5;
	}
	
	text_pos = char_pos;
}


uint16_t LCD_getStringPixelWidth(const char* string) {
	return (text_h_spacing + text_size * 5) * strlen(string);
}


uint16_t LCD_getStringPixelHeight(const char* string) {
	uint16_t pixel_height = text_size * 7 + text_v_spacing;
	
	if(text_wrap)
		return pixel_height;
	
	unsigned int length = strlen(string);
	unsigned int pixel_width = LCD_getStringPixelWidth(string);
	
	unsigned char wrap_count = pixel_width / LCD_XMAX + 2;
	
	return pixel_height * wrap_count;
}

static uint8_t inverted = 0;
void LCD_invert() {
	inverted = !inverted;
	LCD_writeCommand(inverted ? LCD_INVON : LCD_INVOFF);
}