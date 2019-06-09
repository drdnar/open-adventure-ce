#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tice.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <graphx.h>
#include <fileioc.h>
#include <fontlibc.h>

/*#include "dungeon.h"*/
#include "calc.h"
#include "editor.h"
/*#include "advent.h"*/


/*******************************************************************************
 * time.h replacement
 ******************************************************************************/

/**
 * This does not actually return a Unix time, but it is 32-bits and
 * monotonically increasing.  Unless you have a RAM reset.
 */
unsigned long time(unsigned char* ignored)
{
    return atomic_load_increasing_32((volatile uint32_t*)0xF30044);
}
/*
unsigned char seconds_from_time(void* time)
{
	unsigned char* s = time;
    return *s & 0x3F;
}

unsigned char minutes_from_time(void* time)
{
	unsigned short* t = time;
    return (*t >> 6) & 0x3F;
}

unsigned char hours_from_time(void* time)
{
	unsigned char* t = time;
    t++;
    unsigned char h = *t >> 4;
    t++;
    return h | (*t & 1) << 4;
}

unsigned short days_from_time(void* time)
{
    unsigned short* t = time;
    t++;
    return *t >> 1;
}*/




 /*******************************************************************************
 * Error & Clean-up Code
 ******************************************************************************/

/**
 * Exit and handle proper clean-up
 */
void exit_clean(int n)
{
    gfx_End();
    exit(n);
}

/**
 * Exits, but first displays an error message
 */
void exit_fail(char* message)
{
    gfx_SetTextFGColor(gfx_red);
    gfx_SetTextBGColor(gfx_black);
    gfx_PrintStringXY("ERROR:", 1, 1);
    gfx_PrintStringXY(message, 1, 10);
        while (!os_GetCSC());
    exit_clean(1);
}

/**
 * malloc(), but checks for NULL.
 * If NULL, exits with an error message instead of crashing.
 */
void* malloc_safe(int size)
{
    char* memory = malloc(size);
    if (!memory)
        exit_fail("malloc() failed; out of heap?");
    return memory;
}

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

void set_times(uint8_t size, fontlib_load_options_t options)
{
    fontlib_font_t* font;
    font = fontlib_GetFontByStyle(times_pack_name, size, size, 0, 0xFF, 0, 0);
    if (font && fontlib_SetFont(font, options))
        return;
    font_missing(times_pack_name);
}

void set_drsans(uint8_t size, uint8_t weight, fontlib_load_options_t options)
{
    fontlib_font_t* font;
    font = fontlib_GetFontByStyle(drsans_pack_name, size, size, weight, weight, 0, 0);
    if (font && fontlib_SetFont(font, options))
        return;
    font_missing(drsans_pack_name);
}

void print_centered(const char *string) {
    fontlib_SetCursorPosition(fontlib_GetWindowWidth() / 2 + fontlib_GetWindowXMin() - (fontlib_GetStringWidth(string) / 2), fontlib_GetCursorY());
    fontlib_DrawString(string);
}

void print_right(const char *string) {
    fontlib_SetCursorPosition(fontlib_GetWindowWidth() + fontlib_GetWindowXMin() - fontlib_GetStringWidth(string), fontlib_GetCursorY());
    fontlib_DrawString(string);
}

void reverse_colors(void)
{
    fontlib_SetColors(fontlib_GetBackgroundColor(), fontlib_GetForegroundColor());
}

/**
 * Prints a string, but with word-wrap!
 * @param string Text to print
 * @param fake_print Set to true to perform the exact same layout logic, but
 * without actually printing anything, and also return at the first newline.
 * This allows finding word-wrap points.
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
    if (first_printable == '\0')
        first_printable = '\1';
    fontlib_SetNewlineCode('\0');
    fontlib_SetAlternateStopCode(' ');
    fontlib_SetNewlineOptions(FONTLIB_AUTO_CLEAR_TO_EOL | FONTLIB_AUTO_SCROLL);
    do
    {
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
                if (str_width > width && x == left)
                    if (!fake_print)    
                        x = fontlib_DrawString(string);
                    else
                    {
                        do
                            x += (str_width = fontlib_GetGlyphWidth(*string++));
                        while (x < right);
                        x -= str_width;
                        break;
                    }
                else
                {
                    if (fake_print)
                        break;
                    fontlib_Newline();
                    x = left;
                    continue;
                }
            /* If the width returned was zero, that means either another space
             * is waiting to be printed, which will be handled below; or a
             * control code is next, which also will be handled below. */
        }
        string = fontlib_GetLastCharacterRead();
        c = (unsigned char)(*string);
        if (c < first_printable)
        {
            if (c == old_newline && old_newline != '\0')
            {
                if (fake_print)
                    break;
                string++;
                fontlib_Newline();
            }
            else
                break;
        }
        else if (c == ' ')
        {
            string++;
            if (x + space_width < right)
                if (!fake_print)
                    fontlib_DrawGlyph(' ');
                else
                    x += space_width;
            else
                if (fake_print)
                    break;
                else
                {
                    fontlib_Newline();
                    x = left;
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



/*******************************************************************************
 * MAIN
 ******************************************************************************/

#define MAIN_MENU_X 70
#define MAIN_MENU_Y 146
#define MAIN_MENU_FONT_SIZE 16
#define ABOUT_Y 210
#define CURSOR_GLYPH '\x0F'

/* Main Function */
void main(void) {
    fontlib_font_t* times_font;
    char* blah;
    uint8_t selection = 0;
    uint8_t key, old_fgc, line_height = 0, cursor_width = 0;
    bool redraw_main_menu = true;
    /* Make sure RTC is running */
    rtc_Control = RTC_ENABLE;
    gfx_Begin();
    init_history();
    do
    {
        if (redraw_main_menu)
        {
            redraw_main_menu = false;
            gfx_FillScreen(gfx_white);
            fontlib_SetFirstPrintableCodePoint(12);
            fontlib_SetWindowFullScreen();
            fontlib_SetCursorPosition(0, 60);
            fontlib_SetColors(gfx_black, gfx_white);
            fontlib_SetTransparency(false);
            fontlib_SetNewlineOptions(FONTLIB_ENABLE_AUTO_WRAP);
            
            set_times(16, 0);
            print_centered("The\n");
            set_times(23, 0);
            print_centered("Colossal Cave\n");
            set_times(23, 0);
            print_centered("Adventure\n");
            
            set_drsans(14, FONTLIB_NORMAL, 0);
            fontlib_SetCursorPosition(0, ABOUT_Y);
            /*fontlib_DrawString("(c) 1977, 2005 Will Crowther & Don Woods");*/
            fontlib_DrawString(" (c) 1977 Will Crowther");
            print_right("(c) 2017 Eric S. Raymond \n");
            fontlib_DrawString(" (c) 1977, 2005 Don Woods");
            print_right("(c) 2019 Dr. D'nar ");
            
            set_drsans(14, FONTLIB_BOLD, 0);
            line_height = fontlib_GetCurrentFontHeight();
            cursor_width = fontlib_GetGlyphWidth(CURSOR_GLYPH);
            fontlib_SetCursorPosition(MAIN_MENU_X, MAIN_MENU_Y);
            fontlib_DrawString("New");
            fontlib_SetCursorPosition(MAIN_MENU_X, MAIN_MENU_Y + line_height);
            fontlib_DrawString("Resume");
            fontlib_SetCursorPosition(MAIN_MENU_X, MAIN_MENU_Y + line_height + line_height);
            fontlib_DrawString("Exit");
        }
        
        fontlib_SetCursorPosition(MAIN_MENU_X - cursor_width, MAIN_MENU_Y + line_height * selection);
        fontlib_DrawGlyph(CURSOR_GLYPH);
        
        key = 0;
        while (!key)
            key = os_GetCSC();
        
        old_fgc = fontlib_GetForegroundColor();
        fontlib_SetForegroundColor(fontlib_GetBackgroundColor());
        fontlib_SetCursorPosition(MAIN_MENU_X - cursor_width, MAIN_MENU_Y + line_height * selection);
        fontlib_DrawGlyph(CURSOR_GLYPH);
        fontlib_SetForegroundColor(old_fgc);
        
        switch (key)
        {
            case sk_Del:
            case sk_Clear:
            case sk_Mode:
                exit_clean(0);
            case sk_Up:
                if (selection > 0)
                    selection--;
                break;
            case sk_Down:
                if (selection < 2)
                    selection++;
                break;
            case sk_1:
                selection = 0;
                break;
            case sk_2:
                selection = 1;
                break;
            case sk_3:
                selection = 2;
                break;
            case sk_Enter:
            case sk_2nd:
                switch (selection)
                {
                    case 0:
                        blah = get_string(3, 3, 250, 16, fontlib_GetFontByStyle(drsans_pack_name, 14, 14, FONTLIB_BOLD, FONTLIB_BOLD, 0, 0));
                        fontlib_SetWindowFullScreen();
                        fontlib_SetCursorPosition(1, 30);
                        fontlib_DrawString(blah);
                        fontlib_ClearEOL();
                        add_history(blah);
                        break;
                    case 1:
                        fontlib_SetWindow(20, 2, 120, 30);
                        fontlib_HomeUp();
                        print_word_wrap("The quick brown fox jumps over the lazy dog. Jack Dawes loves my big sphinx of quartz.", false);
                        break;
                    case 2:
                        exit_clean(0);
                }
                break;
        }
    }
    while (true);
}
