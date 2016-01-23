/*	calc.c - 8 November 2015
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
//#include <avr/eeprom.h>
#include "util.h"
#include "lcd_st7735.h"

#include "calc.h"

// use this to give the menu item default values
#define MENU_ITEM_DEFAULT .text_size = 1, .updated = 1, .active = 1, .up = NULL, .down = NULL, .left = NULL, .right = NULL
#define MENU_ITEM_MAX 32
enum MENU_TYPES { MENU_TYPE_MAINMENU, MENU_TYPE_SETTINGS, MENU_TYPE_CREATEGRAPH, MENU_TYPE_DISPLAYGRAPH };
	
static int menu_type;

typedef struct menu_item menu_item;
typedef struct menu_item {
	// basic item properties
	char * text;
	vec2i position;
	uint16_t text_color;
	uint16_t outline_color;
	uint8_t text_size;
	
	// pointers to neighboring menu items that can be accessed by the cursor
	menu_item *up, *down, *left, *right;
	
	// function that runs when menu item is selected
	int (*callback)(); 
	
	// this flag will be set to 0 when memory is cleared, indicating that the item is discarded.
	uint8_t active; 
	
	// flag that indicates if an item was updated and needs a re-draw
	uint8_t updated;
};

static menu_item items[MENU_ITEM_MAX];

// cursor focus effect types
#define CURSOR_TYPE_ARROW		0x01
#define CURSOR_TYPE_OUTLINE		0x02
#define CURSOR_TYPE_UNDERLINE	0x04
#define CURSOR_TYPE_HIGHLIGHT	0x08

enum CURSOR_dirs { CURSOR_UP, CURSOR_DOWN, CURSOR_LEFT, CURSOR_RIGHT, CURSOR_TYPE_CROSSHAIR, CURSOR_NONE };
static struct cursor {
	int16_t x, y;  // arbitrary cursor coordinates
	menu_item *focus; // pointer to focused item
	menu_item *prev; // pointer to previously focused item, used to erase cursor
	uint8_t dir;   // request cursor movement direction
	uint8_t fire;  // request fire event on focused item
	uint8_t type;  // combination of effects to show focus
	uint16_t color;
} cursor;


// cursor event handlers
void CALC_cursorUp() { cursor.dir = CURSOR_UP; }
void CALC_cursorDown() { cursor.dir = CURSOR_DOWN; }
void CALC_cursorLeft() { cursor.dir = CURSOR_LEFT; }
void CALC_cursorRight() { cursor.dir = CURSOR_RIGHT; }
void CALC_cursorFire() { cursor.fire = TRUE; }
	

// 8 bit int stack
typedef struct {
	int8_t* elements;
	int16_t top;
	int16_t max;
} stack_c;

// stack functions
void stackInit_c(stack_c *s, int max_size) {
	int16_t *contents;
	contents = (int16_t*)malloc(sizeof(int16_t) * max_size);
	if(contents == NULL) CALC_throwError("Stack creation: Not enough memory to create stack.");
	s->elements = contents;
	s->max = max_size;
	s->top = -1;
}

void stackDeinit_c(stack_c *s) {
	free(s->elements);
	s->elements = NULL;
	s->max = 0;
	s->top = -1;
}

uint8_t stackEmpty_c(stack_c *s) { return s->top < 0; }
uint8_t stackFull_c(stack_c *s) { return s->top >= s->max - 1; }
void stackPush_c(stack_c *s, int8_t elem) {
	if(stackFull_c(s)) CALC_throwError("Stack push: Stack is full.");
	s->elements[++s->top] = elem;
}
int8_t stackPop_c(stack_c *s) { return s->elements[s->top--]; }
	
	
// 32 bit float stack
typedef struct {
	float* elements;
	int16_t top;
	int16_t max;
} stack_f;

// stack functions
void stackInit_f(stack_f *s, int max_size) {
	int16_t *contents;
	contents = (int16_t*)malloc(sizeof(int16_t) * max_size);
	if(contents == NULL) CALC_throwError("Stack creation: Not enough memory to create stack.");
	s->elements = contents;
	s->max = max_size;
	s->top = -1;
}

void stackDeinit_f(stack_f *s) {
	free(s->elements);
	s->elements = NULL;
	s->max = 0;
	s->top = -1;
}

uint8_t stackEmpty_f(stack_f *s) { return s->top < 0; }
uint8_t stackFull_f(stack_f *s) { return s->top >= s->max - 1; }
void stackPush_f(stack_f *s, float elem) { 
	if(stackFull_f(s)) CALC_throwError("Stack push: Stack is full.");
	s->elements[++s->top] = elem; 
}
float stackPop_f(stack_f *s) { return s->elements[s->top--]; }
	
	
stack_c operator_stack;
stack_f RPN_value_stack;

uint8_t CALC_colorscheme;

// calc initialization function, ONLY CALL ONCE
void CALC_init() {
	CALC_colorscheme = 1;
	
	LCD_setForeground(CALC_colorscheme ? LCD_GREEN : LCD_BLACK);
	LCD_setBackground(CALC_colorscheme ? LCD_BLACK : LCD_WHITE);
	cursor.color = CALC_colorscheme ? LCD_WHITE : LCD_BLACK;
	
	cursor.x = LCD_XMAX/2;
	cursor.y = LCD_YMAX/2;
	cursor.focus = NULL;
	cursor.prev = NULL;
	cursor.dir = CURSOR_NONE;
	cursor.fire = 0;
	
	
	// initialize shunting-yard operator stack
	stackInit_c(&operator_stack, 32);
	stackInit_f(&RPN_value_stack, 32);
	
	CALC_clearMenuItems();
	// allocate memory for menu item array as zero
	//items = calloc(sizeof(menu_item), MENU_ITEM_MAX);
}


// calculate calculator state based on cursor flags, resets flags
void CALC_update() {
	
	// fire event on focused item
	if(cursor.fire) {
		cursor.fire = FALSE;
		if(cursor.focus != NULL)
			cursor.focus->callback();
	}
	
	// cursor movement
	switch(cursor.dir) {
		case CURSOR_UP:
			if(cursor.focus->up != NULL) {
				cursor.focus->updated = 1;
				cursor.prev = cursor.focus;
				cursor.focus = cursor.focus->up;
				cursor.focus->updated = 1;
			}
			break;
		case CURSOR_DOWN:
			if(cursor.focus->down != NULL) {
				cursor.focus->updated = 1;
				cursor.prev = cursor.focus;
				cursor.focus = cursor.focus->down;
				cursor.focus->updated = 1;
			}
			break;
		case CURSOR_LEFT:
			if(cursor.focus->left != NULL) {
				cursor.focus->updated = 1;
				cursor.prev = cursor.focus;
				cursor.focus = cursor.focus->left;
				cursor.focus->updated = 1;
			}
			break;
		case CURSOR_RIGHT:
			if(cursor.focus->right != NULL) {
				cursor.focus->updated = 1;
				cursor.prev = cursor.focus;
				cursor.focus = cursor.focus->right;
				cursor.focus->updated = 1;
			}
			break;
		case CURSOR_NONE:
			break;
	}
	
	if(cursor.focus->active) {
		cursor.x = cursor.focus->position.x;
		cursor.y = cursor.focus->position.y;
	}
	
	cursor.dir = CURSOR_NONE;
	
	CALC_drawCurrentMenu();
	
	// update all menu items
	//for(unsigned int i = 0; i < MENU_ITEM_MAX; i++)
	//	if(!items[i].trash)
			
	//}
}


#define FUNCTION_MAX_SIZE 64
static char current_function[FUNCTION_MAX_SIZE];
static uint8_t current_function_updated = 0;
static uint8_t current_function_index = 0;

enum OUTPUT_TYPES { OUTPUT_TYPE_TOKEN, OUTPUT_TYPE_VALUE };

typedef struct {
	int8_t token;
	float value;
	int8_t type;
	int8_t precedence;
} RPN_item;

static RPN_item current_function_RPN[FUNCTION_MAX_SIZE];
static uint8_t RPN_write_index = 0;

enum OPERATOR_TYPES {
	OPERATOR_NULL,
	OPERATOR_X,
	OPERATOR_LPTH,
	OPERATOR_RPTH,
	OPERATOR_POW,
	OPERATOR_DIV,
	OPERATOR_MUL,
	OPERATOR_SUB,
	OPERATOR_ADD,
	OPERATOR_SIN,
	OPERATOR_COS,
	OPERATOR_TAN,
	OPERATOR_LOG,
	OPERATOR_LN
};


void CALC_appendString(const char* str) {
	uint8_t str_length = strlen(str);
	for(uint8_t i = 0; i < str_length; i++) {
		if(current_function_index < FUNCTION_MAX_SIZE) {
			current_function[current_function_index] = str[i];
			current_function_index++;
		} else break;
	}
	current_function_updated = 1;
}

void CALC_appendChar(char c) {
	if(!(current_function_index < FUNCTION_MAX_SIZE)) return;
	current_function[current_function_index] = c;
	current_function_index++;
}

void CALC_popChar() {
	if(current_function_index == 0) return;
	current_function_index--;
	current_function[current_function_index] = '\0';
	current_function_updated = 1;
}

void CALC_clearCurrentFunction() {
	for(uint8_t i = 0; i < FUNCTION_MAX_SIZE; i++)
		current_function[i] = 0;
	current_function_updated = 1;
	current_function_index = 0;
	stackDeinit_c(&operator_stack);
	stackInit_c(&operator_stack, 32);
	stackDeinit_f(&RPN_value_stack);
	stackInit_f(&RPN_value_stack, 32);
}

void CALC_printRPNItem(RPN_item item) {
	uint8_t type = item.type;
	if(type == OUTPUT_TYPE_VALUE) {
		char str[32];
		//sprintf(str, "%f", item.value);
		itoa((int)(item.value + 0.5), str, 10);
		LCD_drawString(str);
	}
	
	if(type == OUTPUT_TYPE_TOKEN) {
		switch(item.token) {
			case OPERATOR_X:
				LCD_drawString("x");
				break;
			case OPERATOR_MUL:
				LCD_drawString("*");
				break;
			case OPERATOR_DIV:
				LCD_drawString("/");
				break;
			case OPERATOR_SUB:
				LCD_drawString("-");
				break;
			case OPERATOR_ADD:
				LCD_drawString("+");
				break;
			case OPERATOR_SIN:
				LCD_drawString("sin");
				break;
			case OPERATOR_COS:
				LCD_drawString("cos");
				break;
			case OPERATOR_TAN:
				LCD_drawString("tan");
				break;
			case OPERATOR_LOG:
				LCD_drawString("log");
				break;
			case OPERATOR_LN:
				LCD_drawString("ln");
				break;
			case OPERATOR_POW:
				LCD_drawString("^");
				break;
			default:
				break;
		}
	}
}

void CALC_addToOutputRPN(RPN_item o) {
	current_function_RPN[RPN_write_index] = o;
//	CALC_printRPNItem(o);
	RPN_write_index++;
}

uint8_t isNumber(char c) { return c >= '0' && c <= '9'; }
	
// returns operator precedence
uint8_t precedence(char o) {
	if(o == '(' || o == ')') return 5;
	if(o == '^') return 4;
	if(o == '*' || o == '/') return 3;
	if(o == '+' || o == '-') return 2;
	return 0;
}

uint8_t getOperatorType(char o) {
	uint8_t type;
	switch(o) {
		case '^':
			type = OPERATOR_POW;
			break;
		case '/':
			type = OPERATOR_DIV;
			break;
		case '*':
			type = OPERATOR_MUL;
			break;
		case '-':
			type = OPERATOR_SUB;
			break;
		case '+':
			type = OPERATOR_ADD;
			break;
		case 's':
			type = OPERATOR_SIN;
			break;
		case 'c':
			type = OPERATOR_COS;
			break;
		case 't': //
			type = OPERATOR_TAN;
			break;
		case 'l': // log
			type = OPERATOR_LOG;
			break;
		case 'n': // ln
			type = OPERATOR_LN;
			break;
		default:
			break;
	}
	return type;
}

void CALC_convertToRPN(char* str) {
	int16_t length = strlen(str);
	char num_string[32];
	memset(num_string, 0, 32);
	uint8_t num_string_index = 0;
	for(uint8_t i = 0; i < length; i++) {
		// number char
		if(isNumber(str[i])) {
			// if there is a number in front
			if(i + 1 != length && isNumber(str[i+1])) {
				num_string[num_string_index] = str[i];
				num_string_index++;
			} else if(num_string_index != 0) { // single digit but numbers previously read
				float val = (float)atoi(num_string);
			//	LCD_drawString(num_string);
				CALC_addToOutputRPN((RPN_item){.value = val, .type = OUTPUT_TYPE_VALUE}); // add number to output
				memset(num_string, 0, 32);
				num_string_index = 0;
			} else { // single digit
				float val = (float)(str[i]-'0');
				CALC_addToOutputRPN((RPN_item){.value = val, .type = OUTPUT_TYPE_VALUE}); // add number to output
			}
		} else {
			// variable x
			if(str[i] == 'x') {
				//LCD_drawString("x");
				CALC_addToOutputRPN((RPN_item){.token = OPERATOR_X, .type = OUTPUT_TYPE_TOKEN});
				continue;
			}
			
			//LCD_drawString("o");
			// function 
			if(str[i] == 's' || str[i] == 'c' || str[i] == 't' || str[i] == 'l') {
				
				if(str[i + 1] == 'n') { // ln case
					stackPush_c(&operator_stack, 'n');
					i += 1; 
				} else {
					stackPush_c(&operator_stack, str[i]);
					i += 2;
				} // skip forward 2 letters
				continue;
			}
			
			// operator
			if(str[i] == '^' || str[i] == '/' || str[i] == '*' || str[i] == '-' || str[i] == '+') {
				
				char o1 = str[i];
				while(!stackEmpty_c(&operator_stack)) {
					char o2 = stackPop_c(&operator_stack);
					if(o2 != '(' && ((o1 != '^' && precedence(o1) <= precedence(o2)) || (o1 == '^' && precedence(o1) < precedence(o2)))) {
						CALC_addToOutputRPN((RPN_item){.token = getOperatorType(o2), .type = OUTPUT_TYPE_TOKEN});
						continue;
					} 
						
					stackPush_c(&operator_stack, o2);
					break;
				}
						
				stackPush_c(&operator_stack, o1);
				continue;
			}
			
			// left parenthesis
			if(str[i] == '(') {
				stackPush_c(&operator_stack, '(');
			}
			
			// right parenthesis
			if(str[i] == ')') {
				char o;
				// pop operators into output until left parenthesis is found
				do {
					if(stackEmpty_c(&operator_stack)) { CALC_throwError("Mismatched parentheses"); return; }
					o = stackPop_c(&operator_stack);
					if(o != '(')
						CALC_addToOutputRPN((RPN_item){.token = getOperatorType(o), .type = OUTPUT_TYPE_TOKEN});
				} while(o != '(');
				
				// discards left parenthesis
				
				// if function is on top of stack, pop into output queue
				if(!stackEmpty_c(&operator_stack)) {
					o = stackPop_c(&operator_stack);
					if(o == 's' || o == 'c' || o == 't' || o == 'l' || o == 'n') {
						CALC_addToOutputRPN((RPN_item){.token = getOperatorType(o), .type = OUTPUT_TYPE_TOKEN});
					} else stackPush_c(&operator_stack, o);
				}
			}
		}
	}
	
	// leftover operators
	while(!stackEmpty_c(&operator_stack)) {
		char o = stackPop_c(&operator_stack);
		if(o == '(' || o == ')')  { CALC_throwError("Invalid parenthesis"); return; }
		CALC_addToOutputRPN((RPN_item){.token = getOperatorType(o), .type = OUTPUT_TYPE_TOKEN});
	}
}


void CALC_printRPN() {
	for(int i = 0; i < RPN_write_index; i++) {
		RPN_item item = current_function_RPN[i];
		CALC_printRPNItem(item);
	}
}


float CALC_evaluateRPN(float in) {
	stackDeinit_f(&RPN_value_stack);
	stackInit_f(&RPN_value_stack, 32);
	for(int i = 0; i < RPN_write_index; i++) {
		RPN_item item = current_function_RPN[i];
		uint8_t type = item.type;
		if(type == OUTPUT_TYPE_VALUE || item.token == OPERATOR_X) {
			if(item.token == OPERATOR_X)
					stackPush_f(&RPN_value_stack, in); // replace x operator with value
			else
				stackPush_f(&RPN_value_stack, item.value); // current item value
		} else if(type == OUTPUT_TYPE_TOKEN) {
			
			float n1, n2;
			if(item.token == OPERATOR_SIN || item.token == OPERATOR_COS || item.token == OPERATOR_TAN || item.token == OPERATOR_LOG || item.token == OPERATOR_LN) {
				if(stackEmpty_f(&RPN_value_stack)) { CALC_throwError("RPN evaluation error: could not find first value in single-sided expression"); }
				n1 = stackPop_f(&RPN_value_stack);
			} else {			
				if(stackEmpty_f(&RPN_value_stack)) { CALC_throwError("RPN evaluation error: could not find first value in two-sided expression"); }
				n1 = stackPop_f(&RPN_value_stack);
				if(stackEmpty_f(&RPN_value_stack)) { CALC_throwError("RPN evaluation error: could not find second value in two-sided expression"); }
				n2 = stackPop_f(&RPN_value_stack);
			}
			
			float result;
			switch(item.token) {
				case OPERATOR_MUL:
					result = n2 * n1;
					break;
				case OPERATOR_DIV:
					result = n2 / n1;
					break;
				case OPERATOR_SUB:
					result = n2 - n1;
					break;
				case OPERATOR_ADD:
					result = n2 + n1;
					break;
				case OPERATOR_SIN:
					result = sin(n1);
					break;
				case OPERATOR_COS:
					result = cos(n1);
					break;
				case OPERATOR_TAN:
					result = tan(n1);
					break;
				case OPERATOR_LOG:
					result = log10(n1);
					break;
				case OPERATOR_LN:
					result = log(n1);
					break;
				case OPERATOR_POW:
					result = pow(n2, n1);
					break;
			}
			
			// push result back onto value stack
			stackPush_f(&RPN_value_stack, result);
		}
	}
	
	if(RPN_value_stack.top > 0) CALC_throwError("RPN evaluation error: too many values in expression");
	return stackPop_f(&RPN_value_stack);
}

/*
void CALC_loadFromSlot(uint8_t slot) {
	
	// hard coded due to EEPROM problems
	CALC_clearCurrentFunction();
	CALC_appendString(CALC_getFromSlot(slot));
}

// DO NOT USE
void CALC_saveToSlot(uint8_t slot, char* data) {
	eeprom_write_block((void*)&data, (const void*)(SAVEDATA_ADDRESS + (slot * 32)), 32);
}
*/

// TODO: save and load to EEPROM
void CALC_saveCurrentFunction() {}
void CALC_loadCurrentFunction() {}

// functions to add strings to current function
void CALC_addX()    { 
	if(current_function_index > 0 && isNumber(current_function[current_function_index-1])) CALC_appendString("*");
	CALC_appendString("x");  
}
void CALC_add1()    { CALC_appendString("1");  }
void CALC_add2()    { CALC_appendString("2");  }
void CALC_add3()    { CALC_appendString("3");  }
void CALC_add4()    { CALC_appendString("4");  }
void CALC_add5()    { CALC_appendString("5");  }
void CALC_add6()    { CALC_appendString("6");  }
void CALC_add7()    { CALC_appendString("7");  }
void CALC_add8()    { CALC_appendString("8");  }
void CALC_add9()    { CALC_appendString("9");  }
void CALC_add0()    { CALC_appendString("0"); }
void CALC_addDot()  { }//CALC_appendString(".");	}
void CALC_addPow()  { CALC_appendString("^");	}
void CALC_addAdd()  { CALC_appendString("+");	}
void CALC_addSub()  { CALC_appendString("-");	}
void CALC_addMul()  { CALC_appendString("*");	}
void CALC_addDiv()  { CALC_appendString("/");	}
void CALC_addSin()  { CALC_appendString("sin"); CALC_appendString("("); }
void CALC_addCos()  { CALC_appendString("cos"); CALC_appendString("("); }
void CALC_addTan()  { CALC_appendString("tan"); CALC_appendString("("); }
void CALC_addLog()  { CALC_appendString("log"); CALC_appendString("("); }
void CALC_addLn()   { CALC_appendString("ln"); CALC_appendString("("); }
void CALC_addLPth() { CALC_appendString("(");	}
void CALC_addRPth() { CALC_appendString(")");	}

const char* slot1 = "(1/2)*x+1";
const char* slot2 = "sin(x)*5";
const char* slot3 = "cos(x^2)*5";
const char* slot4 = "tan(x)";
const char* slot5 = "x^3+x^2-3*x";
const char* slot6 = "ln(x)";

void CALC_loadSlot1() { CALC_clearCurrentFunction(); CALC_appendString(slot1);  CALC_setupGraphCreationMenu(); }
void CALC_loadSlot2() { CALC_clearCurrentFunction(); CALC_appendString(slot2);  CALC_setupGraphCreationMenu(); }
void CALC_loadSlot3() { CALC_clearCurrentFunction(); CALC_appendString(slot3);  CALC_setupGraphCreationMenu(); }
void CALC_loadSlot4() { CALC_clearCurrentFunction(); CALC_appendString(slot4);  CALC_setupGraphCreationMenu(); }
void CALC_loadSlot5() { CALC_clearCurrentFunction(); CALC_appendString(slot5);  CALC_setupGraphCreationMenu(); }
void CALC_loadSlot6() { CALC_clearCurrentFunction(); CALC_appendString(slot6);  CALC_setupGraphCreationMenu(); }


// re-initialize menu item array to zero
void CALC_clearMenuItems() {
	memset(items, 0, sizeof(items));
}


// draw box around specified menu item; width gives box thickness
void CALC_outlineMenuItem(menu_item item, uint8_t width) {
	uint16_t length_px = LCD_getStringPixelWidth(item.text);
	uint16_t height_px = LCD_getStringPixelHeight(item.text);
	for(unsigned int i = 1; i <= width; i++)
		LCD_drawRectOffset(item.position.x, item.position.y, item.position.x + length_px, item.position.y + height_px, LCD_getForeground(), i);
}


// draw underline under specified menu item; width gives line thickness
void CALC_underlineMenuItem(menu_item item, uint8_t width) {
	uint16_t length_px = LCD_getStringPixelWidth(item.text);
	uint16_t height_px = LCD_getStringPixelHeight(item.text);
	for(unsigned int i = 1; i <= width; i++)
		LCD_drawFastLine(item.position.x, item.position.y + height_px + i, item.position.x + length_px, item.position.y + height_px + i, LCD_getForeground());
}


// draw menu item's text and any effects
void CALC_drawMenuItem(menu_item item) {
	LCD_setTextSpacing(1);
	LCD_setTextSize(item.text_size);
	LCD_setTextWrap(FALSE);
	LCD_setTextPosition(item.position.x, item.position.y);
	LCD_drawString(item.text);
//	CALC_outlineMenuItem(item, 1);
//	CALC_underlineMenuItem(item, 1);
}


// simply calls draw functions on each menu item
void CALC_drawCurrentMenu() {	
	for(unsigned char i = 0; i < MENU_ITEM_MAX; i++)
		if(items[i].active && items[i].updated) {
			uint8_t old_size = LCD_getTextSize();
			CALC_drawMenuItem(items[i]);
			
			// draw cursor effect at item
			if(cursor.focus == &items[i]) { // shallow comparison
				if(cursor.type & CURSOR_TYPE_ARROW)
					LCD_drawChar(cursor.x - LCD_getStringPixelWidth(" "), cursor.y, '>', cursor.color, LCD_getBackground(), 1);
				if(cursor.type & CURSOR_TYPE_OUTLINE) {
					uint16_t old_col = LCD_getForeground();
					LCD_setForeground(cursor.color);
					CALC_outlineMenuItem(items[i], 1);
					LCD_setForeground(old_col);
				}
				if(cursor.type & CURSOR_TYPE_UNDERLINE) {
					uint16_t old_col = LCD_getForeground();
					LCD_setForeground(cursor.color);
					CALC_underlineMenuItem(items[i], 1);
					LCD_setForeground(old_col);
				}
			}
			
			// erase cursor effect at prev item
			if(cursor.prev == &items[i]) { // shallow comparison
				if(cursor.type & CURSOR_TYPE_ARROW) {
					LCD_drawChar(items[i].position.x - LCD_getStringPixelWidth(" "), items[i].position.y, '>', LCD_getBackground(), LCD_getBackground(), 1);
				}
				
				if(cursor.type & CURSOR_TYPE_OUTLINE) {
					uint16_t old_col = LCD_getForeground();
					LCD_setForeground(LCD_getBackground());
					CALC_outlineMenuItem(items[i], 1);
					LCD_setForeground(old_col);
				}
				
				if(cursor.type & CURSOR_TYPE_UNDERLINE) {
					uint16_t old_col = LCD_getForeground();
					LCD_setForeground(LCD_getBackground());
					CALC_underlineMenuItem(items[i], 1);
					LCD_setForeground(old_col);
				}
				
			}
			
			// reset LCD formatting globals
			LCD_setTextSize(old_size);
			
			items[i].updated = 0;
		}
		
	// menu special graphics
	switch(menu_type) {
		case MENU_TYPE_CREATEGRAPH:
			;
			if(!current_function_updated) break;
			current_function_updated = 0;
			LCD_fillRect(4, 4, LCD_XMAX - 4, 30, LCD_getBackground());
			LCD_drawRectOffset(4, 4, LCD_XMAX - 4, 30, LCD_getForeground(), 1);
			LCD_setTextPosition(10, 10);
			LCD_setTextSize(1);
			LCD_setTextWrap(TRUE);
			LCD_drawString("f(x) = ");
			LCD_drawString(current_function);
			LCD_drawString("\x5F");
			break;
		default:
			break;
	}
}


// setup main menu items and graphics
void CALC_setupMainMenu() {
	CALC_clearMenuItems();
	LCD_clearColor(LCD_getBackground());
	menu_type = MENU_TYPE_MAINMENU;
	uint8_t margin_left = 20; // menu left alignment point
	uint8_t margin_top = 60;
	items[0] = (menu_item){		MENU_ITEM_DEFAULT, .text = "Create Graph",		.position = { .x = margin_left, .y = margin_top +  0 }, .callback = CALC_setupGraphCreationMenu,
								.down = &items[1], .up = &items[2]
							};
	items[1] = (menu_item){		MENU_ITEM_DEFAULT, .text = "Load Graph",	.position = { .x = margin_left, .y = margin_top + 30 }, .callback = CALC_setupGraphLoadMenu,
								.down = &items[2], .up = &items[0]
							};
	items[2] = (menu_item){		MENU_ITEM_DEFAULT, .text = "Settings",			.position = { .x = margin_left, .y = margin_top + 60 }, .callback = CALC_setupSettingsMenu,
								.down = &items[0], .up = &items[1]
							};
							
	uint16_t old_fg = LCD_getForeground();
	LCD_setTextPosition(10, 10);
	LCD_setTextSpacing(3);
	LCD_setTextSize(2);
	LCD_setForeground(LCD_GREEN);
	LCD_drawString("GRAPH");
	LCD_setForeground(CALC_colorscheme ? LCD_YELLOW : LCD_RED);
	LCD_drawString("128");
	LCD_setForeground(old_fg);
		
	// default cursor focus
	cursor.type = CURSOR_TYPE_ARROW;
	cursor.focus = &items[0];
	cursor.prev = NULL;
	CALC_update();
	CALC_drawCurrentMenu();
}


void CALC_cycle_colorscheme() { 
	CALC_colorscheme = !CALC_colorscheme;
	LCD_setForeground(CALC_colorscheme ? LCD_GREEN : LCD_BLACK);
	LCD_setBackground(CALC_colorscheme ? LCD_BLACK : LCD_WHITE);
	cursor.color = CALC_colorscheme ? LCD_WHITE : LCD_BLACK;
	CALC_setupSettingsMenu(); 
}


// setup main menu items and graphics
void CALC_setupSettingsMenu() {
	CALC_clearMenuItems();
	LCD_clearColor(LCD_getBackground());
	menu_type = MENU_TYPE_SETTINGS;
	uint8_t margin_left = 10; // menu left alignment point
	uint8_t margin_top = 10;
	items[0] = (menu_item){		MENU_ITEM_DEFAULT, .text = "Toggle colorscheme",	.position = { .x = margin_left, .y = margin_top +  0 }, .callback = CALC_cycle_colorscheme,
								.down = &items[1], .up = &items[2]
							};
	items[1] = (menu_item){		MENU_ITEM_DEFAULT, .text = "Invert colors",		.position = { .x = margin_left, .y = margin_top + 20 }, .callback = LCD_invert,
								.down = &items[2], .up = &items[0]
							};
	items[2] = (menu_item){		MENU_ITEM_DEFAULT, .text = "Back to Main",	.position = { .x = margin_left, .y = margin_top + 40 }, .callback = CALC_setupMainMenu,
								.down = &items[0], .up = &items[1]
							};
		
	// default cursor focus
	cursor.type = CURSOR_TYPE_UNDERLINE;
	cursor.focus = &items[0];
	cursor.prev = NULL;
	CALC_update();
	CALC_drawCurrentMenu();
}


// HELPFUL DEFINES FOR BUTTON ITEMS
// variables
#define PADX items[0]
#define PADY items[1]

// numbers
#define PAD1 items[2]
#define PAD2 items[3]
#define PAD3 items[4]
#define PAD4 items[5]
#define PAD5 items[6]
#define PAD6 items[7]
#define PAD7 items[8]
#define PAD8 items[9]
#define PAD9 items[10]
#define PAD0 items[11]
#define PADD items[12]

// operators
#define PADPOW items[22]
#define PADDIV items[13]
#define PADMUL items[14]
#define PADSUB items[15]
#define PADADD items[16]

// special functions
#define PADSIN items[17]
#define PADCOS items[18]
#define PADTAN items[19]
#define PADLOG items[20]
#define PADLN  items[21]

// parenthesis
#define PADLPTH items[23]
#define PADRPTH items[24]

// movement buttons
#define PADCLR items[25]
#define PADLOAD items[26]
#define PADSAVE items[27]
#define PADOK items[28]

void CALC_setupGraphCreationMenu() {
	CALC_clearMenuItems();
	LCD_clearColor(LCD_getBackground());
	menu_type = MENU_TYPE_CREATEGRAPH;
	
	// clear all RPN stuff
	stackDeinit_c(&operator_stack);
	stackInit_c(&operator_stack, 32);
	RPN_write_index = 0;
	
	// standard offsets
	uint8_t sp1 = 15;
	uint8_t sp2 = 30;
	uint8_t sp3 = 45;
	
	// VARIABLES
	uint8_t margin_left = 15; // menu left alignment point
	uint8_t margin_top = 60 - sp1;
	PADX = (menu_item){		MENU_ITEM_DEFAULT, .text = "x",		.position = { .x = margin_left, .y = margin_top +  0 }, .callback = CALC_addX,
								.right = &PADLPTH, .down = &PAD7, .up = &PADLOAD, .left = &PADSIN
							};
//	PADY = (menu_item){		MENU_ITEM_DEFAULT, .text = "y",		.position = { .x = margin_left + sp1, .y = margin_top +  0 }, .callback = CALC_addY,
//								.left = &PADX, .right = &PADLPTH, .down = &PAD8
//							};
							
	// PARENTHESIS
	
	PADLPTH = (menu_item){	MENU_ITEM_DEFAULT, .text = "(",		.position = { .x = margin_left + sp1, .y = margin_top +  0 }, .callback = CALC_addLPth,
								.left = &PADX, .right = &PADRPTH, .down = &PAD8, .up = &PADLOAD
							};
	PADRPTH = (menu_item){	MENU_ITEM_DEFAULT, .text = ")",		.position = { .x = margin_left + sp2, .y = margin_top +  0 }, .callback = CALC_addRPth,
								.left = &PADLPTH, .right = &PADPOW, .down = &PAD9, .up = &PADLOAD
							};
							
							
	// NUMBER PAD
	// 1 2 3
	// 4 5 6
	// 7 8 9
	//   0
	margin_left = 15;
	margin_top = 60;
	
	// 1
	PAD1 = (menu_item){		MENU_ITEM_DEFAULT, .text = "1",		.position = { .x = margin_left + 0, .y = margin_top +  sp2 }, .callback = CALC_add1,
								.right = &PAD2, .up = &PAD4, .down = &PAD0, .left = &PADLOG
							};
	// 2
	PAD2 = (menu_item){		MENU_ITEM_DEFAULT, .text = "2",		.position = { .x = margin_left + sp1, .y = margin_top +  sp2 }, .callback = CALC_add2,
								.left = &PAD1, .right = &PAD3, .up = &PAD5, .down = &PAD0
							};
	// 3
	PAD3 = (menu_item){		MENU_ITEM_DEFAULT, .text = "3",		.position = { .x = margin_left + sp2, .y = margin_top +  sp2 }, .callback = CALC_add3,
								.left = &PAD2, .up = &PAD6, .down =  &PADD, .right = &PADSUB
							};
	// 4
	PAD4 = (menu_item){		MENU_ITEM_DEFAULT, .text = "4",		.position = { .x = margin_left + 0, .y = margin_top +  sp1 }, .callback = CALC_add4,
								.right = &PAD5, .down = &PAD1, .up = &PAD7, .left = &PADTAN
							};
	// 5
	PAD5 = (menu_item){		MENU_ITEM_DEFAULT, .text = "5",		.position = { .x = margin_left + sp1, .y = margin_top +  sp1 }, .callback = CALC_add5,
								.right = &PAD6, .down = &PAD2, .up = &PAD8, .left = &PAD4
							};
	// 6
	PAD6 = (menu_item){		MENU_ITEM_DEFAULT, .text = "6",		.position = { .x = margin_left + sp2, .y = margin_top +  sp1 }, .callback = CALC_add6,
								.left = &PAD5, .down = &PAD3, .up = &PAD9, .right = &PADMUL
							};
	// 7
	PAD7 = (menu_item){		MENU_ITEM_DEFAULT, .text = "7",		.position = { .x = margin_left + 0, .y = margin_top +  0 }, .callback = CALC_add7,
								.right = &PAD8, .up = &PADX, .down = &PAD4, .left = &PADCOS
							};
	// 8
	PAD8 = (menu_item){		MENU_ITEM_DEFAULT, .text = "8",		.position = { .x = margin_left + sp1, .y = margin_top +  0 }, .callback = CALC_add8,
								.right = &PAD9, .left = &PAD7, .down = &PAD5, .up = &PADLPTH
							};
	// 9
	PAD9 = (menu_item){		MENU_ITEM_DEFAULT, .text = "9",		.position = { .x = margin_left + sp2, .y = margin_top +  0 }, .callback = CALC_add9,
								.left = &PAD8, .down = &PAD6, .up = &PADRPTH, .right = &PADDIV
							};
	// 0
	PAD0 = (menu_item){		MENU_ITEM_DEFAULT, .text = "0",		.position = { .x = margin_left + sp1, .y = margin_top +  sp3 }, .callback = CALC_add0,
								.up = &PAD2, .right = &PADD, .down = &PADCLR, .left = &PAD1
							};
	// decimal
	PADD = (menu_item){		MENU_ITEM_DEFAULT, .text = ".",		.position = { .x = margin_left + sp2, .y = margin_top +  sp3 }, .callback = CALC_addDot,
								.up = &PAD3, .left = &PAD0, .right = &PADADD, .down = &PADCLR
							};
							
	// Basic operators
	margin_left = 70;
	
	// power
	PADPOW = (menu_item){	MENU_ITEM_DEFAULT, .text = "^",		.position = { .x = margin_left, .y = margin_top - sp1 }, .callback = CALC_addPow,
								.left = &PADRPTH, .right = &PADSIN, .down = &PADDIV, .up = &PADOK
							};
	// divide
	PADDIV = (menu_item){		MENU_ITEM_DEFAULT, .text = "/",		.position = { .x = margin_left, .y = margin_top +  0 }, .callback = CALC_addDiv,
								.down = &PADMUL, .left = &PAD9, .right = &PADCOS, .up = &PADPOW
							};
	// multiply
	PADMUL = (menu_item){		MENU_ITEM_DEFAULT, .text = "*",	.position = { .x = margin_left, .y = margin_top +  sp1 }, .callback = CALC_addMul,
								.up = &PADDIV, .down = &PADSUB, .left = &PAD6, .right = &PADTAN
							};
	// subtract
	PADSUB = (menu_item){		MENU_ITEM_DEFAULT, .text = "-",		.position = { .x = margin_left, .y = margin_top +  sp2 }, .callback = CALC_addSub,
								.up = &PADMUL, .down = &PADADD, .left = &PAD3, .right = &PADLOG
							};
	// add
	PADADD = (menu_item){		MENU_ITEM_DEFAULT, .text = "+",		.position = { .x = margin_left, .y = margin_top +  sp3 }, .callback = CALC_addAdd,
								.up = &PADSUB, .left = &PADD, .right = &PADLN, .down = &PADCLR
							};
	
	// special functions
	margin_left = 95;
	
	// sin
	PADSIN = (menu_item){		MENU_ITEM_DEFAULT, .text = "sin",	.position = { .x = margin_left, .y = margin_top -  sp1 }, .callback = CALC_addSin,
								.left = &PADPOW, .down = &PADCOS, .up = &PADOK, .right = &PADX
							};
	// cos
	PADCOS = (menu_item){		MENU_ITEM_DEFAULT, .text = "cos",	.position = { .x = margin_left, .y = margin_top +  0 }, .callback = CALC_addCos,
								.up = &PADSIN, .down = &PADTAN, .left = &PADDIV, .right = &PAD7
							};
	// tan
	PADTAN = (menu_item){		MENU_ITEM_DEFAULT, .text = "tan",	.position = { .x = margin_left, .y = margin_top +  sp1 }, .callback = CALC_addTan,
								.up = &PADCOS, .down = &PADLOG, .left = &PADMUL, .right = &PAD4
							};
	// log
	PADLOG = (menu_item){		MENU_ITEM_DEFAULT, .text = "log",	.position = { .x = margin_left, .y = margin_top +  sp2 }, .callback = CALC_addLog,
								.up = &PADTAN, .down = &PADLN, .left = &PADSUB, .right = &PAD1
							};
	// ln
	PADLN  = (menu_item){		MENU_ITEM_DEFAULT, .text = "ln",	.position = { .x = margin_left, .y = margin_top +  sp3 }, .callback = CALC_addLn,
								.up = &PADLOG, .left = &PADADD, .down = &PADOK, .right = &PAD0
							};
							
	// control buttons
	margin_left = 40;
	margin_top = 125;
	// clear
	PADCLR  = (menu_item){		MENU_ITEM_DEFAULT, .text = "del",	.position = { .x = margin_left, .y = margin_top +  0 }, .callback = CALC_popChar,
								.up = &PADADD, .down = &PADSAVE, .right = &PADOK, .left = &PADOK
							};
	// save
	PADSAVE  = (menu_item){		MENU_ITEM_DEFAULT, .text = "clear",	.position = { .x = margin_left, .y = margin_top +  11 }, .callback = CALC_clearCurrentFunction,
								.up = &PADCLR, .down = &PADLOAD, .right = &PADOK, .left = &PADOK
							};
	// load
	PADLOAD  = (menu_item){		MENU_ITEM_DEFAULT, .text = "load",	.position = { .x = margin_left, .y = margin_top +  22 }, .callback = CALC_setupGraphLoadMenu,
								.up = &PADSAVE, .right = &PADOK, .down = &PADRPTH, .left = &PADOK
							};
	// OK
	PADOK  = (menu_item){		MENU_ITEM_DEFAULT, .text = "OK",	.position = { .x = margin_left + 40, .y = margin_top + 5 }, .callback = CALC_setupGraphDisplayMenu,
								.text_size = 3, .left = &PADCLR, .up = &PADLN, .down = &PADSIN
							};
							
	// outline numpad
	LCD_drawRect(10, 40, 55, 117, LCD_getForeground());
	LCD_drawLine(10, 57, 55, 57, LCD_getForeground());
	
	// outline operators
	LCD_drawRect(65, 40, 81, 117, LCD_getForeground());
	
	// outline functions
	LCD_drawRect(90, 40, 117, 117, LCD_getForeground());
	
//	CALC_clearCurrentFunction();
	
	// load pre-loaded function
//	CALC_appendString("sin(2*x)*4");
	
	current_function_updated = 1;
	
	// default cursor focus
	cursor.type = CURSOR_TYPE_OUTLINE;
	cursor.focus = &items[0];
	cursor.prev = NULL;
	CALC_update();
	CALC_drawCurrentMenu();
}


void CALC_setupGraphDisplayMenu() {
	CALC_clearMenuItems();
	LCD_clearColor(LCD_getBackground());
	menu_type = MENU_TYPE_DISPLAYGRAPH;
	
	uint16_t graph_box_color =  CALC_colorscheme ? LCD_WHITE : LCD_BLACK;
	uint16_t graph_axis_color = CALC_colorscheme ? LCD_CYAN : LCD_RED;
	uint16_t graph_grid_color = CALC_colorscheme ? LCD_GREEN : LCD_GREEN;
	uint16_t graph_plot_color = CALC_colorscheme ? LCD_YELLOW : LCD_BLUE;
	
	
	// draw grid
	uint8_t graph_x0 = 4;
	uint8_t graph_y0 = 4;
	uint8_t graph_x1 = LCD_XMAX - 4;
	uint8_t graph_y1 = 124;
	uint8_t graph_width = graph_x1 - graph_x0;
	uint8_t graph_height = graph_y1 - graph_y0;
	uint8_t graph_x_axis = graph_width/2;
	uint8_t graph_y_axis = graph_height/2;
	
	uint8_t graph_grid_spacing = 10;
	uint8_t graph_scale = graph_width / (graph_grid_spacing);
	
	// horizontal grid
	for(int16_t r = 0; r < graph_height; r++)
		if((r - graph_y_axis) % graph_grid_spacing == 0)
			LCD_drawLine(graph_x0, graph_y0 + r, graph_x1, graph_y0 + r, graph_grid_color);
			
	// vertical grid
	for(int16_t c = 0; c < graph_width; c++)
		if((c - graph_x_axis) % graph_grid_spacing == 0)
			LCD_drawLine(graph_x0 + c, graph_y0, graph_x0 + c, graph_y1, graph_grid_color);

	LCD_drawLine(graph_x0 + graph_x_axis, graph_y0, graph_x0 + graph_x_axis, graph_y1, graph_axis_color);	// y axis
	LCD_drawLine(graph_x0, graph_y0 + graph_y_axis, graph_x1, graph_y0 + graph_y_axis, graph_axis_color);	// x axis
	
	LCD_drawRect(graph_x0, graph_y0, graph_x1, graph_y1, graph_box_color); // graph box
	
	// perform string to function conversion, prepare for plotting
	CALC_convertToRPN(current_function);
	LCD_setTextPosition(10, 130);
	LCD_drawString("f(x) = ");
	LCD_drawString(current_function);
	LCD_setTextPosition(10, 145);
	LCD_drawString("graphing...");
	/*
	CALC_printRPN();
	LCD_drawString(" = ");
	float test = CALC_evaluateRPN(1);
	char str[32];
	itoa((int)(test+0.5), str, 10);
	LCD_drawString(str);
	*/
	
	// plot
	for(int16_t c = 0; c < graph_width; c++) {
	//	static uint8_t prev_x = 0;
	//	static uint8_t prev_y = 0;
		// probe two intervals to see how many samples are needed
		float x_test0 = ((float)c - graph_y_axis) / graph_scale;
		float x_test1 = ((float)(c + 1) - graph_y_axis) / graph_scale;
		float y_test0 = CALC_evaluateRPN(x_test0);
		float y_test1 = CALC_evaluateRPN(x_test1);
		int16_t y_test_px0 = (int16_t)(y_test0 * -graph_scale + graph_x_axis + 0.5f) + 1;
		int16_t y_test_px1 = (int16_t)(y_test1 * -graph_scale + graph_x_axis + 0.5f) + 1;
		
		// if both samples not within bounds, skip
		if((y_test_px0 < 0 || y_test_px0 >= graph_height) && (y_test_px1 < 0 || y_test_px1 >= graph_height)) continue;
		
		// multi-sampled pixel plotting
		uint8_t sample_count = CLAMP((uint8_t)abs(y_test_px1 - y_test_px0) * 2, 120, 4);
		for(int16_t s = 0; s < sample_count; s++) {
			// graph coordinates
			float x = ((float)c + (s * 1.0f / sample_count) - graph_y_axis) / graph_scale;
			float y = CALC_evaluateRPN(x);
			// pixel coordinates
			int16_t x_px = (int16_t)(c);
			int16_t y_px = (int16_t)((y * -graph_scale * 0.9f) + graph_x_axis + 0.5f) + 1; // convert to pixel coordinates (width minor adjustments)
		
			// reject if out of bounds
			if(y_px < 0 || y_px >= graph_height) continue;
		
			// draw pixel
			if(c != 0)
				LCD_drawPixel(graph_x0 + (int16_t)x_px, graph_y0 + (int16_t)y_px, graph_plot_color);
		}
	}
	
	LCD_drawRect(graph_x0, graph_y0, graph_x1, graph_y1, graph_box_color); // redraw graph box
	LCD_fillRect(0, 140, 128, 160, LCD_getBackground());
	
	uint8_t margin_left = 65; // menu left alignment point
	uint8_t margin_top = 148;
	
	items[0] = (menu_item){		MENU_ITEM_DEFAULT, .text = "back",			.position = { .x = margin_left + 0, .y = margin_top +  0 }, .callback = CALC_setupGraphCreationMenu,
								.right = &items[1], .left = &items[1]
							};
	items[1] = (menu_item){		MENU_ITEM_DEFAULT, .text = "main",		.position = { .x = margin_left + 30, .y = margin_top + 0 }, .callback = CALC_setupMainMenu,
								.right = &items[0], .left = &items[0]
							};
	
	// default cursor focus
	cursor.type = CURSOR_TYPE_UNDERLINE;
	cursor.focus = &items[0];
	cursor.prev = NULL;
	
	// force update
	CALC_update();
	CALC_drawCurrentMenu();
}

char slot1_str[16] = {0};
char slot2_str[16] = {0};
char slot3_str[16] = {0};
char slot4_str[16] = {0};
char slot5_str[16] = {0};
char slot6_str[16] = {0};

void CALC_setupGraphLoadMenu() {
	CALC_clearMenuItems();
	LCD_clearColor(LCD_getBackground());
	menu_type = MENU_TYPE_DISPLAYGRAPH;
	strcpy(slot1_str, "1: ");
	strcpy(slot2_str, "2: ");
	strcpy(slot3_str, "3: ");
	strcpy(slot4_str, "4: ");
	strcpy(slot5_str, "5: ");
	strcpy(slot6_str, "6: ");
	strcat(slot1_str, slot1);
	strcat(slot2_str, slot2);
	strcat(slot3_str, slot3);
	strcat(slot4_str, slot4);
	strcat(slot5_str, slot5);
	strcat(slot6_str, slot6);
	
	uint8_t margin_left = 15; // menu left alignment point
	uint8_t margin_top = 10;
	items[0] = (menu_item){		MENU_ITEM_DEFAULT, .text = slot1_str,	.position = { .x = margin_left, .y = margin_top +  0 }, .callback = CALC_loadSlot1,
								.up = &items[7], .down = &items[1]
							};
	items[1] = (menu_item){		MENU_ITEM_DEFAULT, .text = slot2_str,	.position = { .x = margin_left, .y = margin_top + 15 }, .callback = CALC_loadSlot2,
								.up = &items[0], .down = &items[2]
							};
	items[2] = (menu_item){		MENU_ITEM_DEFAULT, .text = slot3_str,	.position = { .x = margin_left, .y = margin_top +  30 }, .callback = CALC_loadSlot3,
								.up = &items[1], .down = &items[3]
							};
	items[3] = (menu_item){		MENU_ITEM_DEFAULT, .text = slot4_str,	.position = { .x = margin_left, .y = margin_top + 45 }, .callback = CALC_loadSlot4,
								.up = &items[2], .down = &items[4]
							};
	items[4] = (menu_item){		MENU_ITEM_DEFAULT, .text = slot5_str,	.position = { .x = margin_left, .y = margin_top +  60 }, .callback = CALC_loadSlot5,
								.up = &items[3], .down = &items[5]
							};
	items[5] = (menu_item){		MENU_ITEM_DEFAULT, .text = slot6_str,	.position = { .x = margin_left, .y = margin_top + 75 }, .callback = CALC_loadSlot6,
								.up = &items[4], .down = &items[6]
							};
							
	items[6] = (menu_item){		MENU_ITEM_DEFAULT, .text = "Back to Main",	.position = { .x = margin_left + 20, .y = margin_top + 110 }, .callback = CALC_setupMainMenu,
								.up = &items[5], .down = &items[7]
							};
							
	items[7] = (menu_item){		MENU_ITEM_DEFAULT, .text = "Back to Input",	.position = { .x = margin_left + 20, .y = margin_top + 125 }, .callback = CALC_setupGraphCreationMenu,
								.up = &items[6], .down = &items[0]
							};
	
	// default cursor focus
	cursor.type = CURSOR_TYPE_ARROW | CURSOR_TYPE_UNDERLINE;
	cursor.focus = &items[0];
	cursor.prev = NULL;
	
	// force update
	CALC_update();
	CALC_drawCurrentMenu();
}


CALC_throwError(const char* msg) {
	LCD_clearColor(LCD_RED);	
	LCD_setTextPosition(0, 0);
	LCD_setForeground(LCD_WHITE);
	LCD_setBackground(LCD_RED);
	LCD_setTextSize(3);
	LCD_drawString("ERROR");
	LCD_setTextSize(1);
	LCD_setTextWrap(TRUE);
	LCD_setTextPosition(0, 30);
	LCD_drawString(msg);
	while(1);
};
