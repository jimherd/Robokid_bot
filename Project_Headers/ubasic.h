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
#ifndef __UBASIC_H__
#define __UBASIC_H__

void ubasic_init(char *program);

static void accept(uint8_t token);
static int16_t varfactor(void);
static int16_t factor(void);
static int16_t term(void);
static int16_t expr(void);
static int16_t relation(void);

static void print_statement(void);
static void if_statement(void);
static void let_statement(void);
static void for_statement(void);
static void while_statement(void);
static void rem_statement(void);
static void end_statement(void);
static void wait_statement(void);
void leds_statement(void);
static void sense_statement(void);
static void motors_statement(void);
static void speed_statement(void);
static void read_statement(void);
static void display_statement(void);
static void note_statement(void);
static void move_statement(void);
static void turn_statement(void);
static void cal_statement(void);
static void statement(void);

void ubasic_run(void);
uint8_t run_ubasicp_program(char *program);
uint8_t ubasic_finished(void);
void ubasic_set_variable(uint8_t varum, int16_t value);
int16_t ubasic_get_variable(uint8_t varnum);




#endif /* __UBASIC_H__ */
