/*
 * String input and command history buffer routines.
 *
 * Copyright (c) 2019 Dr. D'nar
 * SPDX-License-Identifier: BSD-2-clause
 */

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
#include "calc.h"
#include "style.h"
#include "ez80.h"


/*******************************************************************************
 * History circular buffer
 ******************************************************************************/

#define MAX_HISTORY 256
#define MAX_HISTORY_MASK 255
char** history = NULL;
unsigned char history_next = 0;

/**
 * Initializes the command history buffer.
 * Does nothing if a history buffer already exists.
 */
void init_history(void)
{
    int i;
    if (history)
        return;
    history = malloc_safe(sizeof(char*) * MAX_HISTORY);
    for (i = 0; i < MAX_HISTORY; i++)
        history[i] = NULL;
    history_next = 0;
}

/**
 * Frees the command history buffer.  All history is deleted.
 */
void free_history(void)
{
    int i;
    if (!history)
        return;
    for (i = 0; i < MAX_HISTORY; i++)
        //if (history[i] != NULL)
            free(history[i]);
    free(history);
    history = NULL;
}

/**
 * Adds the given string to the command history.
 * If the last string added is identical to the one passed, then it is not added
 * again.
 */
void add_history(char* string)
{
    char* item;
    if (!history)
        return;
    if (!strcmp(get_history_item(1), string))
        return;
    free(history[history_next]);
    item = malloc_safe(strlen(string) + 1);
    strcpy(item, string);
    history[history_next] = item;
    /*history_next = (history_next + 1) & MAX_HISTORY_MASK;*/
    history_next++;
}

/**
 * Gets an item from the history buffer.
 * @param n How far back to go.  1 means the most recent item.
 * 0 will either return NULL or the LAST item depending on whether the history
 * buffer has filled up.
 */
char* get_history_item(unsigned char n)
{
    /* This saves six bytes . . . eh, why not? */
    unsigned char i;
    if (!history)
        return NULL;
    i = history_next - n;
    /*return history[(history_next + MAX_HISTORY - n) & MAX_HISTORY_MASK];*/
    return history[i];
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
    editor_context_t* context = malloc_safe(sizeof(editor_context_t));
    context->base_x = x_loc;
    context->base_y = y_loc;
    context->width = box_width;
    fontlib_SetFont(editor_font, 0);
    context->font = editor_font;
    context->font_height = fontlib_GetCurrentFontHeight();
    context->fg_color = foreground_color;
    context->bg_color = background_color;
    context->cursor_glyph = '\2';
    context->cursor_shown = false;
    context->max_length = text_max_length;
    context->cursor_index = 0;
    context->cursor_x = x_loc;
    context->current_length = 0;
    context->str = malloc_safe(1 + text_max_length);
    memset(context->str, '\0', 1 + text_max_length);
    editor_redraw(context);
    return context;
}

/**
 * Closes the editor context, freeing memory resources and erasing the box.
 * Note that you also lose your edit buffer!
 */
void editor_close(editor_context_t* context)
{
    editor_fontlib_config(context);
    fontlib_SetCursorPosition(context->base_x, context->base_y);
    fontlib_ClearEOL();
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
    editor_fontlib_config(context);
    fontlib_SetCursorPosition(context->base_x, context->base_y);
    fontlib_ClearEOL();
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
    context->cursor_x -= fontlib_GetGlyphWidth(context->str[--context->cursor_index]);
}

/**
 * Moves the cursor back to the start of the edit buffer.
 */
void editor_home(editor_context_t* context)
{
    editor_cursor_set(context, 0);
}

/**
 * Moves the cursor to the end of the edit buffer.
 */
void editor_end(editor_context_t* context)
{
    editor_cursor_set(context, context->current_length);
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
        context->cursor_x = context->base_x + fontlib_GetStringWidthL(context->str, index);
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
    memmove(s + 1, s, context->current_length - i + 1);
    context->str[i] = character;
    context->current_length++;
    editor_redraw_from_cursor(context);
}

/**
 * Inserts an entire string into the edit buffer at the current cursor position.
 * If the string is too big, inserts as much as possible.
 * 
 * Also this method is broken.
 *
void editor_insert_str(editor_context_t* context, char* str)
{
    uint8_t i = context->cursor_index;
    int str_len = strlen(str);
    int new_len = context->current_length + str_len;
    char* s = context->str + i;
    if (context->current_length == context->max_length)
        return;
    if (new_len >= context->max_length)
        str_len -= new_len - context->max_length;
    memmove(s + str_len, s, context->current_length - i + 1);
    memcpy(str, s, str_len);
    context->current_length += str_len;
    editor_redraw_from_cursor(context);
}*/


/**
 * Completely replaces the contents of the edit buffer with str.
 */
void editor_set_str(editor_context_t* context, char* str)
{
    size_t len = strlen(str);
    if (len > context->max_length)
        len = context->max_length;
    memcpy(context->str, str, len);
    context->str[len + 1] = '\0';
    context->current_length = (unsigned char)len;
    editor_end(context);
}

/**
 * Deletes n characters from the edit buffer starting at the current cursor position.
 */
void editor_delete(editor_context_t* context, uint8_t n)
{
    uint8_t i = context->cursor_index;
    char* s = context->str + i;
    if (n >= context->current_length - context->cursor_index)
    {
        *s = '\0';
        context->current_length = context->cursor_index;
        return;
    }
    memmove(s, s + n, context->current_length - i - n + 1);
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
    editor_fontlib_config(context);
    fontlib_SetCursorPosition(context->base_x, context->base_y);
    fontlib_ClearEOL();
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
    fontlib_SetCursorPosition(context->cursor_x, context->base_y);
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
    fontlib_SetCursorPosition(context->cursor_x, context->base_y);
    /* We're going to hide the cursor by redrawing everything until the drawing
     * cursor is past the right edge of the cursor glyph. */
    x_end = context->cursor_x + fontlib_GetGlyphWidth(context->cursor_glyph);
    sptr = context->str + context->cursor_index;
    end = context->str + context->current_length;
    do
        fontlib_DrawGlyph(*(sptr++));
    while (fontlib_GetCursorX() <= x_end && sptr < end);
    if (sptr >= end)
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
    '\"', /* sk_Add              0x0A */
    'W', /* sk_Sub              0x0B */
    'R', /* sk_Mul              0x0C */
    'M', /* sk_Div              0x0D */
    'H', /* sk_Power            0x0E */
    '\0', /* sk_Clear            0x0F */
    '\0', /* 10 */
    '?', /* sk_Chs              0x11 */
    '\264', /* sk_3                0x12 */
    'V', /* sk_6                0x13 */
    'Q', /* sk_9                0x14 */
    'L', /* sk_RParen           0x15 */
    'G', /* sk_Tan              0x16 */
    '\0', /* sk_Vars             0x17 */
    '\0', /* 18 */
    ':', /* sk_DecPnt           0x19 */
    'Z', /* sk_2                0x1A */
    'U', /* sk_5                0x1B */
    'P', /* sk_8                0x1C */
    'K', /* sk_LParen           0x1D */
    'F', /* sk_Cos              0x1E */
    'C', /* sk_Prgm             0x1F */
    '\0', /* sk_Stat             0x20 */
    ' ', /* sk_0                0x21 */
    'Y', /* sk_1                0x22 */
    'T', /* sk_4                0x23 */
    'O', /* sk_7                0x24 */
    'J', /* sk_Comma            0x25 */
    'E', /* sk_Sin              0x26 */
    'B', /* sk_Apps             0x27 */
    '\0', /* sk_GraphVar         0x28 */
    '\0', /* 29 */
    'X', /* sk_Store            0x2A */
    'S', /* sk_Ln               0x2B */
    'N', /* sk_Log              0x2C */
    'I', /* sk_Square           0x2D */
    'D', /* sk_Recip            0x2E */
    'A', /* sk_Math             0x2F */
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
    '\'', /* sk_Add              0x0A */
    'w', /* sk_Sub              0x0B */
    'r', /* sk_Mul              0x0C */
    'm', /* sk_Div              0x0D */
    'h', /* sk_Power            0x0E */
    '\0', /* sk_Clear            0x0F */
    '\0', /* 10 */
    '_', /* sk_Chs              0x11 */
    '\264', /* sk_3                0x12 */
    'v', /* sk_6                0x13 */
    'q', /* sk_9                0x14 */
    'l', /* sk_RParen           0x15 */
    'g', /* sk_Tan              0x16 */
    '\0', /* sk_Vars             0x17 */
    '\0', /* 18 */
    ';', /* sk_DecPnt           0x19 */
    'z', /* sk_2                0x1A */
    'u', /* sk_5                0x1B */
    'p', /* sk_8                0x1C */
    'k', /* sk_LParen           0x1D */
    'f', /* sk_Cos              0x1E */
    'c', /* sk_Prgm             0x1F */
    '\0', /* sk_Stat             0x20 */
    ' ', /* sk_0                0x21 */
    'y', /* sk_1                0x22 */
    't', /* sk_4                0x23 */
    'o', /* sk_7                0x24 */
    'j', /* sk_Comma            0x25 */
    'e', /* sk_Sin              0x26 */
    'b', /* sk_Apps             0x27 */
    '\0', /* sk_GraphVar         0x28 */
    '\0', /* 29 */
    'x', /* sk_Store            0x2A */
    's', /* sk_Ln               0x2B */
    'n', /* sk_Log              0x2C */
    'i', /* sk_Square           0x2D */
    'd', /* sk_Recip            0x2E */
    'a', /* sk_Math             0x2F */
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

char cursors[] = {'\4', '\6', '\7'};

/* These are all 32-bit registers, but we don't need all 32 bits, and 32-bit
 * code sucks on the eZ80, so as a performance and code-size hack we're going
 * to treat these as single-byte registers as much as possible.
 * Look in <tice.h> for more information. */
#define timer_control_a (*(volatile unsigned char*)0xF20030)
#define timer_control_b (*(volatile unsigned char*)0xF20031)
#define timer_1_value (*(volatile unsigned char*)0xF20001)
#define timer_1_value_b (*(volatile unsigned char*)0xF20002)
#define timer_1_match_1     (*(unsigned char*)0xF2000A)
#define timer_1_match_2     (*(unsigned char*)0xF2000E)
/**
 * A string input routine.  Gives the user access to their command history.
 * @note Despite its size, this routine still can't handle multiline text, nor
 * text wider than the text window; there is no scrolling logic.
 * @param x_loc Location of the editor
 * @param y_loc Location of the editor
 * @param box_width Width of the editor window; height is implied by font choice
 * @param text_max_length Maximum number of characters to accept
 * @param editor_font Font to use in the editor
 * @param default_text Places some text into the editor before letting the user
 * start typing.
 * @return A pointer to the text the user entered.  You must free() this string
 * yourself.
 */
char* get_string(uint24_t x_loc, uint8_t y_loc, uint24_t box_width, uint8_t text_max_length, fontlib_font_t* editor_font, char* default_text)
{
    editor_context_t* context;
    bool not_done = true;
    unsigned char temp;
    sk_key_t key;
    unsigned int apd_timer;
    bool dimmed = false;
    unsigned char shift = 2;
    context = editor_start(x_loc, y_loc, box_width, text_max_length, editor_font);
    context->cursor_glyph = cursors[shift];
    if (default_text)
    {
        editor_set_str(context, default_text);
        editor_redraw(context);
        default_text = NULL;
    }
    
    /* Set timer 1 to count up */
    timer_control_b = timer_control_b | 2;
    /* 0xF8 = Clear interrupt enable and disable timer 1
     * 2 = Set timer 1 to use the RTC crystal (32768 Hz) */
    timer_control_a = timer_control_a & 0xF8 | 2;
    /* Make sure match registers are disabled */
    timer_1_match_1 = 0xFF;
    timer_1_match_2 = 0xFF;
    do
    {
        clear_on_key();
        /* Reset timer 1 to 0.
         * Recall that timer 1 has already been turned off, so the non-
         * atomic writes won't cause a problem. */
        timer_1_Counter = 0;
        apd_timer = get_rtc_seconds_plus(APD_DIM_TIME);
        /* Enable timer 1 */
        timer_control_a = timer_control_a | 1;
        editor_show_cursor(context);
        do
        {
            /* Prevent battery killing */
            if (get_rtc_seconds() == apd_timer)
            {
                if (!dimmed)
                {
                    dimmed = true;
                    lcd_dim();
                    apd_timer = get_rtc_seconds_plus(APD_QUIT_TIME);
                }
                else
                    exit_apd();
            }
            /* Check value of timer 1.  We need only check one byte of the whole
             * 32-bit register. */
            if (timer_1_value >= 64)
            {
                /* Turn timer off, zero it, and turn it back on */
                timer_control_a = timer_control_a & 0xFA;
                timer_1_Counter = 0;
                timer_control_a = timer_control_a | 1;
                /* Flash cursor */
                editor_toggle_cursor(context);
            }
            key = get_csc();
        } while (!key);
        dimmed = false;
        lcd_bright();
        /* Turn timer off */
        timer_control_a = timer_control_a & 0xFA;
        editor_hide_cursor(context);
        switch (key)
        {
            case sk_Clear:
                editor_flush(context);
                break;
            case sk_Del:
                editor_left(context);
                editor_delete(context, 1);
                break;
            case sk_Left:
                editor_left(context);
                break;
            case sk_Right:
                editor_right(context);
                break;
            case sk_Enter:
                not_done = false;
                break;
            case sk_Alpha:
                shift = shift + 1;
                if (shift > 2)
                    shift = 0;
                context->cursor_glyph = cursors[shift];
                break;
            case sk_Up:
                temp = 1;
                if (get_history_item(temp) == NULL)
                    break;
                do
                {
                    fontlib_SetColors(context->bg_color, context->fg_color);
                    fontlib_SetCursorPosition(context->base_x, context->base_y);
                    fontlib_DrawString(get_history_item(temp));
                    fontlib_SetBackgroundColor(context->bg_color);
                    fontlib_ClearEOL();
                    do
                        key = os_GetCSC();
                    while (!key);
                    switch (key)
                    {
                        case sk_Up:
                            if (get_history_item(temp + 1) != NULL)
                                temp++;
                            break;
                        case sk_Down:
                            if (get_history_item(temp - 1) != NULL)
                                temp--;
                            break;
                        case sk_Enter:
                            editor_flush(context);
                            editor_set_str(context, get_history_item(temp));
                            not_done = false;
                            break;
                        case sk_Clear:
                        case sk_Del:
                        case sk_Mode:
                            not_done = false;
                            break;
                    }
                } while (not_done);
                editor_redraw(context);
                not_done = true;
                break;
            default:
                key = editor_translate_key(key, shift);
                if (key != '\0')
                {
                    editor_insert(context, key);
                    editor_right(context);
                }
        }
        
    } while (not_done);
    return editor_get_string_close(context);
}

