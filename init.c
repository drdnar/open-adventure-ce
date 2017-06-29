#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "advent.h"

/*
 * Initialisation
 */

void initialise(void)
{
    if (oldstyle)
        printf("Initialising...\n");

    for (int i = 1; i <= NOBJECTS; i++) {
        game.place[i] = LOC_NOWHERE;
        game.prop[i] = 0;
        game.link[i + NOBJECTS] = game.link[i] = 0;
    }

    for (int i = 1; i <= NLOCATIONS; i++) {
        game.abbrev[i] = 0;
        if (!(locations[i].description.big == 0 || tkey[i] == 0)) {
            int k = tkey[i];
            if (T_TERMINATE(travel[k]))
                conditions[i] |= (1 << COND_FORCED);
        }
        game.atloc[i] = 0;
    }

    /*  Set up the game.atloc and game.link arrays.
     *  We'll use the DROP subroutine, which prefaces new objects on the
     *  lists.  Since we want things in the other order, we'll run the
     *  loop backwards.  If the object is in two locs, we drop it twice.
     *  This also sets up "game.place" and "fixed" as copies of "PLAC" and
     *  "FIXD".  Also, since two-placed objects are typically best
     *  described last, we'll drop them first. */
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
    game.tally = 0;
    for (int treasure = 1; treasure <= NOBJECTS; treasure++) {
        if (objects[treasure].is_treasure) {
            if (objects[treasure].inventory != 0)
                game.prop[treasure] = -1;
            game.tally = game.tally - game.prop[treasure];
        }
    }

    /*  Clear the hint stuff.  game.hintlc[i] is how long he's been at LOC
     *  with cond bit i.  game.hinted[i] is true iff hint i has been
     *  used. */
    for (int i = 0; i < NHINTS; i++) {
        game.hinted[i] = false;
        game.hintlc[i] = 0;
    }

    /*  Initialise the dwarves.  game.dloc is loc of dwarves,
     *  hard-wired in.  game.odloc is prior loc of each dwarf,
     *  initially garbage.  DALTLC is alternate initial loc for dwarf,
     *  in case one of them starts out on top of the adventurer.  (No
     *  2 of the 5 initial locs are adjacent.)  game.dseen is true if
     *  dwarf has seen him.  game.dflag controls the level of
     *  activation of all this:
     *	0	No dwarf stuff yet (wait until reaches Hall Of Mists)
     *	1	Reached Hall Of Mists, but hasn't met first dwarf
     *	2	Met first dwarf, others start moving, no knives thrown yet
     *	3	A knife has been thrown (first set always misses)
     *	3+	Dwarves are mad (increases their accuracy)
     *  Sixth dwarf is special (the pirate).  He always starts at his
     *  chest's eventual location inside the maze.  This loc is saved
     *  in game.chloc for ref.  the dead end in the other maze has its
     *  loc stored in game.chloc2. */
    game.chloc = LOC_DEADEND12;
    game.chloc2 = LOC_DEADEND13;
    for (int i = 1; i <= NDWARVES; i++) {
        game.dseen[i] = false;
    }
    game.dflag = 0;
    game.dloc[1] = LOC_KINGHALL;
    game.dloc[2] = LOC_WESTBANK;
    game.dloc[3] = LOC_Y2;
    game.dloc[4] = LOC_ALIKE3;
    game.dloc[5] = LOC_COMPLEX;
    game.dloc[6] = game.chloc;

    game.turns = 0;
    game.trnluz = 0;
    game.lmwarn = false;
    game.iwest = 0;
    game.knfloc = 0;
    game.detail = 0;
    game.abbnum = 5;
    game.numdie = 0;
    game.holdng = 0;
    game.dkill = 0;
    game.foobar = 0;
    game.bonus = 0;
    game.clock1 = WARNTIME;
    game.clock2 = FLASHTIME;
    game.conds = setbit(11);
    game.saved = 0;
    game.closng = false;
    game.panic = false;
    game.closed = false;
    game.clshnt = false;
    game.novice = false;
    game.blklin = true;
}
