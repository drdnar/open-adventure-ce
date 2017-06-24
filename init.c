#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "advent.h"
#include "database.h"

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
        if (!(locations[i].description.big == 0 || KEY[i] == 0)) {
            int k = KEY[i];
            if (MOD(labs(TRAVEL[k]), 1000) == 1)
		conditions[i] |= (1 << COND_FORCED);
        }
        game.atloc[i] = 0;
    }

    /*  Set up the game.atloc and game.link arrays as described above.
     *  We'll use the DROP subroutine, which prefaces new objects on the
     *  lists.  Since we want things in the other order, we'll run the
     *  loop backwards.  If the object is in two locs, we drop it twice.
     *  This also sets up "game.place" and "fixed" as copies of "PLAC" and
     *  "FIXD".  Also, since two-placed objects are typically best
     *  described last, we'll drop them first. */
    for (int i = 1; i <= NOBJECTS; i++) {
        int k = NOBJECTS + 1 - i;
        if (FIXD[k] > 0) {
            DROP(k + NOBJECTS, FIXD[k]);
            DROP(k, PLAC[k]);
        }
    }

    for (int i = 1; i <= NOBJECTS; i++) {
        int k = NOBJECTS + 1 - i;
        game.fixed[k] = FIXD[k];
        if (PLAC[k] != 0 && FIXD[k] <= 0)
            DROP(k, PLAC[k]);
    }

    /*  Treasure props are initially -1, and are set to 0 the first time
     *  they are described.  game.tally keeps track of how many are
     *  not yet found, so we know when to close the cave. */
    game.tally = 0;
    for (int treasure = 1; treasure <= NOBJECTS; treasure++) {
	if (object_descriptions[treasure].is_treasure) {
	    if (object_descriptions[treasure].inventory != 0)
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

    /* Define some handy mnemonics.  These correspond to object numbers. */
    AXE = VOCWRD(WORD_AXE, 1);
    BATTERY = VOCWRD(WORD_BATTERY, 1);
    BEAR = VOCWRD(WORD_BEAR, 1);
    BIRD = VOCWRD(WORD_BIRD, 1);
    BLOOD = VOCWRD(WORD_BLOOD, 1);
    BOTTLE = VOCWRD(WORD_BOTTLE, 1);
    CAGE = VOCWRD(WORD_CAGE, 1);
    CAVITY = VOCWRD(WORD_CAVITY, 1);
    CHASM = VOCWRD(WORD_CHASM, 1);
    CLAM = VOCWRD(WORD_CLAM, 1);
    DOOR = VOCWRD(WORD_DOOR, 1);
    DRAGON = VOCWRD(WORD_DRAGON, 1);
    DWARF = VOCWRD(WORD_DWARF, 1);
    FISSURE = VOCWRD(WORD_FISSURE, 1);
    FOOD = VOCWRD(WORD_FOOD, 1);
    GRATE = VOCWRD(WORD_GRATE, 1);
    KEYS = VOCWRD(WORD_KEYS, 1);
    KNIFE = VOCWRD(WORD_KNIFE, 1);
    LAMP = VOCWRD(WORD_LAMP, 1);
    MAGAZINE = VOCWRD(WORD_MAGAZINE, 1);
    MESSAG = VOCWRD(WORD_MESSAG, 1);
    MIRROR = VOCWRD(WORD_MIRROR, 1);
    OGRE = VOCWRD(WORD_OGRE, 1);
    OIL = VOCWRD(WORD_OIL, 1);
    OYSTER = VOCWRD(WORD_OYSTER, 1);
    PILLOW = VOCWRD(WORD_PILLOW, 1);
    PLANT = VOCWRD(WORD_PLANT, 1);
    PLANT2 = PLANT + 1;
    RESER = VOCWRD(WORD_RESER, 1);
    ROD = VOCWRD(WORD_ROD, 1);
    ROD2 = ROD + 1;
    SIGN = VOCWRD(WORD_SIGN, 1);
    SNAKE = VOCWRD(WORD_SNAKE, 1);
    STEPS = VOCWRD(WORD_STEPS, 1);
    TROLL = VOCWRD(WORD_TROLL, 1);
    TROLL2 = TROLL + 1;
    URN = VOCWRD(WORD_URN, 1);
    VEND = VOCWRD(WORD_VEND, 1);
    VOLCANO = VOCWRD(WORD_VOLCANO, 1);
    WATER = VOCWRD(WORD_WATER, 1);

    /* Vocabulary for treasures */
    AMBER = VOCWRD(WORD_AMBER, 1);
    CHAIN = VOCWRD(WORD_CHAIN, 1);
    CHEST = VOCWRD(WORD_CHEST, 1);
    COINS = VOCWRD(WORD_COINS, 1);
    EGGS = VOCWRD(WORD_EGGS, 1);
    EMERALD = VOCWRD(WORD_EMERALD, 1);
    JADE = VOCWRD(WORD_JADE, 1);
    NUGGET = VOCWRD(WORD_NUGGET, 1);
    PEARL = VOCWRD(WORD_PEARL, 1);
    PYRAMID = VOCWRD(WORD_PYRAMID, 1);
    RUBY = VOCWRD(WORD_RUBY, 1);
    RUG = VOCWRD(WORD_RUG, 1);
    SAPPH = VOCWRD(WORD_SAPPH, 1);
    TRIDENT = VOCWRD(WORD_TRIDENT, 1);
    VASE = VOCWRD(WORD_VASE, 1);

    /* These are motion-verb numbers. */
    BACK = VOCWRD(WORD_BACK, 0);
    CAVE = VOCWRD(WORD_CAVE, 0);
    DPRSSN = VOCWRD(WORD_DPRSSN, 0);
    ENTER = VOCWRD(WORD_ENTER, 0);
    ENTRNC = VOCWRD(WORD_ENTRNC, 0);
    LOOK = VOCWRD(WORD_LOOK, 0);
    NUL = VOCWRD(WORD_NUL, 0);
    STREAM = VOCWRD(WORD_STREAM, 0);

    /* And some action verbs. */
    FIND = VOCWRD(WORD_FIND, 2);
    INVENT = VOCWRD(WORD_INVENT, 2);
    LOCK = VOCWRD(WORD_LOCK, 2);
    SAY = VOCWRD(WORD_SAY, 2);
    THROW = VOCWRD(WORD_THROW, 2);

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

    /*  Other random flags and counters, as follows:
     *	game.abbnum	How often we should print non-abbreviated descriptions
     *	game.bonus	Used to determine amount of bonus if he reaches closing
     *	game.clock1	Number of turns from finding last treasure till closing
     *	game.clock2	Number of turns from first warning till blinding flash
     *	game.conds	Min value for cond(loc) if loc has any hints
     *	game.detail	How often we've said "not allowed to give more detail"
     *	game.dkill	# of dwarves killed (unused in scoring, needed for msg)
     *	game.foobar	Current progress in saying "FEE FIE FOE FOO".
     *	game.holdng	Number of objects being carried
     *	igo		How many times he's said "go XXX" instead of "XXX"
     *	game.iwest	How many times he's said "west" instead of "w"
     *	game.knfloc	0 if no knife here, loc if knife here, -1 after caveat
     *	game.limit	Lifetime of lamp (not set here)
     *	game.numdie	Number of times killed so far
     *	game.trnluz	# points lost so far due to number of turns used
     *	game.turns	Tallies how many commands he's given (ignores yes/no)
     */
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
    game.clock1 = 30;
    game.clock2 = 50;
    game.conds = SETBIT(11);
    game.saved = 0;
    game.closng = false;
    game.panic = false;
    game.closed = false;
    game.clshnt = false;
    game.novice = false;
    game.blklin = true;
}
