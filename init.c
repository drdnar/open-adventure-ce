/*
 * Initialisation
 *
 * Copyright (c) 1977, 2005 by Will Crowther and Don Woods
 * Copyright (c) 2017 by Eric S. Raymond
 * SPDX-License-Identifier: BSD-2-clause
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#include "advent.h"
#include "calc.h"

struct settings_t settings = {
    .logfp = NULL,
    .oldstyle = false,
    .prompt = true
};

/* Inline initializers moved to function below for ANSI Cness */
struct game_t game;

int initialise(void)
{
    int i, k, treasure;
    int seedval;
    if (settings.oldstyle)
        printf("Initialising...\n");
    
    game.dloc[1] = LOC_KINGHALL;
    game.dloc[2] = LOC_WESTBANK;
    game.dloc[3] = LOC_Y2;
    game.dloc[4] = LOC_ALIKE3;
    game.dloc[5] = LOC_COMPLEX;

    /*  Sixth dwarf is special (the pirate).  He always starts at his
     *  chest's eventual location inside the maze. This loc is saved
     *  in chloc for ref. The dead end in the other maze has its
     *  loc stored in chloc2. */
    game.dloc[6] = LOC_DEADEND12;
    game.chloc   = LOC_DEADEND12;
    game.chloc2  = LOC_DEADEND13;
    game.abbnum  = 5;
    game.clock1  = WARNTIME;
    game.clock2  = FLASHTIME;
    game.newloc  = LOC_START;
    game.loc     = LOC_START;
    game.limit   = GAMELIMIT;
    game.foobar  = WORD_EMPTY;

    srand(time(NULL));
    seedval = (int)rand();
    set_seed(seedval);

    for (i = 1; i <= NOBJECTS; i++) {
        game.place[i] = LOC_NOWHERE;
    }

    for (i = 1; i <= NLOCATIONS; i++) {
        if (!(get_location(i)->description.big == 0 ||
              get_tkey(i) == 0)) {
            k = get_tkey(i);
            if (T_TERMINATE(get_travelop(k)))
                conditions[i] |= (1 << COND_FORCED);
        }
    }

    /*  Set up the game.atloc and game.link arrays.
     *  We'll use the DROP subroutine, which prefaces new objects on the
     *  lists.  Since we want things in the other order, we'll run the
     *  loop backwards.  If the object is in two locs, we drop it twice.
     *  Also, since two-placed objects are typically best described
     *  last, we'll drop them first. */
    for (i = NOBJECTS; i >= 1; i--) {
        if (get_object(i)->fixd > 0) {
            drop(i + NOBJECTS, get_object(i)->fixd);
            drop(i, get_object(i)->plac);
        }
    }

    for (i = 1; i <= NOBJECTS; i++) {
        k = NOBJECTS + 1 - i;
        game.fixed[k] = get_object(k)->fixd;
        if (get_object(k)->plac != 0 && get_object(k)->fixd <= 0)
            drop(k, get_object(k)->plac);
    }

    /*  Treasure props are initially -1, and are set to 0 the first time
     *  they are described.  game.tally keeps track of how many are
     *  not yet found, so we know when to close the cave. */
    for (treasure = 1; treasure <= NOBJECTS; treasure++) {
        if (get_object(treasure)->is_treasure) {
            if (get_object(treasure)->inventory != 0)
                game.prop[treasure] = STATE_NOTFOUND;
            game.tally = game.tally - game.prop[treasure];
        }
    }
    game.conds = setbit(11);

    return seedval;
}
