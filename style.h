#ifndef STYLE_H
#define STYLE_H
#include <graphx.h>
#include <fileioc.h>
#include <fontlibc.h>

extern char* times_pack_name;
extern char* drsans_pack_name;
extern unsigned char foreground_color;
extern unsigned char background_color;

void font_missing(char* name);
fontlib_font_t* set_font(char* name, uint8_t size, uint8_t weight, uint8_t style_set, uint8_t style_reset, fontlib_load_options_t options);
fontlib_font_t* set_times(uint8_t size, fontlib_load_options_t options);
fontlib_font_t* set_drsans(uint8_t size, uint8_t weight, fontlib_load_options_t options);
void print_centered(const char *string);
void print_right(const char *string);
void fontlib_reverse_colors(void);
char* print_word_wrap(const char* string, bool fake_print);
void print_centered_word_wrap(const char* string);
void print_configure(char* name, uint8_t size, uint8_t weight, fontlib_load_options_t options);
void print_clear(void);
void print_newline(void);
void print(const char* string);


#endif
