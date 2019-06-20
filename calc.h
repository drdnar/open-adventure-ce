#ifndef CALC_H
#define CALC_H
/*#include "dehuffman.h"*/
#include <tice.h>
void exit_clean(int n);
void exit_main(int n);
void exit_fail(char* message);
void* malloc_safe(int size);

sk_key_t wait_any_key();

unsigned long time(unsigned char* ignored);
char* readline_len(char* prompt, unsigned char max_len);
char* readline(char* prompt);
#endif
