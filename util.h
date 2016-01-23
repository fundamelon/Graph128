/*	util.h - 10 November 2015
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


#ifndef IO_H_
#define IO_H_

#define TRUE 1
#define FALSE 0
#define NULL 0

#define PI 3.1415926535

#define GET_BIT(a,b) ((a) & (1 << (b)))
#define SET_BIT(a,b) ((a) |= (1 << (b)))
#define CLR_BIT(a,b) ((a) &= ~(1 << (b)))

#define CLR_BYTE(a) ((a) = 0xFF)

#define HI(a) (a) >> 8
#define LO(a) (a) // & 0xFF

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) > (b) ? (b) : (a))
#define CLAMP(a,b,c) MAX(MIN(a, b), c)
#define SWAP(a, b) do { typeof(a) SWAP = a; a = b; b = SWAP; } while(0)
	
		
typedef struct {
	uint8_t x;
	uint8_t y;	
} vec2uc;

typedef struct {
	uint16_t x;
	uint16_t y;
} vec2i;

typedef struct {
	float x;
	float y;
} vec2f;


#endif /* IO_H_ */