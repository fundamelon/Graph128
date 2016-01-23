/*	ichor001_labfinal_MCU1.c - 8 November 2015
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
#include <stdlib.h>

#include "usart_ATmega1284.h"


typedef struct task {
	int state;
	unsigned long period;
	unsigned long elapsedTime;
	int (*TickFct)(int);
} task;


task tasks[2];

const unsigned char task_count = 3;
const unsigned long tasksPeriodGCD = 10;
const unsigned long ADC_period = 20;
const unsigned long LOGIC_period = 10;
const unsigned long TX_period = 10;

volatile unsigned long timerCurrent;
volatile unsigned long timerPeriod;

void timerOn() {
	TCCR1B = 0x0B; // prescaler
	TIMSK1 = 0x02;
	OCR1A = 125; // ms per tick
	TCNT1 = 0;
	timerCurrent = 0;
	SREG |= 0x80;
}


void timerSet(int ms) {
	timerPeriod = ms;
}


volatile unsigned char timer_flag = 0;

ISR(TIMER1_COMPA_vect) {
	timerCurrent++;
	if(timerCurrent > timerPeriod) {
		timerCurrent = 0;
		timer_flag = 1;
	}
}

volatile unsigned char data = 0x00;


void ADC_mux(unsigned char pin) {
	ADMUX = pin;
	for(unsigned char i = 0; i < 128; i++) asm("nop");
}

enum ADC_states { ADC_init, ADC_sample };

volatile unsigned short joystick_x = 0;
volatile unsigned short joystick_y = 0;
volatile unsigned short deadpos_x = 0;
volatile unsigned short deadpos_y = 0;


int ADC_tick(int state) {
	
	switch(state) {
		case ADC_init:
			ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
			// delay for deadzone calibration
			for(unsigned short i = 0; i < 4096; i++) asm("nop");
		
			ADC_mux(0x01);
			deadpos_x = ADC;
			ADC_mux(0x00);
			deadpos_y = ADC;
		
			state = ADC_sample;
			break;
		case ADC_sample:
			state = ADC_sample;
			break;
		default:
			break;
	}

	switch(state) {
		case ADC_sample:
			ADC_mux(0x01);
			joystick_x = ADC;
			ADC_mux(0x00);
			joystick_y = ADC;
			break;
		default:
			break;
	}
	
	return state;
}


volatile uint8_t flag_send_joystick = 0;
volatile uint8_t flag_send_buttons = 0;

volatile uint8_t joystick_status;
volatile uint8_t up, down, left, right;
volatile uint8_t button_status;
enum LOGIC_states { LOGIC_init, LOGIC_update };
	
int LOGIC_tick(int state) {
	
	static unsigned char deadzoned_x = 0;
	static unsigned char deadzoned_y = 0;
	
	static unsigned char timer_x = 0;
	static unsigned char timer_y = 0;
	static unsigned int timer_deadzone = 0;
	
	const unsigned char deadzone = 20;
	
	switch(state) {
		case LOGIC_init:
			up = down = left = right = 0;
			state = LOGIC_update;
			break;
		case LOGIC_update:
			state = LOGIC_update;
			break;
		default:
			state = LOGIC_init;
			break;
	}

	switch(state) {
		case LOGIC_update:
			; 
			// feature extraction from joystick
			up = down = left = right = 0;
			short dx;
			short dy;
			dx = -((short)joystick_x - (short)deadpos_x);
			dy = ((short)joystick_y - (short)deadpos_y);
			
			// x-axis
			if(abs(dx) < 20) {
				// x-axis is deadzoned
				deadzoned_x = 1;
				if(abs(dy) < deadzone)
					timer_deadzone = 0;
				else
					timer_deadzone++;
			} else if(timer_deadzone < 200) {
				// time since moved out of deadzone before repeat
				timer_deadzone++;
				if(dx < 0) left = 1;
				if(dx > 0) right = 1;
			} else if(deadzoned_x) {
				// repeat based on magnitude (deadzoned_y is used as a toggling flag)
				deadzoned_x = 0;
				timer_x = 0;
				if(dx < 0) left = 1;
				if(dx > 0) right = 1;
			//	if(pos_x < min_x) pos_x = max_x;
			//	if(pos_x > max_x) pos_x = min_x;
			} else {
				// change timer length based on magnitude
				timer_x+=2;
				if(abs(dx) < 100) {
					if(timer_x >= (1000 / LOGIC_period)) deadzoned_x = 1;
				} else if(abs(dx) < 200) {
					if(timer_x >= (500 / LOGIC_period)) deadzoned_x = 1;
				} else if(abs(dx) < 340) {
					if(timer_x >= (250 / LOGIC_period)) deadzoned_x = 1;
				} else {
					if(timer_x >= (100 / LOGIC_period)) deadzoned_x = 1;
				}
			
			}
		
			// y-axis
			if(abs(dy) < 20) {
				// y-axis is deadzoned
				deadzoned_y = 1;
				if(abs(dx) < deadzone)
					timer_deadzone = 0; // both deadzoned
				else
					timer_deadzone++; // x-axis in use - keep incrementing timer
			} else if(timer_deadzone < 200) {
				// time since moved out of deadzone before repeat
				timer_deadzone++;
				if(dy < 0) down = 1;
				if(dy > 0) up = 1;
			} else if(deadzoned_y) {
				// repeat based on magnitude (deadzoned_y is used as a toggling flag)
				deadzoned_y = 0;
				timer_y = 0;
				if(dy < 0) down = 1;
				if(dy > 0) up = 1;
			//	if(pos_y < min_y) pos_y = max_y;
			//	if(pos_y > max_y) pos_y = min_y;
			} else {
				// change timer length based on magnitude
				timer_y+=2;
				if(abs(dy) < 100) {
					if(timer_y >= (1000 / LOGIC_period)) deadzoned_y = 1;
				} else if(abs(dy) < 200) {
					if(timer_y >= (500 / LOGIC_period)) deadzoned_y = 1;
				} else if(abs(dy) < 300) {
					if(timer_y >= (250 / LOGIC_period)) deadzoned_y = 1;
				} else {
					if(timer_y >= (100 / LOGIC_period)) deadzoned_y = 1;
				}
			}
			
			// limit to one axis at a time
			if(abs(dx) < abs(dy)) left = right = 0;
			if(abs(dx) > abs(dy)) up = down = 0;
			
			// write to status variable
			uint8_t old_status;
			old_status = joystick_status;
			joystick_status = (up << 0) | (down << 1) | (left << 2) | (right << 3);
			PORTD = joystick_status << 3;
			
			// raise flag to send only if status changed
			if(old_status != joystick_status)
				flag_send_joystick = 1;
				
			// same for button inputs
			old_status = button_status;
			
			if(~PINA & 0x80) 
				button_status |= 0x10;
			else
				button_status &= ~0x10;
				
			if(old_status != button_status)
				flag_send_buttons = 1;
		
			break;
		default:
			break;
	}
	
	return state;
}


enum TX_states { TX_init, TX_idle };
	
int TX_tick(int state) {
	
	switch(state) {
		case TX_init:
			initUSART(0);
			state = TX_idle;
			break;
		case TX_idle:
			if(flag_send_joystick && USART_IsSendReady(0)) {
				USART_Send(joystick_status | button_status, 0);
				flag_send_joystick = 0;
			}
			
			if(flag_send_buttons && USART_IsSendReady(0)) {
				USART_Send(button_status, 0);
				flag_send_buttons = 0;
			}
			state = TX_idle;
			break;
		default:
			state = TX_init;
			break;
	}

	switch(state) {
		case TX_idle:
			break;
		default:
			break;
	}
	
	return state;
}



int main(void) {
		
	// A as input for ADC and buttons
	DDRA = 0x00; PINA = 0xFF;
	
	// D as output for debug and USART
	DDRD = 0xFF; PIND = 0x00;

	unsigned char i = 0;
	tasks[i].state = ADC_init;
	tasks[i].period = ADC_period;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &ADC_tick;
	i++;	
	tasks[i].state = LOGIC_init;
	tasks[i].period = LOGIC_period;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &LOGIC_tick;
	i++;
	tasks[i].state = TX_init;
	tasks[i].period = TX_period;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &TX_tick;
	i++;

	timerSet(tasksPeriodGCD);
	timerOn();

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
