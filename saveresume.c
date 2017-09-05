#include <stdlib.h>
#include <string.h>
#include <editline/readline.h>
#include <time.h>

#include "advent.h"
#include "dungeon.h"

/*
 * (ESR) This replaces  a bunch of particularly nasty FORTRAN-derived code;
 * see the history.adoc file in the source distribution for discussion.
 */

#define VRSION	27	/* bump on save format change */

/*
 * If you change the first three members, the resume function may not properly
 * reject saves from older versions.  Yes, this glues us to a hardware-
 * dependent length of long.  Later members can change, but bump the version
 * when you do that.
 */
struct save_t {
    long savetime;
    long mode;		/* not used, must be present for version detection */
    long version;
    struct game_t game;
};
struct save_t save;

#define IGNORE(r) do{if (r){}}while(0)

int savefile(FILE *fp, long version)
/* Save game to file. No input or output from user. */
{
    save.savetime = time(NULL);
    save.mode = -1;
    save.version = (version == 0) ? VRSION : version;

    save.game = game;
    IGNORE(fwrite(&save, sizeof(struct save_t), 1, fp));
    return (0);
}

/* Suspend and resume */
int suspend(void)
{
    /*  Suspend.  Offer to save things in a file, but charging
     *  some points (so can't win by using saved games to retry
     *  battles or to start over after learning zzword).
     *  If ADVENT_NOSAVE is defined, do nothing instead. */

#ifdef ADVENT_NOSAVE
    return GO_UNKNOWN;
#endif
    FILE *fp = NULL;

    rspeak(SUSPEND_WARNING);
    if (!yes(arbitrary_messages[THIS_ACCEPTABLE], arbitrary_messages[OK_MAN], arbitrary_messages[OK_MAN]))
        return GO_CLEAROBJ;
    game.saved = game.saved + 5;

    while (fp == NULL) {
        char* name = readline("\nFile name: ");
        if (name == NULL)
            return GO_TOP;
        fp = fopen(name, WRITE_MODE);
        if (fp == NULL)
            printf("Can't open file %s, try again.\n", name);
        free(name);
    }

    savefile(fp, VRSION);
    fclose(fp);
    rspeak(RESUME_HELP);
    exit(EXIT_SUCCESS);
}

int resume(void)
{
    /*  Resume.  Read a suspended game back from a file.
     *  If ADVENT_NOSAVE is defined, do nothing instead. */

#ifdef ADVENT_NOSAVE
    return GO_UNKNOWN;
#endif
    FILE *fp = NULL;

    if (game.loc != 1 ||
        game.abbrev[1] != 1) {
        rspeak(RESUME_ABANDON);
        if (!yes(arbitrary_messages[THIS_ACCEPTABLE], arbitrary_messages[OK_MAN], arbitrary_messages[OK_MAN]))
            return GO_CLEAROBJ;
    }

    while (fp == NULL) {
        char* name = readline("\nFile name: ");
        if (name == NULL)
            return GO_TOP;
        fp = fopen(name, READ_MODE);
        if (fp == NULL)
            printf("Can't open file %s, try again.\n", name);
        free(name);
    }

    return restore(fp);
}

bool is_valid(struct game_t*);

int restore(FILE* fp)
{
    /*  Read and restore game state from file, assuming
     *  sane initial state.
     *  If ADVENT_NOSAVE is defined, do nothing instead. */
#ifdef ADVENT_NOSAVE
    return GO_UNKNOWN;
#endif

    IGNORE(fread(&save, sizeof(struct save_t), 1, fp));
    fclose(fp);
    if (save.version != VRSION) {
        rspeak(VERSION_SKEW, save.version / 10, MOD(save.version, 10), VRSION / 10, MOD(VRSION, 10));
    } else if (is_valid(&save.game)) {
        game = save.game;
    }
    return GO_TOP;
}

bool is_valid(struct game_t* valgame)
{
    /*  Save files can be roughly grouped into three groups:
     *  With valid, reaceable state, with valid, but unreachable
     *  state and with invaild state. We check that state is
     *  valid: no states are outside minimal or maximal value
     */

    /* Prevent division by zero */
    if (valgame->abbnum == 0) {
        return false;
    }

    /* Prevent RNG substitution. Why we are saving PRNG parameters? */

    if (valgame->lcg_a != game.lcg_a || valgame->lcg_c != game.lcg_c || valgame->lcg_m != game.lcg_m) {
        return false;
    }

    /* Check for RNG overflow. Truncate */
    if (valgame->lcg_x >= game.lcg_m) {
        valgame->lcg_x %= game.lcg_m;
    }

    /*  Bounds check for locations */
    if ( valgame->chloc  < -1 || valgame->chloc  > NLOCATIONS ||
         valgame->chloc2 < -1 || valgame->chloc2 > NLOCATIONS ||
         valgame->loc    < -1 || valgame->loc    > NLOCATIONS ||
         valgame->newloc < -1 || valgame->newloc > NLOCATIONS ||
         valgame->oldloc < -1 || valgame->oldloc > NLOCATIONS ||
         valgame->oldlc2 < -1 || valgame->oldlc2 > NLOCATIONS) {
        return false;
    }
    /*  Bounds check for location arrays */
    for (int i = 0; i <= NDWARVES; i++) {
        if (valgame->dloc[i]  < -1 || valgame->dloc[i]  > NLOCATIONS  ||
            valgame->odloc[i] < -1 || valgame->odloc[i] > NLOCATIONS) {
            return false;
        }
    }

    for (int i = 0; i <= NOBJECTS; i++) {
        if (valgame->place[i] < -1 || valgame->place[i] > NLOCATIONS  ||
            valgame->fixed[i] < -1 || valgame->fixed[i] > NLOCATIONS) {
            return false;
        }
    }

    /*  Bounds check for dwarves */
    if (valgame->dtotal < 0 || valgame->dtotal > NDWARVES ||
        valgame->dkill < 0  || valgame->dkill  > NDWARVES) {
        return false;
    }

    /*  Validate that we didn't die too many times in save */
    if (valgame->numdie >= NDEATHS) {
        return false;
    }

    /* Recalculate tally, throw the towel if in disagreement */
    long temp_tally = 0;
    for (int treasure = 1; treasure <= NOBJECTS; treasure++) {
        if (objects[treasure].is_treasure) {
            if (valgame->prop[treasure] == STATE_NOTFOUND) {
                ++temp_tally;
            }
        }
    }
    if (temp_tally != valgame->tally) {
        return false;
    }

    /* Check that properties of objects aren't beyond expected */
    for (obj_t obj = 0; obj <= NOBJECTS; obj++) {
        if (valgame->prop[obj] < STATE_NOTFOUND || valgame->prop[obj] > 1) {
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
                if (valgame->prop[obj] == 2) // There are multiple different states, but it's convenient to clump them together
                    continue;
            case BEAR:
                if (valgame->prop[BEAR] == CONTENTED_BEAR || valgame->prop[BEAR] == BEAR_DEAD)
                    continue;
            default:
                return false;
            }
        }
    }

    /* Check that values in linked lists for objects in locations are inside bounds */
    for (loc_t loc = LOC_NOWHERE; loc <= NLOCATIONS; loc++) {
        if (valgame->atloc[loc] < NO_OBJECT || valgame->atloc[loc] > NOBJECTS * 2) {
            return false;
        }
    }
    for (obj_t obj = 0; obj <= NOBJECTS * 2; obj++ ) {
        if (valgame->link[obj] < NO_OBJECT || valgame->link[obj] > NOBJECTS * 2) {
            return false;
        }
    }

    return true;
}

/* end */