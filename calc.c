#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tice.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

#include <graphx.h>
#include <fileioc.h>
#include <fontlibc.h>
#include <compression.h>

#include "dungeon.h"
#include "calc.h"
#include "editor.h"
#include "style.h"
#include "advent.h"
#include "ez80.h"
#include "splash.h"

/* Random place to put globals */
command_word_t empty_command_word;
const char* save_file_header = SAVE_FILE_HEADER;
jmp_buf return_to_main;
char paginate_message[65];
#define CURSOR_GLYPH '\x0F'


bool valid_name(char* filename)
{
    unsigned int i;
    unsigned char c;
    size_t len = strlen(filename);
    if (len == 0 || len > 8)
        return false;
    for (i = 0; i < len; i++, filename++)
    {
        c = (unsigned char)(*filename);
        if (c >= 'A' && c <= 'Z')
            continue;
        if (c >= 'a' && c <= 'z')
            continue;
        /* convert theta */
        if (c == '\264')
        {
            *filename = '[';
            continue;
        }
        if (c >= '0' && c <= '9' && i != 0)
            continue;
        return false;
    }
    return true;
}

static void restore_splash(void)
{
    gfx_Blit(gfx_buffer);
}


/*******************************************************************************
 * Key routines
 ******************************************************************************/

 sk_key_t wait_any_key()
{
    sk_key_t key;
    unsigned int timer = get_rtc_seconds_plus(APD_DIM_TIME);
    bool dimmed = false;
    clear_on_key();
    do
    {
        if (get_rtc_seconds() == timer)
        {
            if (!dimmed)
            {
                dimmed = true;
                lcd_dim();
                timer = get_rtc_seconds_plus(APD_QUIT_TIME);
            }
            else
                exit_apd();
        }
        key = get_csc();
    }
    while (!key);
    lcd_bright();
    return key;
}

sk_key_t wait_any_key_msg(char* msg)
{
    fontlib_SetCursorPosition(0, LCD_HEIGHT - fontlib_GetCurrentFontHeight());
    fontlib_DrawString(msg);
    return wait_any_key();
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

char* readline_len(char* prompt, unsigned char max_len, char* default_text)
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
    return get_string(x, y, LCD_WIDTH - x, max_len, font, default_text);
}

char* readline(char* prompt)
{
    return readline_len(prompt, 24, NULL);
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
    /* Clean up scratch RAM (heap/.bss) to prevent graphical ugliness. */
    os_EnableHomeTextBuffer();
    asm_ClrTxtShd();
    exit(n);
}

void exit_apd()
{
    save_apd();
    exit_clean(0);
}

void exit_main(int n)
{
    wait_any_key_msg("Press any key to continue. . . .");
    free_history();
    free(save_file_name);
    save_file_name = NULL;
    longjmp(return_to_main, n);
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
void* malloc_safe(size_t size)
{
    char* memory = malloc(size);
    if (!memory)
        exit_fail("malloc() failed; out of heap?");
    return memory;
}


/*******************************************************************************
 * START & RESUME GAME
 ******************************************************************************/

static void do_game()
{
    print_configure(drsans_pack_name, 14, FONTLIB_BOLD, 0);
    gfx_FillScreen(background_color);
    fontlib_SetTransparency(false);
    print_reset_pagination();
    print_clear();
    init_history();
    play();
    free_history();
}

#define MAX_SAVES 255
#define SAVE_LIST_X 32
#define SAVE_LIST_Y 15
#define ARCHIVE_MARKER '\225'

typedef struct
{
    unsigned long time;
    char name[9];
    bool archived;
} save_file_t;

static int save_compare(save_file_t* a, save_file_t* b)
{
    if (a->time == b->time)
        return 0;
    if (a->time > b->time)
        return -1;
    return 1;
}

static void resume_game(void)
{
    uint8_t key, old_fgc, line_height, cursor_width, asterisk_width;
    unsigned char selection, total_saves, current_index, start_index, stop_index, saves_per_screen;
    bool redraw;
    char* var_name;
    uint8_t* search_pos;
    ti_var_t file;
    save_t* save;
    save_file_t* current_item;
    save_file_t* save_list = calloc(MAX_SAVES, sizeof(save_file_t));
resume_restart:
    redraw = true;
    selection = 0;
    total_saves = 0;
    current_index = 0;
    start_index = 0;
    stop_index = 0;
    var_name = NULL;
    search_pos = NULL;
    
    while ((var_name = ti_Detect(&search_pos, save_file_header)) != NULL)
    {
        file = ti_Open(var_name, "r");
        if (!file)
        {
            /* Just ignore the error and forget the file. */
            ti_Close(file);
            continue;
        }
        save = ti_GetDataPtr(file);
        save_list[total_saves].archived = ti_IsArchived(file);
        strncpy(&save_list[total_saves].name, var_name, 8);
        save_list[total_saves].time = save->savetime;
        ti_Close(file);
        if (save->version != VRSION)
            continue;
        total_saves++;
        /* I'm not sure how the heck the user made so many saves, but we're
         * cutting them off now! */
        if (total_saves == MAX_SAVES)
            break;
    }

    if (total_saves == 0)
    {
        restore_splash();
        free(save_list);
        fontlib_SetCursorPosition(0, LCD_HEIGHT / 3 - 7);
        print_centered_compressed(NO_SAVED_GAMES);
        key = wait_any_key_msg(get_arbitrary_message(ANY_KEY_TO_RETURN));
        return;
    }

    /* Sort saves by time, so the most recent save appears first */
    qsort(save_list, total_saves, sizeof(save_file_t), &save_compare);
    set_drsans(14, FONTLIB_BOLD, 0);
    cursor_width = fontlib_GetGlyphWidth(CURSOR_GLYPH) + 1;
    asterisk_width = fontlib_GetGlyphWidth(ARCHIVE_MARKER);
    line_height = fontlib_GetCurrentFontHeight();
    saves_per_screen = (LCD_HEIGHT - SAVE_LIST_Y) / line_height;

    do
    {
        if (redraw)
        {
            redraw = false;
            stop_index = start_index + saves_per_screen;
            fontlib_SetWindow(SAVE_LIST_X, SAVE_LIST_Y, LCD_WIDTH - SAVE_LIST_X, LCD_HEIGHT - SAVE_LIST_Y);
            restore_splash();
            fontlib_SetCursorPosition(1, 1);
            draw_compressed(SELECT_FILE);
            fontlib_HomeUp();
            if (stop_index >= total_saves)
                stop_index = total_saves;
            for (current_index = start_index; current_index < stop_index; current_index++)
            {
                current_item = &save_list[current_index];
                if (current_item->archived)
                    fontlib_DrawGlyph(ARCHIVE_MARKER);
                else
                    fontlib_ShiftCursorPosition(asterisk_width, 0);
                fontlib_DrawString(current_item->name);
                fontlib_Newline();
            }
        }

        fontlib_SetCursorPosition(SAVE_LIST_X - cursor_width, SAVE_LIST_Y + line_height * selection);
        fontlib_DrawGlyph(CURSOR_GLYPH);
        key = wait_any_key();
        gfx_BlitRectangle(gfx_buffer, SAVE_LIST_X - cursor_width, SAVE_LIST_Y + line_height * selection, cursor_width, line_height);

        current_item = &save_list[start_index + selection];
        switch (key)
        {
            case sk_Stat:
                file = ti_Open(current_item->name, "r");
                if (!file)
                    /* Uuuh . . . what? */
                    break;
                if (ti_IsArchived(file))
                    ti_SetArchiveStatus(false, file);
                else
                {
                    gfx_End();
                    os_ClrHome();
                    os_PutStrFull(get_arbitrary_message(ARCHIVING_FILE));
                    ti_SetArchiveStatus(true, file);
                    gfx_Begin();
                }
                current_item->archived = ti_IsArchived(file);
                ti_Close(file);
                redraw = true;
                break;
            case sk_Del:
                redraw = true;
                fontlib_SetWindowFullScreen();
                gfx_FillScreen(background_color);
                fontlib_SetCursorPosition(0, LCD_HEIGHT / 3 - 16);
                print_centered_compressed(CONFIRM_DELETE);
                print_centered(current_item->name);
                fontlib_Newline();
                print_centered_compressed(DELETE_LINE_1);
                print_centered_compressed(DELETE_LINE_2);
                if (wait_any_key() != sk_Enter)
                    break;
                ti_Delete(current_item->name);
                goto resume_restart;
            case sk_Mode:
            case sk_Clear:
                return;
            case sk_Enter:
            case sk_2nd:
                save_file_name = malloc(8);
                strncpy(save_file_name, current_item->name, 8);
                free(save_list);
                do_game();
            case sk_Up:
                if (selection == 0)
                {
                    if (start_index == 0)
                        break;
                    redraw = true;
                    start_index--;
                    break;
                }
                selection--;
                break;
            case sk_Down:
                if (selection == saves_per_screen - 1)
                {
                    if (stop_index >= total_saves)
                        break;
                    redraw = true;
                    start_index++;
                    break;
                }
                if (start_index + selection < stop_index - 1)
                    selection++;
                break;
        }
    }
    while (key != sk_Clear);
    free(save_list);
}
 
 /*******************************************************************************
 * MAIN
 ******************************************************************************/

#define MAIN_MENU_X 70
#define MAIN_MENU_Y 146
#define MAIN_MENU_FONT_SIZE 16
#define ABOUT_Y 210

void main(void) {
    gfx_sprite_t* splash;
    uint8_t selection = 0;
    uint8_t key, line_height = 0, cursor_width = 0;
    bool redraw_main_menu;
    /* Initialize stuff */
    gfx_Begin();
    ti_CloseAll();
    load_dungeon();
    decompress_string(dungeon + compressed_strings[get_arbitrary_message_index(PAGINATE_MESSAGE)], paginate_message);
    /* Initialize splash graphic */
    gfx_SetPalette(splash_pal, sizeof_splash_pal, 0);
    splash = gfx_AllocSprite(splashl_width, splashl_height, &malloc_safe);
    zx7_Decompress(splash, splashl_data);
    gfx_Sprite_NoClip(splash, 0, 0);
    free(splash);
    splash = gfx_AllocSprite(splashr_width, splashr_height, &malloc_safe);
    zx7_Decompress(splash, splashr_data);
    gfx_Sprite_NoClip(splash, splashl_width, 0);
    free(splash);
    asm("; Quick and dirty vertical mirror routine");
    asm("	ld	hl, 0D40000h + (320 * 129)");
    asm("	ld	de, 0D40000h + (320 * 130)");
    asm("	ld	a, 110");
    asm("_main_mirror_loop:");
    asm("	ld	bc, 320");
    asm("	ldir");
    asm("	ld	bc, -(320 * 2)");
    asm("	add	hl, bc");
    asm("	dec	a");
    asm("	jr	nz, _main_mirror_loop");
    asm("; Quick and dirty copy to other half of VRAM to cache decompressed image");
    asm("	ld	hl, 0D40000h");
    asm("	ld	de, 0D40000h + (320 * 240)");
    asm("	ld	bc, 320 * 240");
    asm("	ldir");
    /*if (!setjmp(return_to_main))
        We currently have nothing that cares why we're returning here. */
    setjmp(return_to_main);
    redraw_main_menu = true;
    /* Make sure RTC is running */
    rtc_Control = RTC_ENABLE;
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
            restore_splash();
            
            fontlib_SetTransparency(true);
            fontlib_SetFirstPrintableCodePoint(12);
            fontlib_SetWindowFullScreen();
            fontlib_SetCursorPosition(0, 50);
            fontlib_SetColors(foreground_color, background_color);
            fontlib_SetNewlineOptions(FONTLIB_ENABLE_AUTO_WRAP);

            set_times(16, 0);
            print_centered_compressed(TITLE_LINE_1);
            set_times(23, 0);
            print_centered_compressed(TITLE_LINE_2);
            print_centered_compressed(TITLE_LINE_3);
            set_times(13, 0);
            fontlib_ShiftCursorPosition(0, 2);
            print_centered("v" VERSION_STRING);

            set_drsans(14, FONTLIB_NORMAL, 0);
            fontlib_SetCursorPosition(0, ABOUT_Y);
            draw_compressed(ABOUT_1);
            print_right(ABOUT_2);
            draw_compressed(ABOUT_3);
            print_right(ABOUT_4);

            set_drsans(14, FONTLIB_BOLD, 0);
            line_height = fontlib_GetCurrentFontHeight();
            cursor_width = fontlib_GetGlyphWidth(CURSOR_GLYPH) + 2;
            fontlib_SetCursorPosition(MAIN_MENU_X, MAIN_MENU_Y);
            draw_compressed(ITEM_NEW);
            fontlib_SetCursorPosition(MAIN_MENU_X, MAIN_MENU_Y + line_height);
            draw_compressed(ITEM_RESUME);
            fontlib_SetCursorPosition(MAIN_MENU_X, MAIN_MENU_Y + line_height + line_height);
            draw_compressed(ITEM_EXIT);
        }

        fontlib_SetCursorPosition(MAIN_MENU_X - cursor_width, MAIN_MENU_Y + line_height * selection);
        fontlib_DrawGlyph(CURSOR_GLYPH);
        key = wait_any_key();
        gfx_BlitRectangle(gfx_buffer, MAIN_MENU_X - cursor_width, MAIN_MENU_Y + line_height * selection, cursor_width, line_height);

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
                        free(save_file_name);
                        save_file_name = NULL;
                        do_game();
                        break;
                    case 1:
                        redraw_main_menu = true;
                        resume_game();
                        break;
                    case 2:
                        exit_clean(0);
                }
                break;
        }
    }
    while (true);
}
