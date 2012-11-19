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

#include "global.h"

//#define DEBUG 0

//#if DEBUG
//#define  DEBUG_PRINTF(const char *format, ...)   printf(__VA_ARGS__)
//#else
//#define // DEBUG_PRINTF(...)   
//#endif

char *ptr, *nextptr;

#define MAX_NUMLEN 6

struct keyword_token {
  char 		*keyword;
  uint8_t 	token;
//  void		(*token_function)(void);
};

static uint8_t current_token = TOKENIZER_ERROR;

static const struct keyword_token keywords[] = {

  {"if", 		TOKENIZER_IF},
  {"else", 		TOKENIZER_ELSE},
  {"for", 		TOKENIZER_FOR},
  {"to", 		TOKENIZER_TO},
//  {"return",	TOKENIZER_RETURN},
  {"end", 		TOKENIZER_END},
  
  {"while", 	TOKENIZER_WHILE}, 	// brl4
  {"#", 		TOKENIZER_REM},     // brl4
  
  {"wait", 		TOKENIZER_WAIT},	// jth
  {"wa", 		TOKENIZER_WAIT},	// jth
  {"leds", 		TOKENIZER_LEDS},	// jth
  {"le", 		TOKENIZER_LEDS},	// jth
  {"motors",	TOKENIZER_MOTORS},	// jth
  {"mo",		TOKENIZER_MOTORS},	// jth
  {"speed", 	TOKENIZER_SPEED},	// jth
  {"sp", 		TOKENIZER_SPEED},	// jth
  {"display",	TOKENIZER_DISPLAY},	// jth
  {"di",	    TOKENIZER_DISPLAY},	// jth 
  {"print", 	TOKENIZER_PRINT},	// jth 
  {"pr", 		TOKENIZER_PRINT},	// jth 
  {"sense", 	TOKENIZER_SENSE},	// jth 
  {"se", 		TOKENIZER_SENSE}, 	// jth 
  {"read", 		TOKENIZER_READ},	// jth 
  {"re", 		TOKENIZER_READ},  	// jth 
  {"note", 		TOKENIZER_NOTE},	// jth 
  {"no", 		TOKENIZER_NOTE},    // jth 
  {NULL, 		TOKENIZER_ERROR}
};

/*---------------------------------------------------------------------------*/
static uint8_t singlechar(void) 
{
    if (*ptr == ';') {  //(*ptr == '\n')
		return TOKENIZER_CR;
	} else if (*ptr == ',') {
		return TOKENIZER_COMMA;
	} else if (*ptr == '+') {
		return TOKENIZER_PLUS;
	} else if (*ptr == '-') {
		return TOKENIZER_MINUS;
	} else if (*ptr == '&') {
		return TOKENIZER_AND;
	} else if (*ptr == '|') {
		return TOKENIZER_OR;
	} else if (*ptr == '*') {
		return TOKENIZER_ASTR;
	} else if (*ptr == '/') {
		return TOKENIZER_SLASH;
	} else if (*ptr == '%') {
		return TOKENIZER_MOD;
	} else if (*ptr == '(') {
		return TOKENIZER_LEFTPAREN;
	} else if (*ptr == ')') {
		return TOKENIZER_RIGHTPAREN;
    } else if (*ptr == '{') {
        return TOKENIZER_LEFTBRACK;
    } else if (*ptr == '}') {
        return TOKENIZER_RIGHTBRACK;
	} else if (*ptr == '<') {
		return TOKENIZER_LT;
	} else if (*ptr == '>') {
		return TOKENIZER_GT;
	} else if (*ptr == '=') {
		return TOKENIZER_EQ;
	}
	return 0;
}
/*---------------------------------------------------------------------------*/
static uint8_t get_next_token(void) 
{
	struct keyword_token const *kt;
	uint8_t i;

	// DEBUG_PRINTF("get_next_token(): '%s'\n", ptr);

	if (*ptr == 0) {
		return TOKENIZER_ENDOFINPUT;
	}

	if (isdigit(*ptr)) {
		for (i = 0; i < MAX_NUMLEN; ++i) {
			if(!isdigit(ptr[i])) {
				if(i > 0) {
					nextptr = ptr + i;
					return TOKENIZER_NUMBER;
				} else {
					// DEBUG_PRINTF("get_next_token: error due to too short number\n");
					return TOKENIZER_ERROR;
				}
			}
			if(!isdigit(ptr[i])) {
				// DEBUG_PRINTF("get_next_token: error due to malformed number\n");
				return TOKENIZER_ERROR;
			}
		}
		// DEBUG_PRINTF("get_next_token: error due to too long number\n");
		return TOKENIZER_ERROR;
	} else if (singlechar()) {
		nextptr = ptr + 1;
		return singlechar();
    } else if (*ptr == '\'') {
		nextptr = ptr;
		do {
			++nextptr;
        } while (*nextptr != '\'');
		++nextptr;
		return TOKENIZER_STRING;
	} else {
		for(kt = keywords; kt->keyword != NULL; ++kt) {
			if(strncmp(ptr, kt->keyword, strlen(kt->keyword)) == 0) {
				nextptr = ptr + strlen(kt->keyword);
				return kt->token;
			}
		}
	}
	if(*ptr >= 'a' && *ptr <= 'z') {
		nextptr = ptr + 1;
		return TOKENIZER_VARIABLE;
	}
	return TOKENIZER_ERROR;
}
/*---------------------------------------------------------------------------*/
// CHANGED -- BRL4 -- failed if first char was not a token
void tokenizer_init(char *program) 
{
	ptr = program;
    nextptr = program;
    tokenizer_next();
    //current_token =  get_next_token();
}
/*---------------------------------------------------------------------------*/
uint8_t tokenizer_token(void) 
{
	return current_token;
}
/*---------------------------------------------------------------------------*/
void tokenizer_next(void) 
{

	if (tokenizer_finished()) {
		return;
	}
	// DEBUG_PRINTF("tokenizer_next: %p\n", nextptr);
	ptr = nextptr;
    // CHANGED brl4 -- ignore tabs, newlines
    while (*ptr == ' ' | *ptr == '\t' | *ptr == '\n' | *ptr == '\r') {
		++ptr;
	}
	current_token = get_next_token();
	// DEBUG_PRINTF("tokenizer_next: '%s' %d\n", ptr, current_token);
	return;
}
/*---------------------------------------------------------------------------*/
uint8_t tokenizer_num(void) 
{
	return atoi(ptr);
}

/*---------------------------------------------------------------------------*/
void tokenizer_string(char *dest, uint8_t len) 
{
	char *string_end;
	uint8_t string_len;

	if (tokenizer_token() != TOKENIZER_STRING) {
		return;
	}
    string_end = strchr(ptr + 1, '\'');
	if (string_end == NULL) {
		return;
	}
	string_len = string_end - ptr - 1;
	if (len < string_len) {
		string_len = len;
	}
	memcpy(dest, ptr + 1, string_len);
	dest[string_len] = 0;
}

/*---------------------------------------------------------------------------*/
void tokenizer_error_print(void) 
{
	// DEBUG_PRINTF("tokenizer_error_print: '%s'\n", ptr);
}

/*---------------------------------------------------------------------------*/
uint8_t tokenizer_finished(void) 
{
	return *ptr == 0 || current_token == TOKENIZER_ENDOFINPUT;
}

/*---------------------------------------------------------------------------*/
uint8_t tokenizer_variable_num(void) 
{
	return *ptr - 'a';
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
char* tokenizer_ptr(void)
{
    return ptr;
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
void tokenizer_ptr_set(char *p)
{
    ptr = p;
    current_token = get_next_token();
}

/*---------------------------------------------------------------------------*/
