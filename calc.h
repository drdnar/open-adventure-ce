#ifndef CALC_H
#define CALC_H
#include <tice.h>

void exit_clean(int n);
void exit_apd(void);
void exit_main(int n);
void exit_fail(char* message);
void* malloc_safe(int size);

sk_key_t wait_any_key();

unsigned long time(unsigned char* ignored);
char* readline_len(char* prompt, unsigned char max_len);
char* readline(char* prompt);

#define APD_DIM_TIME 300
#define APD_QUIT_TIME 120

#endif
