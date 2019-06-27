#include <graphx.h>
#include <fontlibc.h>
#include "calc.h"
#include "style.h"

unsigned char foreground_color = gfx_white;
unsigned char background_color = gfx_black;

/*******************************************************************************
 * Font Stuff
 ******************************************************************************/

char* times_pack_name = "Times";
char* drsans_pack_name = "DrSans";

void font_missing(char* name)
{
    gfx_SetTextFGColor(gfx_red);
    gfx_SetTextBGColor(gfx_black);
    gfx_PrintStringXY("ERROR:", 1, 1);
    gfx_PrintStringXY("Font pack \"", 1, 10);
    gfx_PrintString(name);
    gfx_PrintString("\" missing or corrupted");
        while (!os_GetCSC());
    exit_clean(1);
}

fontlib_font_t* set_font(char* name, uint8_t size, uint8_t weight, uint8_t style_set, uint8_t style_reset, fontlib_load_options_t options)
{
    fontlib_font_t* font;
    font = fontlib_GetFontByStyle(name, size, size, weight, weight > 0 ? weight : 0xFF, style_set, style_reset);
    if (font && fontlib_SetFont(font, options))
        return font;
    font_missing(name);
    return NULL;
}

fontlib_font_t* set_times(uint8_t size, fontlib_load_options_t options)
{
    return set_font(times_pack_name, size, 0, 0, 0, options);
}

fontlib_font_t* set_drsans(uint8_t size, uint8_t weight, fontlib_load_options_t options)
{
    return set_font(drsans_pack_name, size, weight, 0, 0, options);
}

void print_centered(const char *string)
{
    fontlib_SetCursorPosition(fontlib_GetWindowWidth() / 2 + fontlib_GetWindowXMin() - (fontlib_GetStringWidth(string) / 2), fontlib_GetCursorY());
    fontlib_DrawString(string);
}

void print_right(const char *string)
{
    fontlib_SetCursorPosition(fontlib_GetWindowWidth() + fontlib_GetWindowXMin() - fontlib_GetStringWidth(string), fontlib_GetCursorY());
    fontlib_DrawString(string);
}

void fontlib_reverse_colors(void)
{
    fontlib_SetColors(fontlib_GetBackgroundColor(), fontlib_GetForegroundColor());
}

unsigned char print_lines_printed;
char* print_font_pack_name;
unsigned char print_size, print_weight, print_lines;
fontlib_load_options_t print_options;
unsigned char print_window_height;
unsigned int print_cursor_x;
unsigned char print_cursor_y;

void print_disable_pagination(void)
{
    print_lines_printed = 0x80;
}

void print_reset_pagination(void)
{
    print_lines_printed = 0;
}

/**
 * Prints a string, but with word wrap!
 * @param string Text to print
 * @param fake_print Set to true to perform the exact same layout logic, but
 * without actually printing anything, and also return at the first newline.
 * This allows finding word wrap points.
 * @note fake_print DOES care what the current cursor X position is---it uses
 * that to figure out how to deal with words too big to fit into the text
 * window.  Such words will get force-printed starting on their own line.
 * @return Returns a pointer to the last character processed, which will either
 * be '\0', a control code (such as newline), or the first character of the next
 * line of text.
 */
char* print_word_wrap(const char* string, bool fake_print)
{
    char old_stop = fontlib_GetAlternateStopCode();
    unsigned char old_nlo = fontlib_GetNewlineOptions();
    char old_newline = fontlib_GetNewlineCode();
    unsigned int left = fontlib_GetWindowXMin();
    unsigned int width = fontlib_GetWindowWidth();
    unsigned int right = left + width;
    unsigned int str_width;
    unsigned int x = fontlib_GetCursorX();
    unsigned char first_printable = (unsigned char)fontlib_GetFirstPrintableCodePoint();
    unsigned char c;
    unsigned int space_width = fontlib_GetGlyphWidth(' ');
    bool newline = false;
    if (first_printable == '\0')
        first_printable = '\1';
    fontlib_SetNewlineCode('\0');
    fontlib_SetAlternateStopCode(' ');
    fontlib_SetNewlineOptions(FONTLIB_AUTO_CLEAR_TO_EOL | FONTLIB_AUTO_SCROLL | FONTLIB_ENABLE_AUTO_WRAP);
    do
    {
        if (newline)
        {
            newline = false;
            if (print_lines_printed < 0x80)
            {
                print_lines_printed++;
                if (print_lines_printed >= print_lines)
                    break;
            }
            fontlib_Newline();
            x = left;
        }
        /* Check if the next word can fit on the current line */
        str_width = fontlib_GetStringWidth(string);
        if (x + str_width < right)
            if (!fake_print)
                x = fontlib_DrawString(string);
            else
                x += str_width;
        else
        {
            /* If the word is super-long such that it won't fit in the window,
             * then forcibly print it starting on a new line. */
            if (str_width != 0)
            {
                if (str_width > width && x == left)
                    if (!fake_print)    
                        x = fontlib_DrawString(string);
                    else
                    {
                        do
                            x += (str_width = fontlib_GetGlyphWidth(*string++));
                        while (x < right);
                        /*x -= str_width;*/
                        string--;
                        break;
                    }
                else
                {
                    if (fake_print)
                        break;
                    newline = true;
                    continue;
                }
            }
            /* If the width returned was zero, that means either another space
             * is waiting to be printed, which will be handled below; or a
             * control code is next, which also will be handled below.  This can
             * occur, for example, if a control code immediately follows a
             * space. */
        }
        /* FontLibC will kindly tell us exactly where it left off. */
        string = fontlib_GetLastCharacterRead();
        /* Now we need to deal with why the last word was terminated. */
        c = (unsigned char)(*string);
        /* If it's a control code, we either process a newline or give up. */
        if (c < first_printable)
        {
            /*if (c == old_newline && old_newline != '\0')
            {
                string++;
                if (fake_print)
                    break;
                newline = true;
                continue;
            }
            else*/ if (c == ZERO_WIDTH_SPACE)
                string++;
            else if (c == '\t')
            {
                string++;
                x += 32;
                x &= 0xFFFFE0;
                if (!fake_print)
                {
                    fontlib_ClearEOL();
                    fontlib_SetCursorPosition(x, fontlib_GetCursorY());
                }
            }
            else
                break;
        }
        /* If it's a space, we need to process that manually since DrawString
         * won't handle it because we set space as a stop code. */
        if (c == ' ')
        {
            string++;
            /* We do actually need to check if there's space to print the
             * space. */
            if (x + space_width < right)
            {
                if (!fake_print)
                    fontlib_DrawGlyph(' ');
                x += space_width;
            }
            else
            {
                /* If there isn't room, we need to eat the space; it would look
                 * weird to print a space at the start of the next line. 
                 * However, we do not eat ALL the spaces if there's more than
                 * one, just the first one or two. */
                if (*string == ' ') /* Take care of possible second space. */
                    string++;
                if (fake_print)
                    break;
                newline = true;
            }
        }
    } while (true);
    if (!fake_print)
        fontlib_ClearEOL();
    fontlib_SetNewlineCode(old_newline);
    fontlib_SetAlternateStopCode(old_stop);
    fontlib_SetNewlineOptions(old_nlo);
    return string;
}

/*
void print_centered_word_wrap(const char* string)
{
    char* line_end;
    unsigned char c;
    unsigned char first_printable = (unsigned char)fontlib_GetFirstPrintableCodePoint();
    unsigned int line_length;
    unsigned int center = fontlib_GetWindowXMin() + fontlib_GetWindowWidth()/2;
    size_t chars;
    do
    {
        /* We can just ask the word wrapping routine to find breaking points for
         * us. * /
        line_end = print_word_wrap(string, true);
        chars = line_end - string;
        c = (unsigned char)*string;
        /* Don't include trailing space in length * /
        if (chars > 0 && *(string - 1) == ' ')
            chars--;
        line_length = fontlib_GetStringWidthL(string, chars);
        fontlib_SetCursorPosition(center - line_length / 2, fontlib_GetCursorY());
        fontlib_DrawStringL(string, chars);
        if (c == '\0')
            break;
        else if (c < first_printable)
            break;
        string = line_end;
    } while (true);
}*/

void print_configure(char* name, uint8_t size, uint8_t weight, fontlib_load_options_t options)
{
    uint8_t fheight;
    print_font_pack_name = name;
    print_size = size;
    print_weight = weight;
    print_options = options;
    set_font(name, size, weight, 0, 0, options);
    fheight = fontlib_GetCurrentFontHeight();
    /* Make the window one line smaller so there's room for the text entry box
     * at the bottom. */
    print_lines = ((LCD_HEIGHT / fheight) - 1);
    print_window_height = fheight * print_lines;
}

static void print_setup_internal(void)
{
    fontlib_SetWindow(0, 0, LCD_WIDTH, print_window_height);
    set_font(print_font_pack_name, print_size, print_weight, 0, 0, print_options);
    fontlib_SetCursorPosition(print_cursor_x, print_cursor_y);
    fontlib_SetNewlineOptions(FONTLIB_AUTO_CLEAR_TO_EOL | FONTLIB_AUTO_SCROLL | FONTLIB_ENABLE_AUTO_WRAP);
}

static void print_cleanup_internal(void)
{
    print_cursor_x = fontlib_GetCursorX();
    print_cursor_y = fontlib_GetCursorY();
}

void print_clear(void)
{
    print_setup_internal();
    fontlib_ClearWindow();
    /*fontlib_HomeUp();*/
    if (print_lines_printed < 0x80)
        print_reset_pagination();
    fontlib_SetCursorPosition(0, print_window_height - fontlib_GetCurrentFontHeight());
    print_cleanup_internal();
}

static void print_paginate(void)
{
    print_reset_pagination();
    wait_any_key_msg("Press any key to see more. . . .");
}

void print_newline(void)
{
    if (++print_lines_printed >= print_lines)
        print_paginate();
    print_setup_internal();
    fontlib_Newline();
    print_cleanup_internal();
}

void print(const char* string)
{
    char next;
    char* stop;
    unsigned int len;
    bool centering = false;
    do
    {
        if (centering)
        {
            stop = print_word_wrap(string, true);
            len = stop - string;
            len = fontlib_GetStringWidthL(string, len);
            print_cursor_x += (fontlib_GetWindowWidth() - len) / 2;
        }
        print_setup_internal();
        string = print_word_wrap(string, false);
        print_cleanup_internal();
        next = *string;
        if (next == '\0')
            break;
        else if (next == FORMAT_CENTERING)
        {
            string++;
            centering = true;
        }
        else if (next == '\n')
        {
            string++;
            centering = false;
            print_newline();
        }
    } while (true);
}
