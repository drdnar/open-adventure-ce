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

#include "dungeon.h"
#include "calc.h"
#include "editor.h"
#include "style.h"
#include "advent.h"

/* Random place to put globals */
command_word_t empty_command_word;
char* save_file_header = SAVE_FILE_HEADER;


sk_key_t wait_any_key()
{
    sk_key_t key;
    do
    {
        asm("   ei");
        asm("   halt");
        key = os_GetCSC();
    }
    while (!key);
    return key;
}

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
 * readline replacement
 ******************************************************************************/

char* readline_len(char* prompt, unsigned char max_len)
{
    fontlib_font_t* font;
    unsigned int x;
    unsigned char y;
    font = set_drsans(14, FONTLIB_BOLD, 0);
    y = LCD_HEIGHT - fontlib_GetCurrentFontHeight();
    fontlib_SetWindowFullScreen();
    fontlib_SetCursorPosition(0, y);
    fontlib_DrawString(prompt);
    x = fontlib_GetCursorX();
    return get_string(x, y, LCD_WIDTH - x, max_len, font);
}

char* readline(char* prompt)
{
    return readline_len(prompt, 24);
}


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

void exit_main(int n)
{
    wait_any_key();
    exit_clean(n);
}

/**
 * Exits, but first displays an error message
 */
void exit_fail(char* message)
{
    gfx_FillScreen(gfx_black);
    gfx_SetTextFGColor(gfx_red);
    gfx_SetTextBGColor(gfx_black);
    gfx_PrintStringXY("ERROR:", 1, 1);
    gfx_PrintStringXY(message, 1, 10);
    wait_any_key();
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


void do_game(void)
{
    print_configure(drsans_pack_name, 14, FONTLIB_BOLD, 0);
    gfx_FillScreen(background_color);
    print_reset_pagination();
    print_clear();
    init_history();
    play();
    free_history();
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
    ti_CloseAll();
    gfx_Begin();
    init_history();
    /* Need to initialize some globals */
    settings.logfp = NULL;
    settings.oldstyle = false;
    settings.prompt = true;
    empty_command_word.raw[0] = '\0';
    empty_command_word.id = WORD_EMPTY;
    empty_command_word.type = NO_WORD_TYPE;
    do
    {
        if (redraw_main_menu)
        {
            redraw_main_menu = false;
            gfx_FillScreen(background_color);
            fontlib_SetFirstPrintableCodePoint(12);
            fontlib_SetWindowFullScreen();
            fontlib_SetCursorPosition(0, 60);
            fontlib_SetColors(foreground_color, background_color);
            fontlib_SetTransparency(false);
            fontlib_SetNewlineOptions(FONTLIB_ENABLE_AUTO_WRAP);
            
            set_times(16, 0);
            print_centered("The\n");
            set_times(23, 0);
            print_centered("Colossal Cave\n");
            times_font = set_times(23, 0);
            print_centered("Adventure");
            key = times_font->space_above + times_font->baseline_height;
            times_font = set_times(13, 0);
            fontlib_ShiftCursorPosition(8, key - times_font->space_above - times_font->baseline_height);
            fontlib_DrawString("beta 6/20");
            
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
        
        key = wait_any_key();
        
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
                        /*blah = get_string(3, 3, 250, 16, fontlib_GetFontByStyle(drsans_pack_name, 14, 14, FONTLIB_BOLD, FONTLIB_BOLD, 0, 0));
                        fontlib_SetWindowFullScreen();
                        fontlib_SetCursorPosition(1, 30);
                        fontlib_DrawString(blah);
                        fontlib_ClearEOL();
                        add_history(blah);*/
                        do_game();
                        break;
                    case 1:
                        break;
                    case 2:
                        exit_clean(0);
                }
                break;
        }
    }
    while (true);
}
