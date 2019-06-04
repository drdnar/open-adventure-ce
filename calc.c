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
/*#include "advent.h"*/


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
    gfx_End();
    exit(1);
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


#define MAIN_MENU_X 70
#define MAIN_MENU_Y 146
#define MAIN_MENU_FONT_SIZE 16
#define ABOUT_Y 210
#define CURSOR_GLYPH '\x0F'

/* Main Function */
void main(void) {
    fontlib_font_t* times_font;
    uint8_t selection = 0;
    uint8_t key, old_fgc, line_height = 0, cursor_width = 0;
    bool redraw_main_menu = true;
    gfx_Begin();
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
                gfx_End();
                exit(0);
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
                if (selection == 2)
                    
                break;
        }
    }
    while (true);
}
