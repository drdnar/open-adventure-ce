#ifndef STYLE_H
#define STYLE_H
#include <graphx.h>
#include <fileioc.h>
#include <fontlibc.h>

#define FORMAT_CENTERING '\1'
#define ZERO_WIDTH_SPACE '\6'

extern char* times_pack_name;
extern char* drsans_pack_name;
extern unsigned char foreground_color;
extern unsigned char background_color;
extern unsigned char print_lines_printed;

void gfx_resume_render_splash(void);
void font_missing(char* name);
fontlib_font_t* set_font(char* name, uint8_t size, uint8_t weight, uint8_t style_set, uint8_t style_reset, fontlib_load_options_t options);
fontlib_font_t* set_times(uint8_t size, fontlib_load_options_t options);
fontlib_font_t* set_drsans(uint8_t size, uint8_t weight, fontlib_load_options_t options);
void draw_compressed(int message);
void print_centered(const char* string);
void print_centered_compressed(int message);
void print_right(int message);
void fontlib_reverse_colors(void);
void print_disable_pagination(void);
void print_reset_pagination(void);
char* print_word_wrap(const char* string, bool fake_print);
void print_centered_word_wrap(const char* string);
void print_configure(char* name, uint8_t size, uint8_t weight, fontlib_load_options_t options);
void print_clear(void);
void print_newline(void);
void print_compressed(int message);
void print(const char* string);


#endif
