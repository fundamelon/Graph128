/*	ichor001_labfinal_MCU0.c - 8 November 2015
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


#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include "usart_ATmega1284.h"

#include "util.h"
#include "spi.h"
#include "pinmap.h"
#include "lcd_st7735.h"
#include "calc.h"


typedef struct task {
	int state;
	unsigned long period;
	unsigned long elapsedTime;
	int (*TickFct)(int);
} task;

task tasks[3];

const unsigned char task_count = 3;
const unsigned long tasksPeriodGCD = 1;
const unsigned long DISPLAY_period = 1;
const unsigned long RX_period = 2;
const unsigned long LOGIC_period = 1;



/*
uint8_t buffer_1[LCD_SIZE] PROGMEM;
*/
// === TIMER ===

volatile unsigned long TIMER_current;
volatile unsigned long TIMER_period;

void TIMER_on() {
	TCCR1B = 0x0B; // prescaler
	TIMSK1 = 0x02;
	OCR1A = 125; // ms per tick
	TCNT1 = 0;
	TIMER_current = 0;
	SREG |= 0x80;
}


void TIMER_set(int ms) {
	TIMER_period = ms;
}


volatile unsigned char timer_flag = 0;

ISR(TIMER1_COMPA_vect) {
	
	TIMER_current++;
	if(TIMER_current > TIMER_period) {
		TIMER_current = 0;
		timer_flag = 1;
	}
}

// --- end TIMER ---

// === USART ===

enum RX_states { RX_init, RX_idle };
volatile unsigned char RX_data = 0x00;
volatile unsigned char flag_received = 0;

int RX_tick(int state) {
	
	switch(state) {
		case RX_init:
			initUSART(0);
			state = RX_idle;
			break;
		case RX_idle:
			if(USART_HasReceived(0)) {
				RX_data = USART_Receive(0);
				flag_received = 1;
			}
			state = RX_idle;
			break;
		default:
			state = RX_init;
			break;
	}

	switch(state) {
			case RX_idle:
			break;
		default:
			break;
	}
	
	return state;
}


// --- end USART ---

// === DISPLAY ===


enum DISPLAY_states { DISPLAY_init, DISPLAY_draw, DISPLAY_error, DISPLAY_idle, DISPLAY_update };

int DISPLAY_tick(int state) {
	static unsigned long tick = 0;
	switch(state) {
		case DISPLAY_init:
			// display initialization
			LCD_init();
			LCD_clear();
			LCD_clearColor(LCD_BLACK);
			LCD_setForeground(LCD_GREEN);
			LCD_setBackground(LCD_BLACK);
			CALC_init();
			CALC_setupMainMenu();
			state = DISPLAY_idle;
			break;
		case DISPLAY_error:
			LCD_clearColor(LCD_RED);
			LCD_setForeground(LCD_WHITE);
			LCD_setBackground(LCD_RED);
			LCD_drawString("ERROR");
			break;
		case DISPLAY_idle:
			CALC_update();
			;		
			break;
		case DISPLAY_update:
			break;
		default:
			state = DISPLAY_init;
			break;
	}

	switch(state) {
		case DISPLAY_idle:
			break;
		case DISPLAY_update:
			break;
		default:
			break;
	}
	
	tick++;
	return state;
}

// --- end DISPLAY ---

// === LOGIC ===


enum LOGIC_states { LOGIC_init, LOGIC_idle };

int LOGIC_tick(int state) {
	
	switch(state) {
		case LOGIC_init:
			state = LOGIC_idle;
			break;
		case LOGIC_idle:
			state = LOGIC_idle;
			break;
		default:
			state = LOGIC_init;
			break;
	}

	switch(state) {
		case LOGIC_idle:
			if(flag_received) {
				flag_received = 0;
				
				if(RX_data & 0x01)
					CALC_cursorDown();
				if(RX_data & 0x02)
					CALC_cursorUp();
				if(RX_data & 0x04)
					CALC_cursorRight();
				if(RX_data & 0x08)
					CALC_cursorLeft();
					
			//	char* str[10];
			//	itoa(RX_data, str, 16);
			//	LCD_setTextPosition(0, 0);
			//	LCD_drawString(str);
					
				if(RX_data & 0x10)
					CALC_cursorFire();
			}
			break;
		default:
			break;
	}
	
	return state;
}

// --- end LOGIC ---



int main(void)
{
	
	// port setup (refer to pinmap.h)
	LCD_DDR_CONTROL |= LCD_PORT_CONTROL_MASK;
	LCD_PORT_CONTROL &= ~LCD_PORT_CONTROL_MASK;
	
	LCD_DDR_SELECT |= LCD_PORT_SELECT_MASK;
	LCD_PORT_SELECT &= ~LCD_PORT_SELECT_MASK;
	
	SPI_masterInit();
	
	unsigned char i = 0;
	tasks[i].state = RX_init;
	tasks[i].period = DISPLAY_period;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &RX_tick;
	i++;
	tasks[i].state = LOGIC_init;
	tasks[i].period = LOGIC_period;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &LOGIC_tick;
	i++;
	tasks[i].state = DISPLAY_init;
	tasks[i].period = DISPLAY_period;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &DISPLAY_tick;
	i++;

	TIMER_set(tasksPeriodGCD);
	TIMER_on();

    while(1) {

		unsigned char i;
		for(i = 0; i < task_count; ++i) {
			if(tasks[i].elapsedTime >= tasks[i].period) {
				tasks[i].state = tasks[i].TickFct(tasks[i].state);
				tasks[i].elapsedTime = 0;
			}
			tasks[i].elapsedTime += tasksPeriodGCD;
		}

		while(!timer_flag);
		timer_flag = 0;
    }
}
