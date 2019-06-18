/*
 * Actions for the duneon-running code.
 * 
 * Zilog's C compiler is a bit . . . special, so this file is broken up into
 * smaller units.
 *
 * Copyright (c) 1977, 2005 by Will Crowther and Don Woods
 * Copyright (c) 2017 by Eric S. Raymond
 * SPDX-License-Identifier: BSD-2-clause
 */

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "advent.h"
#include "dungeon.h"
#ifdef CALCULATOR
#include "calc.h"
#endif
#include <stdint.h>

phase_codes_t fill(verb_t, obj_t);
phase_codes_t attack(command_t command);
phase_codes_t bigwords(vocab_t id);
void blast(void);
phase_codes_t vbreak(verb_t verb, obj_t obj);
phase_codes_t brief(void);
phase_codes_t vcarry(verb_t verb, obj_t obj);
int chain(verb_t verb);
phase_codes_t discard(verb_t verb, obj_t obj);
phase_codes_t drink(verb_t verb, obj_t obj);
phase_codes_t eat(verb_t verb, obj_t obj);
phase_codes_t extinguish(verb_t verb, obj_t obj);
phase_codes_t feed(verb_t verb, obj_t obj);
phase_codes_t fill(verb_t verb, obj_t obj);
phase_codes_t find(verb_t verb, obj_t obj);
phase_codes_t fly(verb_t verb, obj_t obj);
phase_codes_t inven(void);
phase_codes_t light(verb_t verb, obj_t obj);
phase_codes_t listen(void);
phase_codes_t lock(verb_t verb, obj_t obj);
phase_codes_t pour(verb_t verb, obj_t obj);
phase_codes_t quit(void);
phase_codes_t read(command_t command);
phase_codes_t reservoir(void);
phase_codes_t rub(verb_t verb, obj_t obj);
phase_codes_t say(command_t command);
phase_codes_t throw_support(vocab_t spk);
phase_codes_t throw (command_t command);
phase_codes_t wake(verb_t verb, obj_t obj);
phase_codes_t seed(verb_t verb, const char *arg);
phase_codes_t waste(verb_t verb, turn_t turns);
phase_codes_t wave(verb_t verb, obj_t obj);
phase_codes_t action(command_t command);
