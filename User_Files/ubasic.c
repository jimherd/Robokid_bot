/*
 * Copyright (c) 2006, Adam Dunkels
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

//#define DEBUG 0

//#if DEBUG
//#define  DEBUG_PRINTF(...)  sprintf(__VA_ARGS__)
//#else
//#define  DEBUG_PRINTF(const char *st, ...)
//#endif

#include "global.h"

static char const *program_ptr;
#define MAX_STRINGLEN 40
static char string[MAX_STRINGLEN];

#define MAX_GOSUB_STACK_DEPTH 10
static int gosub_stack[MAX_GOSUB_STACK_DEPTH];
static int gosub_stack_ptr;

struct for_state {
  int16_t line_after_for;
  int16_t for_variable;
  int to;
};
#define MAX_FOR_STACK_DEPTH 4
static struct for_state for_stack[MAX_FOR_STACK_DEPTH];
static int for_stack_ptr;

#define MAX_VARNUM 26
static int16_t variables[MAX_VARNUM];

static uint8_t ended;

static int16_t expr(void);
static void line_statement(void);
static void statement(void);

static uint8_t  gUbasic_Error;
/*---------------------------------------------------------------------------*/
void ubasic_init(const char *program) 
{
	program_ptr = program;
	gUbasic_Error = 0;
	for_stack_ptr = gosub_stack_ptr = 0;
	tokenizer_init(program);
	ended = 0;
}
/*---------------------------------------------------------------------------*/
static void accept(uint8_t token) 
{
	if (token != tokenizer_token()) {
		//     DEBUG_PRINTF("Token not what was expected (expected %d, got %d)\n",
		//		 token, tokenizer_token());
		tokenizer_error_print();
		exit(1);
	}
	// DEBUG_PRINTF("Expected %d, got it\n", token);
	tokenizer_next();
}
/*---------------------------------------------------------------------------*/
static int16_t varfactor(void) 
{
	int r;
	// DEBUG_PRINTF("varfactor: obtaining %d from variable %d\n", variables[tokenizer_variable_num()], tokenizer_variable_num());
	r = ubasic_get_variable(tokenizer_variable_num());
	accept(TOKENIZER_VARIABLE);
	return r;
}
/*---------------------------------------------------------------------------*/
static int16_t factor(void) 
{
	int16_t r;

	// DEBUG_PRINTF("factor: token %d\n", tokenizer_token());
	switch (tokenizer_token()) {
	case TOKENIZER_NUMBER:
		r = tokenizer_num();
		// DEBUG_PRINTF("factor: number %d\n", r);
		accept(TOKENIZER_NUMBER);
		break;
	case TOKENIZER_LEFTPAREN:
		accept(TOKENIZER_LEFTPAREN);
		r = expr();
		accept(TOKENIZER_RIGHTPAREN);
		break;
	default:
		r = varfactor();
		break;
	}
	return r;
}
/*---------------------------------------------------------------------------*/
static int16_t term(void) 
{
	int16_t f1, f2;
	int16_t op;

	f1 = factor();
	op = tokenizer_token();
	// DEBUG_PRINTF("term: token %d\n", op);
	while (op == TOKENIZER_ASTR || op == TOKENIZER_SLASH || op == TOKENIZER_MOD) {
		tokenizer_next();
		f2 = factor();
		// DEBUG_PRINTF("term: %d %d %d\n", f1, op, f2);
		switch(op) {
			case TOKENIZER_ASTR:
			f1 = f1 * f2;
			break;
			case TOKENIZER_SLASH:
			f1 = f1 / f2;
			break;
			case TOKENIZER_MOD:
			f1 = f1 % f2;
			break;
		}
		op = tokenizer_token();
	}
	// DEBUG_PRINTF("term: %d\n", f1);
	return f1;
}
/*---------------------------------------------------------------------------*/
static int16_t expr(void) 
{
	int16_t t1, t2;
	int16_t op;

	t1 = term();
	op = tokenizer_token();
	// DEBUG_PRINTF("expr: token %d\n", op);
	while (op == TOKENIZER_PLUS || op == TOKENIZER_MINUS || op == TOKENIZER_AND
			|| op == TOKENIZER_OR) {
		tokenizer_next();
		t2 = term();
		// DEBUG_PRINTF("expr: %d %d %d\n", t1, op, t2);
		switch(op) {
			case TOKENIZER_PLUS:
			t1 = t1 + t2;
			break;
			case TOKENIZER_MINUS:
			t1 = t1 - t2;
			break;
			case TOKENIZER_AND:
			t1 = t1 & t2;
			break;
			case TOKENIZER_OR:
			t1 = t1 | t2;
			break;
		}
		op = tokenizer_token();
	}
	// DEBUG_PRINTF("expr: %d\n", t1);
	return t1;
}
/*---------------------------------------------------------------------------*/
static int16_t relation(void) 
{
	int16_t r1, r2;
	int16_t op;

	r1 = expr();
	op = tokenizer_token();
	// DEBUG_PRINTF("relation: token %d\n", op);
	while (op == TOKENIZER_LT || op == TOKENIZER_GT || op == TOKENIZER_EQ) {
		tokenizer_next();
		r2 = expr();
		// DEBUG_PRINTF("relation: %d %d %d\n", r1, op, r2);
		switch(op) {
			case TOKENIZER_LT:
			r1 = r1 < r2;
			break;
			case TOKENIZER_GT:
			r1 = r1 > r2;
			break;
			case TOKENIZER_EQ:
			r1 = r1 == r2;
			break;
		}
		op = tokenizer_token();
	}
	return r1;
}
/*---------------------------------------------------------------------------*/
static void jump_linenum(int16_t linenum) 
{
	tokenizer_init(program_ptr);
	while (tokenizer_num() != linenum) {
		do {
			do {
				tokenizer_next();
			}while(tokenizer_token() != TOKENIZER_CR &&
					tokenizer_token() != TOKENIZER_ENDOFINPUT);
			if(tokenizer_token() == TOKENIZER_CR) {
				tokenizer_next();
			}
		}while(tokenizer_token() != TOKENIZER_NUMBER);
		// DEBUG_PRINTF("jump_linenum: Found line %d\n", tokenizer_num());
	}
}
/*---------------------------------------------------------------------------*/
static void goto_statement(void) 
{
	accept(TOKENIZER_GOTO);
	jump_linenum(tokenizer_num());
}
/*---------------------------------------------------------------------------*/
static void print_statement(void) 
{
	accept(TOKENIZER_PRINT);
	do {
		if(tokenizer_token() == TOKENIZER_STRING) {
			tokenizer_string(string, sizeof(string));
			send_msg(string);
			tokenizer_next();
		} else if(tokenizer_token() == TOKENIZER_COMMA) {
			send_msg(" ");
			tokenizer_next();
		} else if(tokenizer_token() == TOKENIZER_SEMICOLON) {
			tokenizer_next();
		} else if((tokenizer_token() == TOKENIZER_VARIABLE) || (tokenizer_token() == TOKENIZER_NUMBER)) {
			//			send_msg(bcd(expr(), tempstring));
		} else {
			break;
		}
	} while((tokenizer_token() != TOKENIZER_CR) && (tokenizer_token() != TOKENIZER_ENDOFINPUT));
	send_msg("\n");
// DEBUG_PRINTF("End of print\n");
	tokenizer_next();
}
/*---------------------------------------------------------------------------*/
static void if_statement(void) 
{
	int16_t r;

	accept(TOKENIZER_IF);

	r = relation();
	// DEBUG_PRINTF("if_statement: relation %d\n", r);
	accept(TOKENIZER_THEN);
	if (r) {
		statement();
	} else {
		do{
		tokenizer_next();
	}while(tokenizer_token() != TOKENIZER_ELSE &&
			tokenizer_token() != TOKENIZER_CR &&
			tokenizer_token() != TOKENIZER_ENDOFINPUT);
	if(tokenizer_token() == TOKENIZER_ELSE) {
		tokenizer_next();
		statement();
	} else if(tokenizer_token() == TOKENIZER_CR) {
		tokenizer_next();
	}
}
}
/*---------------------------------------------------------------------------*/
static void let_statement(void) 
{
	int16_t var;

	var = tokenizer_variable_num();

	accept(TOKENIZER_VARIABLE);
	accept(TOKENIZER_EQ);
	ubasic_set_variable(var, expr());
	// DEBUG_PRINTF("let_statement: assign %d to %d\n", variables[var], var);
	accept(TOKENIZER_CR);

}
/*---------------------------------------------------------------------------*/
static void gosub_statement(void) 
{
	int16_t linenum;
	accept(TOKENIZER_GOSUB);
	linenum = tokenizer_num();
	accept(TOKENIZER_NUMBER);
	accept(TOKENIZER_CR);
	if (gosub_stack_ptr < MAX_GOSUB_STACK_DEPTH) {
		gosub_stack[gosub_stack_ptr] = tokenizer_num();
		gosub_stack_ptr++;
		jump_linenum(linenum);
	} else {
		// DEBUG_PRINTF("gosub_statement: gosub stack exhausted\n");
	}
}
/*---------------------------------------------------------------------------*/
static void return_statement(void) 
{
	accept(TOKENIZER_RETURN);
	if (gosub_stack_ptr > 0) {
		gosub_stack_ptr--;
		jump_linenum(gosub_stack[gosub_stack_ptr]);
	} else {
		// DEBUG_PRINTF("return_statement: non-matching return\n");
	}
}
/*---------------------------------------------------------------------------*/
static void next_statement(void) 
{
	int16_t var;

	accept(TOKENIZER_NEXT);
	var = tokenizer_variable_num();
	accept(TOKENIZER_VARIABLE);
	if (for_stack_ptr > 0 && var == for_stack[for_stack_ptr - 1].for_variable) {
		ubasic_set_variable(var, ubasic_get_variable(var) + 1);
		if (ubasic_get_variable(var) <= for_stack[for_stack_ptr - 1].to) {
			jump_linenum(for_stack[for_stack_ptr - 1].line_after_for);
		} else {
			for_stack_ptr--;
			accept(TOKENIZER_CR);
		}
	} else {
		// DEBUG_PRINTF("next_statement: non-matching next (expected %d, found %d)\n", for_stack[for_stack_ptr - 1].for_variable, var);
		accept(TOKENIZER_CR);
	}

}
/*---------------------------------------------------------------------------*/
static void for_statement(void) 
{
	int for_variable, to;

	accept(TOKENIZER_FOR);
	for_variable = tokenizer_variable_num();
	accept(TOKENIZER_VARIABLE);
	accept(TOKENIZER_EQ);
	ubasic_set_variable(for_variable, expr());
	accept(TOKENIZER_TO);
	to = expr();
	accept(TOKENIZER_CR);

	if (for_stack_ptr < MAX_FOR_STACK_DEPTH) {
		for_stack[for_stack_ptr].line_after_for = tokenizer_num();
		for_stack[for_stack_ptr].for_variable = for_variable;
		for_stack[for_stack_ptr].to = to;
		//     DEBUG_PRINTF("for_statement: new for, var %d to %d\n",
		//		 for_stack[for_stack_ptr].for_variable,
		//		 for_stack[for_stack_ptr].to);

		for_stack_ptr++;
	} else {
		// DEBUG_PRINTF("for_statement: for stack depth exceeded\n");
	}
}
/*---------------------------------------------------------------------------*/
static void end_statement(void) 
{
	accept(TOKENIZER_END);
	ended = 1;
}
/*---------------------------------------------------------------------------*/
static void wait_statement(void) 
{
	uint16_t wait_value, count, temp;

	accept(TOKENIZER_WAIT);
	wait_value = expr();
	//
	// wait_value is in units of 0.1 sec.  Convert to units of 8mS
	//
	count = (wait_value * 12) + (wait_value >> 1); // convert to units of 8mS
	CLR_TIMER16;
	FOREVER {
		GET_TIMER16(temp);
		if (temp > count) {
			break;
		}
	}
//	accept(TOKENIZER_NUMBER);
	accept(TOKENIZER_CR);
}
/*---------------------------------------------------------------------------*/
// leds_statement : set/clear the four on-board LEDs
// ==============
//
// Format :  leds  led_A  led_B  led_C  led_D
//
//        led_* : 0=OFF, 1=ON,  2=leave in current state
//
static void leds_statement(void) 
{
uint8_t led_value;
	
	accept(TOKENIZER_LEDS);
	led_value = tokenizer_num();
	switch (led_value) {
		case 0 :
			clr_LED(LED_A); break;
		case 1 : 
			set_LED(LED_A, FLASH_OFF); break;
		case 2 : 
			set_LED(LED_A, FLASH_ON); break;
		default :
			break;
	}
	accept(TOKENIZER_NUMBER);
	led_value = tokenizer_num();
	switch (led_value) {
		case 0 :
			clr_LED(LED_B); break;
		case 1 : 
			set_LED(LED_B, FLASH_OFF); break;
		case 2 : 
			set_LED(LED_B, FLASH_ON); break;
		default :
			break;
	}
	accept(TOKENIZER_NUMBER);
	led_value = tokenizer_num();
	switch (led_value) {
		case 0 :
			clr_LED(LED_C); break;
		case 1 : 
			set_LED(LED_C, FLASH_OFF); break;
		case 2 : 
			set_LED(LED_C, FLASH_ON); break;
		default :
			break;
	}
	accept(TOKENIZER_NUMBER);
	led_value = tokenizer_num();
	switch (led_value) {
		case 0 :
			clr_LED(LED_D); break;
		case 1 : 
			set_LED(LED_D, FLASH_OFF); break;
		case 2 : 
			set_LED(LED_D, FLASH_ON); break;
		default :
			break;
	}
	accept(TOKENIZER_NUMBER);
	accept(TOKENIZER_CR);
}
/*---------------------------------------------------------------------------*/
enum SWITCHES {SW_A=16, SW_B, SW_C, SW_D};

static void sense_statement(void) 
{
uint8_t  channel, value, var;
	
	accept(TOKENIZER_SENSE);
	channel = tokenizer_num();
	accept(TOKENIZER_NUMBER);
	if (channel < DUMMY_LAST_SENSOR) {    // must be one of the analogue channels
		value = get_adc(channel);
	} else {
		switch (channel) {
			case SW_A : value = switch_A; break;
			case SW_B : value = switch_B; break;
			case SW_C : value = switch_C; break;
			case SW_D : value = switch_D; break;
			default   : break;
		}
	}
	var = tokenizer_variable_num();
	accept(TOKENIZER_VARIABLE);
	ubasic_set_variable(var, value);
	accept(TOKENIZER_CR);
}
/*---------------------------------------------------------------------------*/
// motors_statement : switch motors on and off
// ================
//
// format :     motors     left_motor   right_motor
//
//          left_motor, right_motor :: 0=off, 1=forward, 2=backward
//
static void motors_statement(void) 
{
int16_t  temp;
	
	accept(TOKENIZER_MOTORS);
	//
	// 1st parameter defines action on LEFT motor
	//
	temp = expr();
    if (temp == 0) {
		set_motor(LEFT_MOTOR, MOTOR_BRAKE, 0);
	} else if (temp == 1) {
		set_motor(LEFT_MOTOR, MOTOR_FORWARD, gLeft_Speed);
	} else if (temp == 2) {
		set_motor(LEFT_MOTOR, MOTOR_BACKWARD, gLeft_Speed);
	}
	//
	// 2nd parameter defines action on RIGHT motor
	//
	temp = expr();
    if (temp == 0) {  
        set_motor(RIGHT_MOTOR, MOTOR_BRAKE, 0);
    } else if (temp == 1) {	
    	set_motor(RIGHT_MOTOR, MOTOR_FORWARD, gRight_Speed); 
	} else if (temp == 2) {
		set_motor(RIGHT_MOTOR, MOTOR_BACKWARD, gRight_Speed);
	}	

	accept(TOKENIZER_CR);
}
/*---------------------------------------------------------------------------*/
// speed statement : set speed for left and right motors 
//
// format :     speed     left_motor   right_motor
//
//          left_motor, right_motor :: 0 to 100 (% value)
//
// Note that this does not make the motors move, it only sets internal speed values to 
// be used by a 'motors' command.
//
static void speed_statement(void) 
{
	accept(TOKENIZER_SPEED);
	
	gLeft_Speed = expr();        // gLeft_Speed is a global variable
	gRight_Speed = expr();       // gRight_Speed is a global variable

	accept(TOKENIZER_CR);
}
/*---------------------------------------------------------------------------*/
// read_statement : read a system value (e.g. clock, wheel counts, etc)
// ==============
//
//  read  item  variable
//
enum SYSTEM_VALUES {NOW_TIME};

static void read_statement(void) 
{
	uint8_t  item, variable_num;
	uint16_t value;
	
	accept(TOKENIZER_READ);
	item = tokenizer_num();
	accept(TOKENIZER_NUMBER);
	variable_num = tokenizer_variable_num();
	accept(TOKENIZER_VARIABLE);
	accept(TOKENIZER_CR);
	
	switch (item) {
	case NOW_TIME :
		value = tick_count_16;
		break;
	default :
		break;
	}
	ubasic_set_variable(variable_num, value);
}
/*---------------------------------------------------------------------------*/
// text_statement : output string to 2 character 7-seg display
// ==============
//
static void text_statement(void) 
{
	accept(TOKENIZER_TEXT);
	do {
		if(tokenizer_token() == TOKENIZER_STRING) {
			tokenizer_string(string, sizeof(string));
			display_string(string, SEVEN_SEG_AB);
			tokenizer_next();
		} 
	} while((tokenizer_token() != TOKENIZER_CR) && (tokenizer_token() != TOKENIZER_ENDOFINPUT));
	tokenizer_next();
}
/*---------------------------------------------------------------------------*/
// tone_statement : sound note for a set duration of 0.1sec
// ==============
//
static void tone_statement(void) 
{
	uint8_t note;
	uint16_t duration;
	
	accept(TOKENIZER_TONE);
	note = expr();
	duration = expr() * 100;	// convert duration to mS
	set_tone(note, duration);
	accept(TOKENIZER_CR);
}
/*---------------------------------------------------------------------------*/
// move_statement : do a robot distance move
// ==============
//
static void move_statement(void) 
{
	uint8_t  mode;
	int8_t   l_speed, r_speed;
//	motor_t  motor;
	int16_t  distance;
	
	accept(TOKENIZER_MOVE);
	distance = expr();
	if (distance < 0) {
		mode = MOTOR_BACKWARD;
	} else {
		mode = MOTOR_FORWARD;
	}
	distance = (distance * WHEEL_CONSTANT) / 100;    // convert to wheel counts
	
	if (mode == MOTOR_BACKWARD) {
		l_speed = -(int8_t) gLeft_Speed;
	} else {
		l_speed = (int8_t) gLeft_Speed;
	}
	if (mode == MOTOR_BACKWARD) {
		r_speed = -(int8_t) gRight_Speed;
	} else {
		r_speed = (int8_t) gRight_Speed;
	}
//	if (sequence_left_speed > sequence_right_direction) {
//		motor = LEFT_MOTOR;
//	} else {
//		motor = RIGHT_MOTOR;
//	}
//	move_distance(sequence_distance, motor, l_speed, r_speed);
               
	accept(TOKENIZER_CR);
}
/*---------------------------------------------------------------------------*/
static void turn_statement(void) 
{
	accept(TOKENIZER_TURN);
	accept(TOKENIZER_CR);
}
/*---------------------------------------------------------------------------*/
static void cal_statement(void) 
{
	accept(TOKENIZER_CAL);
	calibrate();
	accept(TOKENIZER_CR);
}
/*---------------------------------------------------------------------------*/
static void statement(void) 
{
	static uint8_t token;

	token = tokenizer_token();

	switch (token) {
	case TOKENIZER_PRINT:
		print_statement();
		break;
	case TOKENIZER_IF:
		if_statement();
		break;
	case TOKENIZER_GOTO:
		goto_statement();
		break;
	case TOKENIZER_GOSUB:
		gosub_statement();
		break;
	case TOKENIZER_RETURN:
		return_statement();
		break;
	case TOKENIZER_FOR:
		for_statement();
		break;
	case TOKENIZER_NEXT:
		next_statement();
		break;
	case TOKENIZER_END:
		end_statement();
		break;
	case TOKENIZER_WAIT:
		wait_statement();
		break;
	case TOKENIZER_LEDS:
		leds_statement();
		break;
	case TOKENIZER_MOTORS:
		motors_statement();
		break;
	case TOKENIZER_SPEED:
		speed_statement();
		break;
	case TOKENIZER_SENSE:
		sense_statement();
		break;
	case TOKENIZER_READ:
		read_statement();
		break;
	case TOKENIZER_TEXT:
		text_statement();
		break;
	case TOKENIZER_TONE:
		tone_statement();
		break;
	case TOKENIZER_MOVE:
		move_statement();
		break;
	case TOKENIZER_TURN:
		turn_statement();
		break;
	case TOKENIZER_CAL:
		cal_statement();
		break;
	case TOKENIZER_LET:
		accept(TOKENIZER_LET);
		/* Fall through. */
	case TOKENIZER_VARIABLE:
		let_statement();
		break;
	default:
		// DEBUG_PRINTF("ubasic.c: statement(): not implemented %d\n", token);
		exit(1);
	}
}
/*---------------------------------------------------------------------------*/
static void line_statement(void) 
{
	// DEBUG_PRINTF("----------- Line number %d ---------\n", tokenizer_num());
	/*    current_linenum = tokenizer_num();*/
	accept(TOKENIZER_NUMBER);
	statement();
	return;
}
/*---------------------------------------------------------------------------*/
void ubasic_run(void) 
{
	if (tokenizer_finished()) {
		// DEBUG_PRINTF("uBASIC program finished\n");
		return;
	}
	line_statement();
}
/*---------------------------------------------------------------------------*/
uint8_t ubasic_finished(void) 
{
	return ended || tokenizer_finished();
}
/*---------------------------------------------------------------------------*/
void ubasic_set_variable(uint8_t varnum, int16_t value) {
	if (varnum > 0 && varnum <= MAX_VARNUM) {
		variables[varnum] = value;
	}
}
/*---------------------------------------------------------------------------*/
int16_t ubasic_get_variable(uint8_t varnum) 
{
	if ((varnum > 0) && (varnum <= MAX_VARNUM)) {
		return variables[varnum];
	}
	return 0;
}
/*---------------------------------------------------------------------------*/
