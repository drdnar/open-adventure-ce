#include <tice.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <graphx.h>
#include <fontlibc.h>

#include "editor.h"

void fail_fast(char* message)
{
    gfx_SetTextFGColor(gfx_red);
    gfx_SetTextBGColor(gfx_black);
    gfx_PrintStringXY("ERROR:", 1, 1);
    gfx_PrintStringXY(message, 1, 10);
        while (!os_GetCSC());
    gfx_End();
    exit(1);
}

void* malloc_failable(int size)
{
    char* memory = malloc(size);
    if (!memory)
        fail_fast("malloc() failed; out of heap?");
    return memory;
}

/*******************************************************************************
 * History circular buffer
 ******************************************************************************/

#define MAX_HISTORY 32
#define MAX_HISTORY_MASK 31
char** history = NULL;
uint8_t history_next = 0;
bool history_init = false;

void init_history(void)
{
    int i;
    if (history)
        return;
    history = malloc_failable(sizeof(intptr_t) * MAX_HISTORY);
    if (!history)
        exit(0);
    for (i = 0; i < MAX_HISTORY; i++)
        history[i] = NULL;
    history_next = 0;
    history_init = true;
}

void free_history(void)
{
    int i;
    if (!history)
        return;
    for (i = 0; i < MAX_HISTORY; i++)
        if (history[i] != NULL)
            free(history[i]);
    free(history);
    history_init = false;
}

void add_history(char* string)
{
    char* item = history[history_next];
    if (item != NULL)
        free(item);
    history[history_next] = string;
    history_next = (history_next + 1) & MAX_HISTORY_MASK;
    
}


/*******************************************************************************
 * Line editor
 ******************************************************************************/
/*
gfx_HorizLine_NoClip(uint24_t x, uint8_t y, uint24_t length);
gfx_Rectangle_NoClip(uint24_t x, uint8_t y, uint24_t width, uint8_t height);
gfx_FillRectangle_NoClip(uint24_t x, uint8_t y, uint24_t width, uint8_t height);
uint8_t gfx_SetColor(uint8_t index); // returns previous color
*/


/**
 * Internal routine for making sure FontLibC is set up properly.
 */
void editor_fontlib_config(editor_context_t* context)
{
	fontlib_SetWindow(context->base_x, context->base_y, context->width, context->font_height);
    fontlib_SetColors(context->fg_color, context->bg_color);
    fontlib_SetFont(context->font, 0);
    fontlib_SetTransparency(false);
}

/**
 * Redraws the contents of the editor, also erases any blank space.
 */
void editor_redraw_from_cursor(editor_context_t* context)
{
    editor_fontlib_config(context);
    fontlib_SetCursorPosition(context->cursor_x, context->base_y);
    fontlib_DrawString(context->str + context->cursor_index);
    fontlib_ClearEOL();
}

/**
 * Redraws the contents of the editor, also erases any blank space.
 */
void editor_redraw(editor_context_t* context)
{
    editor_fontlib_config(context);
    fontlib_SetCursorPosition(context->base_x, context->base_y);
    fontlib_DrawString(context->str);
    fontlib_ClearEOL();
}

/**
 * Allocates and draws an empty editor session.
 */
editor_context_t* editor_start(uint24_t x_loc, uint8_t y_loc, uint24_t box_width, uint8_t text_max_length, fontlib_font_t* editor_font)
{
    int i;
    editor_context_t* context = malloc_failable(sizeof(context));
    context->base_x = x_loc;
    context->base_y = y_loc;
    context->width = box_width;
    fontlib_SetFont(editor_font, 0);
    context->font = editor_font;
    context->font_height = fontlib_GetCurrentFontHeight();
    context->fg_color = gfx_black;
    context->bg_color = gfx_white;
    context->cursor_glyph = '\1';
    context->cursor_shown = false;
    context->max_length = text_max_length;
    context->cursor_index = 0;
    context->current_length = 0;
    context->str = malloc_failable(1 + text_max_length);
    for (i = 0; i <= text_max_length; i++)
        context->str[i] = '\0';
    editor_redraw(context);
    return context;
}

/**
 * Closes the editor context, freeing memory resources and erasing the box.
 * Note that you also lose your edit buffer!
 */
void editor_close(editor_context_t* context)
{
    gfx_FillRectangle_NoClip(context->base_x, context->base_y, context->width, context->font_height);
    free(context->str);
    free(context);
}

/**
 * Closes the editor context, freeing memory resources and erasing the box.
 * You must free the edit buffer yourself.
 */
char* editor_get_string_close(editor_context_t* context)
{
    char* s = context->str;
    gfx_FillRectangle_NoClip(context->base_x, context->base_y, context->width, context->font_height);
    free(context);
    return s;
}

/**
 * Advances the cursor.
 */
void editor_right(editor_context_t* context)
{
    if (context->cursor_index >= context->current_length)
        return;
    context->cursor_x += fontlib_GetGlyphWidth(context->str[context->cursor_index++]);
}

/**
 * Moves the cursor back one place.
 */
void editor_left(editor_context_t* context)
{
    if (context->cursor_index <= 0)
        return;
    context->cursor_x -= fontlib_GetGlyphWidth(context->str[context->cursor_index--]);
}

/**
 * Sets the cursor position.
 */
void editor_cursor_set(editor_context_t* context, uint8_t index)
{
    context->cursor_index = index;
    if (index == 0)
        context->cursor_x = context->base_x;
    else
        context->cursor_x = context->base_x + fontlib_GetStringWidthL(context->str, index - 1);
}

/**
 * Inserts one character into the edit buffer at the current cursor position.
 */
void editor_insert(editor_context_t* context, char character)
{
    uint8_t i = context->cursor_index;
    char* s = &context->str[i];
    if (context->current_length >= context->max_length)
        return;
    memmove(s, s + 1, context->current_length - i + 1);
    context->str[i] = character;
    context->current_length++;
    editor_redraw_from_cursor(context);
}

/**
 * Inserts an entire string into the edit buffer at the current cursor position.
 * If the string is too big, inserts as much as possible.
 */
void editor_insert_str(editor_context_t* context, char* str)
{
    uint8_t i = context->cursor_index;
    int str_len = strlen(str);
    int new_len = context->current_length + str_len;
    char* s = &context->str[i];
    if (new_len >= context->max_length)
    {
        str_len -= new_len - context->max_length;
        if (str_len <= 0)
            return;
    }
    memmove(s, s + str_len, context->current_length - i + 1);
    memcpy(str, s, str_len);
    context->current_length += str_len;
    editor_redraw_from_cursor(context);
}

/**
 * Deletes n characters from the edit buffer starting at the current cursor position.
 */
void editor_delete(editor_context_t* context, uint8_t n)
{
    uint8_t i = context->cursor_index;
    char* s = &context->str[i];
    if (n >= context->current_length - context->cursor_index)
    {
        *s = '\0';
        context->current_length = context->cursor_index;
        return;
    }
    memmove(s + n, s, context->current_length - i - n + 1);
    context->current_length -= n;
    editor_redraw_from_cursor(context);
}

/**
 * Erases the entire contents of the edit buffer.
 */
void editor_flush(editor_context_t* context)
{
    context->current_length = 0;
    context->cursor_x = context->base_x;
    context->cursor_index = 0;
    context->str[0] = '\0';
    gfx_FillRectangle_NoClip(context->base_x, context->base_y, context->width, context->font_height);
}

/**
 * Display a cursor.
 */
void editor_show_cursor(editor_context_t* context)
{
    if (context->cursor_shown)
        return;
    context->cursor_shown = true;
    editor_fontlib_config(context);
    fontlib_SetCursorPosition(context->base_x, context->base_y);
    fontlib_DrawGlyph(context->cursor_glyph);
}

/**
 * Hides the cursor.
 */
void editor_hide_cursor(editor_context_t* context)
{
    char* sptr;
    char* end;
    unsigned int x_end;
    if (!context->cursor_shown)
        return;
    context->cursor_shown = false;
    editor_fontlib_config(context);
    fontlib_SetCursorPosition(context->base_x, context->base_y);
    /* We're going to hide the cursor by redrawing everything until the drawing
     * cursor is past the right edge of the cursor glyph. */
    x_end = context->cursor_x + fontlib_GetGlyphWidth(context->cursor_glyph);
    sptr = &context->str[context->cursor_index];
    end = &context->str[0] + context->current_length;
    do
        fontlib_DrawGlyph(*sptr++);
    while (fontlib_GetCursorX() <= x_end && sptr < end);
    fontlib_ClearEOL();
}

/**
 * Shows the cursor if it's not shown, hides it if it's visible.
 */
void editor_toggle_cursor(editor_context_t* context)
{
    if (context->cursor_shown)
        editor_hide_cursor(context);
    else
        editor_show_cursor(context);
}

char key_translation_table[3][0x39] =
{
    { /* Unshifted */
    '\0', /* 0 */
    '\0', /* sk_Down             0x01 */
    '\0', /* sk_Left             0x02 */
    '\0', /* sk_Right            0x03 */
    '\0', /* sk_Up               0x04 */
    '\0', '\0', '\0', '\0', /* 5, 6, 7, 8 */
    '\0', /* sk_Enter            0x09 */
    '+', /* sk_Add              0x0A */
    '-', /* sk_Sub              0x0B */
    '*', /* sk_Mul              0x0C */
    '/', /* sk_Div              0x0D */
    '^', /* sk_Power            0x0E */
    '\0', /* sk_Clear            0x0F */
    '\0', /* 10 */
    '\\', /* sk_Chs              0x11 */
    '3', /* sk_3                0x12 */
    '6', /* sk_6                0x13 */
    '9', /* sk_9                0x14 */
    ')', /* sk_RParen           0x15 */
    '\0', /* sk_Tan              0x16 */
    '\0', /* sk_Vars             0x17 */
    '\0', /* 18 */
    '.', /* sk_DecPnt           0x19 */
    '2', /* sk_2                0x1A */
    '5', /* sk_5                0x1B */
    '8', /* sk_8                0x1C */
    '(', /* sk_LParen           0x1D */
    '\0', /* sk_Cos              0x1E */
    '\0', /* sk_Prgm             0x1F */
    '\0', /* sk_Stat             0x20 */
    '0', /* sk_0                0x21 */
    '1', /* sk_1                0x22 */
    '4', /* sk_4                0x23 */
    '7', /* sk_7                0x24 */
    ',', /* sk_Comma            0x25 */
    '\0', /* sk_Sin              0x26 */
    '\0', /* sk_Apps             0x27 */
    '\0', /* sk_GraphVar         0x28 */
    '\0', /* 29 */
    '\0', /* sk_Store            0x2A */
    '\0', /* sk_Ln               0x2B */
    '\0', /* sk_Log              0x2C */
    '\0', /* sk_Square           0x2D */
    '\0', /* sk_Recip            0x2E */
    '\0', /* sk_Math             0x2F */
    '\0', /* sk_Alpha            0x30 */
    '\0', /* sk_Graph            0x31 */
    '\0', /* sk_Trace            0x32 */
    '\0', /* sk_Zoom             0x33 */
    '\0', /* sk_Window           0x34 */
    '\0', /* sk_Yequ             0x35 */
    '\0', /* sk_2nd              0x36 */
    '\0', /* sk_Mode             0x37 */
    '\0' /* sk_Del              0x38 */
    },
    { /* Capital */
    '\0', /* 0 */
    '\0', /* sk_Down             0x01 */
    '\0', /* sk_Left             0x02 */
    '\0', /* sk_Right            0x03 */
    '\0', /* sk_Up               0x04 */
    '\0', '\0', '\0', '\0', /* 5, 6, 7, 8 */
    '\0', /* sk_Enter            0x09 */
    '+', /* sk_Add              0x0A */
    '-', /* sk_Sub              0x0B */
    '*', /* sk_Mul              0x0C */
    '/', /* sk_Div              0x0D */
    '^', /* sk_Power            0x0E */
    '\0', /* sk_Clear            0x0F */
    '\0', /* 10 */
    '\\', /* sk_Chs              0x11 */
    '3', /* sk_3                0x12 */
    '6', /* sk_6                0x13 */
    '9', /* sk_9                0x14 */
    ')', /* sk_RParen           0x15 */
    '\0', /* sk_Tan              0x16 */
    '\0', /* sk_Vars             0x17 */
    '\0', /* 18 */
    '.', /* sk_DecPnt           0x19 */
    '2', /* sk_2                0x1A */
    '5', /* sk_5                0x1B */
    '8', /* sk_8                0x1C */
    '(', /* sk_LParen           0x1D */
    '\0', /* sk_Cos              0x1E */
    '\0', /* sk_Prgm             0x1F */
    '\0', /* sk_Stat             0x20 */
    '0', /* sk_0                0x21 */
    '1', /* sk_1                0x22 */
    '4', /* sk_4                0x23 */
    '7', /* sk_7                0x24 */
    ',', /* sk_Comma            0x25 */
    '\0', /* sk_Sin              0x26 */
    '\0', /* sk_Apps             0x27 */
    '\0', /* sk_GraphVar         0x28 */
    '\0', /* 29 */
    '\0', /* sk_Store            0x2A */
    '\0', /* sk_Ln               0x2B */
    '\0', /* sk_Log              0x2C */
    '\0', /* sk_Square           0x2D */
    '\0', /* sk_Recip            0x2E */
    '\0', /* sk_Math             0x2F */
    '\0', /* sk_Alpha            0x30 */
    '\0', /* sk_Graph            0x31 */
    '\0', /* sk_Trace            0x32 */
    '\0', /* sk_Zoom             0x33 */
    '\0', /* sk_Window           0x34 */
    '\0', /* sk_Yequ             0x35 */
    '\0', /* sk_2nd              0x36 */
    '\0', /* sk_Mode             0x37 */
    '\0' /* sk_Del              0x38 */
    },
    { /* Lowercase */
    '\0', /* 0 */
    '\0', /* sk_Down             0x01 */
    '\0', /* sk_Left             0x02 */
    '\0', /* sk_Right            0x03 */
    '\0', /* sk_Up               0x04 */
    '\0', '\0', '\0', '\0', /* 5, 6, 7, 8 */
    '\0', /* sk_Enter            0x09 */
    '+', /* sk_Add              0x0A */
    '-', /* sk_Sub              0x0B */
    '*', /* sk_Mul              0x0C */
    '/', /* sk_Div              0x0D */
    '^', /* sk_Power            0x0E */
    '\0', /* sk_Clear            0x0F */
    '\0', /* 10 */
    '\\', /* sk_Chs              0x11 */
    '3', /* sk_3                0x12 */
    '6', /* sk_6                0x13 */
    '9', /* sk_9                0x14 */
    ')', /* sk_RParen           0x15 */
    '\0', /* sk_Tan              0x16 */
    '\0', /* sk_Vars             0x17 */
    '\0', /* 18 */
    '.', /* sk_DecPnt           0x19 */
    '2', /* sk_2                0x1A */
    '5', /* sk_5                0x1B */
    '8', /* sk_8                0x1C */
    '(', /* sk_LParen           0x1D */
    '\0', /* sk_Cos              0x1E */
    '\0', /* sk_Prgm             0x1F */
    '\0', /* sk_Stat             0x20 */
    '0', /* sk_0                0x21 */
    '1', /* sk_1                0x22 */
    '4', /* sk_4                0x23 */
    '7', /* sk_7                0x24 */
    ',', /* sk_Comma            0x25 */
    '\0', /* sk_Sin              0x26 */
    '\0', /* sk_Apps             0x27 */
    '\0', /* sk_GraphVar         0x28 */
    '\0', /* 29 */
    '\0', /* sk_Store            0x2A */
    '\0', /* sk_Ln               0x2B */
    '\0', /* sk_Log              0x2C */
    '\0', /* sk_Square           0x2D */
    '\0', /* sk_Recip            0x2E */
    '\0', /* sk_Math             0x2F */
    '\0', /* sk_Alpha            0x30 */
    '\0', /* sk_Graph            0x31 */
    '\0', /* sk_Trace            0x32 */
    '\0', /* sk_Zoom             0x33 */
    '\0', /* sk_Window           0x34 */
    '\0', /* sk_Yequ             0x35 */
    '\0', /* sk_2nd              0x36 */
    '\0', /* sk_Mode             0x37 */
    '\0' /* sk_Del              0x38 */
    }
};



/**
 * Translates a keycode into a glyph.
 */
char editor_translate_key(char key, unsigned char shift)
{
    if (shift <= shift_lower_alpha)
        return key_translation_table[shift][key];
    return '\0';
}


char* get_string_(uint24_t x_loc, uint8_t y_loc, uint24_t box_width, uint8_t text_max_length, fontlib_font_t* editor_font)
{
    editor_context_t* context;
    unsigned char seconds;
    unsigned char key;
    context = editor_start(x_loc, y_loc, box_width, text_max_length, editor_font);
    
    seconds = rtc_Seconds;
    
    
    if (rtc_Seconds > seconds)
    {
        seconds = rtc_Seconds;
        editor_toggle_cursor(context);
    }
    return NULL;
}

