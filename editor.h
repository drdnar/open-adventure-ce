#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <fontlibc.h>

void init_history(void);
void free_history(void);
void add_history(char* string);
char* get_history_item(unsigned char n);

typedef struct
{
    /* Physical layout */
    unsigned int base_x;
    unsigned char base_y;
    unsigned int width;
    unsigned char font_height;
    /* Style */
    fontlib_font_t* font;
    unsigned char fg_color;
    unsigned char bg_color;
    char cursor_glyph;
    bool cursor_shown;
    /* Edit buffer */
    char* str;
    unsigned char max_length;
    unsigned char current_length;
    unsigned char cursor_index;
    unsigned int cursor_x;
} editor_context_t;

typedef enum
{
    shift_none,
    shift_alpha,
    shift_lower_alpha
} edit_shift_options_t;

void editor_redraw_from_cursor(editor_context_t* context);
void editor_redraw(editor_context_t* context);
void editor_show_cursor(editor_context_t* context);
void editor_hide_cursor(editor_context_t* context);
void editor_toggle_cursor(editor_context_t* context);
editor_context_t* editor_start(unsigned int x_loc, unsigned char y_loc, unsigned int box_width, unsigned char text_max_length, fontlib_font_t* editor_font);
void editor_close(editor_context_t* context);
char* editor_get_string_close(editor_context_t* context);
char* get_string(unsigned int x_loc, unsigned char y_loc, unsigned int box_width, unsigned char text_max_length, fontlib_font_t* editor_font);
void editor_right(editor_context_t* context);
void editor_left(editor_context_t* context);
void editor_cursor_set(editor_context_t* context, unsigned char index);
void editor_insert(editor_context_t* context, char character);
/*void editor_insert_str(editor_context_t* context, char* str);*/
void editor_set_str(editor_context_t* context, char* str);
void editor_delete(editor_context_t* context, unsigned char n);
void editor_flush(editor_context_t* context);
char editor_translate_key(char key, unsigned char shift);

