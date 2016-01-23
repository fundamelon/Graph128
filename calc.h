/*	calc.h - 8 November 2015
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


#ifndef CALC_H_
#define CALC_H_

void CALC_init();

void CALC_drawCurrentMenu();

void CALC_setupMainMenu();
void CALC_setupSettingsMenu();
void CALC_setupGraphCreationMenu();
void CALC_setupGraphDisplayMenu();
void CALC_setupGraphLoadMenu();

// event handlers for calculator cursor
void CALC_cursorUp();
void CALC_cursorDown();
void CALC_cursorLeft();
void CALC_cursorRight();
void CALC_cursorFire();

CALC_throwError(const char* msg);

#endif /* CALC_H_ */