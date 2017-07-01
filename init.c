#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#include "advent.h"

/*
 * Initialisation
 */

long initialise(void)
{
    if (oldstyle)
        printf("Initialising...\n");

    /* Initialize our LCG PRNG with parameters tested against
     * Knuth vol. 2. by the original authors */
    game.lcg_a = 1093;
    game.lcg_c = 221587;
    game.lcg_m = 1048576;
    srand(time(NULL));
    long seedval = (long)rand();
    set_seed(seedval);

    for (int i = 1; i <= NOBJECTS; i++) {
        game.place[i] = LOC_NOWHERE;
    }

    for (int i = 1; i <= NLOCATIONS; i++) {
        if (!(locations[i].description.big == 0 || tkey[i] == 0)) {
            int k = tkey[i];
            if (T_TERMINATE(travel[k]))
                conditions[i] |= (1 << COND_FORCED);
        }
    }

    /*  Set up the game.atloc and game.link arrays.
     *  We'll use the DROP subroutine, which prefaces new objects on the
     *  lists.  Since we want things in the other order, we'll run the
     *  loop backwards.  If the object is in two locs, we drop it twice.
     *  Also, since two-placed objects are typically best described
     *  last, we'll drop them first. */
    for (int i = NOBJECTS; i >= 1; i--) {
        if (objects[i].fixd > 0) {
            drop(i + NOBJECTS, objects[i].fixd);
            drop(i, objects[i].plac);
        }
    }

    for (int i = 1; i <= NOBJECTS; i++) {
        int k = NOBJECTS + 1 - i;
        game.fixed[k] = objects[k].fixd;
        if (objects[k].plac != 0 && objects[k].fixd <= 0)
            drop(k, objects[k].plac);
    }

    /*  Treasure props are initially -1, and are set to 0 the first time
     *  they are described.  game.tally keeps track of how many are
     *  not yet found, so we know when to close the cave. */
    for (int treasure = 1; treasure <= NOBJECTS; treasure++) {
        if (objects[treasure].is_treasure) {
            if (objects[treasure].inventory != 0)
                game.prop[treasure] = -1;
            game.tally = game.tally - game.prop[treasure];
        }
    }
    game.conds = setbit(11);

    return seedval;
}
