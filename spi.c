/*	spi.c - 10 November 2015
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

#include "util.h"

void SPI_masterInit() {
	
	DDRB = 0xBF; PINB = 0xBF; PORTB = 0x40; // MISO input
	CLR_BYTE(SPCR);
	SPCR = (1 << SPE) | (1 << MSTR);// | (1 << SPR0);
//	SET_BIT(SPCR, SPE);
//	SET_BIT(SPCR, MSTR);
	
	SET_BIT(SPSR, SPI2X);
//	SPSR |= (1 << SPI2X);
//	SREG |=  (1 << 7);
	SET_BIT(SREG, 7);
}


void SPI_masterTransmit(unsigned char data) {
	
	SPDR = data;
	while(!GET_BIT(SPSR, SPIF));
}