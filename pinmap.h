/*	pinmap.h - 10 November 2015
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


#ifndef PINMAP_H_
#define PINMAP_H_

#include <avr/io.h>

// PORTA: peripheral control pins
// LCD
#define LCD_PORT_CONTROL PORTA
#define LCD_DDR_CONTROL	 DDRA
#define LCD_PIN_RST	0  // PORTA[0] : ST7735[2] (TFT LCD reset)
#define LCD_PIN_DC	1  // PORTA[0] : ST7735[3] (TFT LCD data/command)

#define LCD_PORT_CONTROL_MASK (1 << LCD_PIN_RST) | (1 << LCD_PIN_DC)

// PORTC: slave select pins
#define LCD_PORT_SELECT PORTC
#define LCD_DDR_SELECT DDRC
#define LCD_PIN_SS 0

#define LCD_PORT_SELECT_MASK (1 << LCD_PIN_SS) // PORTC[0] : ST7735[5] (TFT LCD chip select)


#endif /* PINMAP_H_ */