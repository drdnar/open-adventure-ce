/*
 * Saving and resuming.
 *
 * (ESR) This replaces  a bunch of particularly nasty FORTRAN-derived code;
 * see the history.adoc file in the source distribution for discussion.
 *
 * Copyright (c) 1977, 2005 by Will Crowther and Don Woods
 * Copyright (c) 2017 by Eric S. Raymond
 * SPDX-License-Identifier: BSD-2-clause
 */

#include <stdlib.h>
#include <string.h>
#ifndef CALCULATOR
#include <editline/readline.h>
#include <time.h>
#endif
#include <stdint.h>

#ifdef CALCULATOR
#include <fileioc.h>
#endif

#include "advent.h"
#include "dungeon.h"
#ifdef CALCULATOR
#include "calc.h"
#include "style.h"
#endif

struct save_t save;

#ifdef CALCULATOR
bool save_validated;
char* save_file_name;
#endif

#define IGNORE(r) do{if (r){}}while(0)

int savefile(FILE *fp, int32_t version)
/* Save game to file. No input or output from user. */
{
    save.savetime = time(NULL);
    save.mode = -1;
    save.version = (version == 0) ? VRSION : version;
#ifdef CALCULATOR
    strcpy(save.id_str, save_file_header);
#endif

    save.game = game;

    IGNORE(fwrite(&save, sizeof(struct save_t), 1, fp));

    return (0);
}

#ifdef CALCULATOR
static bool enough_mem()
{
    char* ignored;
    size_t free_mem;
    free_mem = os_MemChk(&ignored);
    /* _MemChk probably SHOULDN'T move anything around in this case, but we'll
     * refresh the dungeon pointers just to be safe */
    load_dungeon();
    return free_mem >= sizeof(save_t) + 16;
}

static bool is_archived(char* name)
{
    bool archived;
    ti_var_t file;
    if (!(file = ti_Open(name, "r")))
        return false;
    archived = ti_IsArchived(file);
    ti_Close(file);
    return archived;
}

void save_apd()
{
    FILE *fp;
    bool archived;
    if (!save_file_name)
        return;
    if (!enough_mem())
        return;
    archived = is_archived(save_file_name);
    fp = fopen(save_file_name, WRITE_MODE);
    if (fp == NULL)
        return;
    /* Only deduct three points---they've either already waited a few minutes,
     * or had to leave to do something else. */
    game.saved += 3;
    savefile(fp, VRSION);
    if (archived)
    {
        gfx_End();
        ti_SetArchiveStatus(true, fp->slot);
    }
    fclose(fp);
}

int set_save_file_name(void)
{
    char* header;
    char* name = NULL;
    ti_var_t dungeon_file = 0;

    if (!enough_mem())
    {
        print_compressed(NOT_ENOUGH_MEM);
        return GO_TOP;
    }

    while (true) {
        free(name);
        name = readline_len(get_arbitrary_message(FILE_NAME_PROMPT), 8, save_file_name);
        if (name == NULL || strlen(name) == 0)
        {
            free(name);
            return GO_TOP;
        }
        if (!valid_name(name))
        {
            print_compressed(INVALID_FILE_NAME);
            continue;
        }
        dungeon_file = ti_Open(name, "r");
        if (dungeon_file)
        {
            header = ti_GetDataPtr(dungeon_file);
            ti_Close(dungeon_file);
            if (strcmp(header, save_file_header))
            {
                print_compressed(CREATED_BY_OTHER_PROGRAM);
                continue;
            }
        }
        break;
    }
    free(save_file_name);
    save_file_name = name;
    rspeak(OK_MAN);
    return GO_TOP;
}
#endif

/* Suspend and resume */
int suspend(void)
{
    /*  Suspend.  Offer to save things in a file, but charging
     *  some points (so can't win by using saved games to retry
     *  battles or to start over after learning zzword).
     *  If ADVENT_NOSAVE is defined, do nothing instead. */
    char* name;
    FILE *fp = NULL;
#ifdef CALCULATOR
    ti_var_t save_file;
    bool was_archived = false;
#endif
#ifdef ADVENT_NOSAVE
    return GO_UNKNOWN;
#endif
#ifdef CALCULATOR
    if (!enough_mem())
    {
        print_compressed(NOT_ENOUGH_MEM);
        return GO_TOP;
    }
#endif

    rspeak(SUSPEND_WARNING);
    if (!yes(get_arbitrary_message_index(THIS_ACCEPTABLE), get_arbitrary_message_index(OK_MAN), get_arbitrary_message_index(OK_MAN)))
        return GO_CLEAROBJ;

    while (fp == NULL) {
#ifndef CALCULATOR
        name = readline("\nFile name: ");
#else
        name = readline_len(get_arbitrary_message(FILE_NAME_PROMPT), 8, save_file_name);
        was_archived = false;
#endif
        if (name == NULL || strlen(name) == 0)
        {
            free(name);
            return GO_TOP;
        }
#ifdef CALCULATOR
        if (valid_name(name))
        {
        save_file = ti_Open(name, "r");
        if (save_file)
        {
            if (strcmp(ti_GetDataPtr(save_file), save_file_header))
            {
                print_compressed(CREATED_BY_OTHER_PROGRAM);
                free(name);
                continue;
            }
            else
                was_archived = ti_IsArchived(save_file);
        }
        ti_Close(save_file);
#endif
        fp = fopen(name, WRITE_MODE);
        if (fp == NULL)
#ifndef CALCULATOR
            printf("Can't open file %s, try again.\n", name);
#else
            print_compressed(FAILED_TO_OPEN);
#endif
#ifdef CALCULATOR
        }
        else
            print_compressed(INVALID_FILE_NAME);
#endif
        free(name);
    }

    game.saved = game.saved + 5;

    savefile(fp, VRSION);
#ifdef CALCULATOR
    if (was_archived)
    {
        gfx_End();
        os_ClrHome();
        os_PutStrFull(get_arbitrary_message(SAVING_FILE));
        ti_SetArchiveStatus(true, fp->slot);
        gfx_resume_render_splash();
        gfx_FillScreen(background_color);
    }
#endif
    fclose(fp);
#ifdef CALCULATOR
    /* Writing can potentially move the dungeon file, so refresh pointers to be
     * safe. */
    load_dungeon();
#endif
    rspeak(RESUME_HELP);
#ifndef CALCULATOR
    exit(EXIT_SUCCESS);
#else
    exit_main(EXIT_SUCCESS);
    /* make ZDS shut up */
    return 0;
#endif
}

int resume()
{
    /*  Resume.  Read a suspended game back from a file.
     *  If ADVENT_NOSAVE is defined, do nothing instead. */
    char* name = NULL;
    bool restore_successful;

#ifdef ADVENT_NOSAVE
    return GO_UNKNOWN;
#endif
    FILE *fp = NULL;

    if (game.loc != 1 ||
        game.abbrev[1] != 1) {
        rspeak(RESUME_ABANDON);
        if (!yes(get_arbitrary_message_index(THIS_ACCEPTABLE), get_arbitrary_message_index(OK_MAN), get_arbitrary_message_index(OK_MAN)))
            return GO_CLEAROBJ;
    }

    while (fp == NULL) {
#ifndef CALCULATOR
        name = readline("\nFile name: ");
#else
        name = readline_len(get_arbitrary_message(FILE_NAME_PROMPT), 8, save_file_name);
#endif
        if (name == NULL || strlen(name) == 0)
        {
            free(name);
            return GO_TOP;
        }
#ifdef CALCULATOR
        /* This also handles converting the theta character as a side-effect. */
        IGNORE(valid_name(name));
#endif
        fp = fopen(name, READ_MODE);
        if (fp == NULL)
#ifndef CALCULATOR
            printf("Can't open file %s, try again.\n", name);
#else
            print_compressed(FAILED_TO_OPEN_2);
#endif
        else
            break;
        free(name);
    }

#ifndef CALCULATOR
    free(name);
#endif
    restore_successful = restore(fp);
#ifdef CALCULATOR
    if (save_validated)
    {
        free(save_file_name);
        save_file_name = name;
    }
    else
    {
        free(name);
    }
#endif
    return restore_successful;
}

int restore(FILE* fp)
{
#ifdef CALCULATOR
    save_validated = false;
#endif
    /*  Read and restore game state from file, assuming
     *  sane initial state.
     *  If ADVENT_NOSAVE is defined, do nothing instead. */
#ifdef ADVENT_NOSAVE
    return GO_UNKNOWN;
#endif

    IGNORE(fread(&save, sizeof(struct save_t), 1, fp));
    fclose(fp);

#ifdef CALCULATOR
    if (strcmp(save.id_str, save_file_header))
        print_compressed(FILE_USED_BY_OTHER_PROGRAM);
    else
#endif
    if (save.version != VRSION) {
        rspeak(VERSION_SKEW, save.version / 10, MOD(save.version, 10), VRSION / 10, MOD(VRSION, 10));
    } else if (is_valid(save.game)) {
        game = save.game;
#ifdef CALCULATOR
        save_validated = true;
#endif
    }
#ifdef CALCULATOR
    else
        print_compressed(CORRUPTED_SAVE);
#endif
    return GO_TOP;
}

bool is_valid(struct game_t valgame)
{
    /*  Save files can be roughly grouped into three groups:
     *  With valid, reaceable state, with valid, but unreachable
     *  state and with invaild state. We check that state is
     *  valid: no states are outside minimal or maximal value
     */
    int i, temp_tally, treasure;
    obj_t obj;
    loc_t loc;

    /* Prevent division by zero */
    if (valgame.abbnum == 0) {
        return false;	// LCOV_EXCL_LINE
    }

    /* Check for RNG overflow. Truncate */
    if (valgame.lcg_x >= LCG_M) {
        valgame.lcg_x %= LCG_M; // LCOV_EXCL_LINE
    }

    /* Check for RNG underflow. Transpose */
    if (valgame.lcg_x < LCG_M) {
        valgame.lcg_x = LCG_M + (valgame.lcg_x % LCG_M);
    }

    /*  Bounds check for locations */
    if ( valgame.chloc  < -1 || valgame.chloc  > NLOCATIONS ||
         valgame.chloc2 < -1 || valgame.chloc2 > NLOCATIONS ||
         valgame.loc    <  0 || valgame.loc    > NLOCATIONS ||
         valgame.newloc <  0 || valgame.newloc > NLOCATIONS ||
         valgame.oldloc <  0 || valgame.oldloc > NLOCATIONS ||
         valgame.oldlc2 <  0 || valgame.oldlc2 > NLOCATIONS) {
        return false;	// LCOV_EXCL_LINE
    }
    /*  Bounds check for location arrays */
    for (i = 0; i <= NDWARVES; i++) {
        if (valgame.dloc[i]  < -1 || valgame.dloc[i]  > NLOCATIONS  ||
            valgame.odloc[i] < -1 || valgame.odloc[i] > NLOCATIONS) {
            return false;	// LCOV_EXCL_LINE
        }
    }

    for (i = 0; i <= NOBJECTS; i++) {
        if (valgame.place[i] < -1 || valgame.place[i] > NLOCATIONS  ||
            valgame.fixed[i] < -1 || valgame.fixed[i] > NLOCATIONS) {
            return false;	// LCOV_EXCL_LINE
        }
    }

    /*  Bounds check for dwarves */
    if (valgame.dtotal < 0 || valgame.dtotal > NDWARVES ||
        valgame.dkill < 0  || valgame.dkill  > NDWARVES) {
        return false;	// LCOV_EXCL_LINE
    }

    /*  Validate that we didn't die too many times in save */
    if (valgame.numdie >= NDEATHS) {
        return false;	// LCOV_EXCL_LINE
    }

    /* Recalculate tally, throw the towel if in disagreement */
    temp_tally = 0;
    for (treasure = 1; treasure <= NOBJECTS; treasure++) {
        if (get_object(treasure)->is_treasure) {
            if (valgame.prop[treasure] == STATE_NOTFOUND) {
                ++temp_tally;
            }
        }
    }
    if (temp_tally != valgame.tally) {
        return false;	// LCOV_EXCL_LINE
    }

    /* Check that properties of objects aren't beyond expected */
    for (obj = 0; obj <= NOBJECTS; obj++) {
        if (valgame.prop[obj] < STATE_NOTFOUND || valgame.prop[obj] > 1) {
            switch (obj) {
            case RUG:
            case DRAGON:
            case BIRD:
            case BOTTLE:
            case PLANT:
            case PLANT2:
            case TROLL:
            case URN:
            case EGGS:
            case VASE:
            case CHAIN:
                if (valgame.prop[obj] == 2) // There are multiple different states, but it's convenient to clump them together
                    continue;
            /* FALLTHRU */
            case BEAR:
                if (valgame.prop[BEAR] == CONTENTED_BEAR || valgame.prop[BEAR] == BEAR_DEAD)
                    continue;
            /* FALLTHRU */
            default:
                return false;	// LCOV_EXCL_LINE
            }
        }
    }

    /* Check that values in linked lists for objects in locations are inside bounds */
    for (loc = LOC_NOWHERE; loc <= NLOCATIONS; loc++) {
        if (valgame.atloc[loc] < NO_OBJECT || valgame.atloc[loc] > NOBJECTS * 2) {
            return false;	// LCOV_EXCL_LINE
        }
    }
    for (obj = 0; obj <= NOBJECTS * 2; obj++ ) {
        if (valgame.link[obj] < NO_OBJECT || valgame.link[obj] > NOBJECTS * 2) {
            return false;	// LCOV_EXCL_LINE
        }
    }

    return true;
}

/* end */
