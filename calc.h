#ifndef CALC_H
#define CALC_H
#include <tice.h>

void exit_clean(int n);
void exit_apd(void);
void exit_main(int n);
void exit_fail(char* message);
void* malloc_safe(int size);

sk_key_t wait_any_key();
sk_key_t wait_any_key_msg(char* msg);

unsigned long time(unsigned char* ignored);

char* readline_len(char* prompt, unsigned char max_len, char* default_text);
char* readline(char* prompt);

bool valid_name(const char* filename);

#define APD_DIM_TIME 90
#define APD_QUIT_TIME 210

#define COMMAND_STRING(A_STRING_DEFINE_ON_COMMAND_LINE) ZILOGS_COMPILER_DOESNT_SUPPORT_PASSING_DIRECTLY(A_STRING_DEFINE_ON_COMMAND_LINE)
#define ZILOGS_COMPILER_DOESNT_SUPPORT_PASSING_DIRECTLY(A_STRING_DEFINE_ON_COMMAND_LINE) #A_STRING_DEFINE_ON_COMMAND_LINE
#define VERSION_STRING COMMAND_STRING(VERSION)

#endif
