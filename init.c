#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "advent.h"
#include "database.h"
#include "newdb.h"

/*
 * Initialisation
 */

/*  Current limits:
 *     12600 words of message text (LINES, LINSIZ).
 *	885 travel options (TRAVEL, TRVSIZ).
 *	330 vocabulary words (KTAB, ATAB, TABSIZ).
 *	185 locations (LTEXT, STEXT, KEY, COND, game.abbrev, game.atloc,
 *                   LOCSND, LOCSIZ).
 *	100 objects (PLAC, game.place, FIXD, game.fixed, game.link (twice),
 *                   PTEXT, game.prop, OBJSND, OBJTXT).
 *	 35 "action" verbs (ACTSPK, VRBSIZ).
 *	277 random messages (RTEXT, RTXSIZ).
 *	 12 different player classifications (CTEXT, CVAL, CLSMAX).
 *	 20 hints (game.hintlc, game.hinted, HINTS, HNTSIZ).
 *        5 "# of turns" threshholds (TTEXT, TRNVAL, TRNSIZ).
 *  There are also limits which cannot be exceeded due to the structure of
 *  the database.  (E.G., The vocabulary uses n/1000 to determine word type,
 *  so there can't be more than 1000 words.)  These upper limits are:
 *	1000 non-synonymous vocabulary words
 *	300 locations
 *	100 objects
 *  Note:
 *   - the object count limit has been abstracted as NOBJECTS
 *   - the random message limit has been abstracted as RTXSIZ
 *   - maximum locations limit has been abstracted as LOCSIZ
 */

/*  Description of the database format
 *
 *
 *  The data file contains several sections.  Each begins with a line containing
 *  a number identifying the section, and ends with a line containing "-1".
 *
 *  Section 1: Long form descriptions.  Each line contains a location number,
 *	a tab, and a line of text.  The set of (necessarily adjacent) lines
 *	whose numbers are X form the long description of location X.
 *  Section 2: Short form descriptions.  Same format as long form.  Not all
 *	places have short descriptions.
 *  Section 3: Travel table.  Each line contains a location number (X), a second
 *	location number (Y), and a list of motion numbers (see section 4).
 *	each motion represents a verb which will go to Y if currently at X.
 *	Y, in turn, is interpreted as follows.  Let M=Y/1000, N=Y mod 1000.
 *		If N<=300	it is the location to go to.
 *		If 300<N<=500	N-300 is used in a computed goto to
 *					a section of special code.
 *		If N>500	message N-500 from section 6 is printed,
 *					and he stays wherever he is.
 *	Meanwhile, M specifies the conditions on the motion.
 *		If M=0		it's unconditional.
 *		If 0<M<100	it is done with M% probability.
 *		If M=100	unconditional, but forbidden to dwarves.
 *		If 100<M<=200	he must be carrying object M-100.
 *		If 200<M<=300	must be carrying or in same room as M-200.
 *		If 300<M<=400	game.prop(M % 100) must *not* be 0.
 *		If 400<M<=500	game.prop(M % 100) must *not* be 1.
 *		If 500<M<=600	game.prop(M % 100) must *not* be 2, etc.
 *	If the condition (if any) is not met, then the next *different*
 *	"destination" value is used (unless it fails to meet *its* conditions,
 *	in which case the next is found, etc.).  Typically, the next dest will
 *	be for one of the same verbs, so that its only use is as the alternate
 *	destination for those verbs.  For instance:
 *		15	110022	29	31	34	35	23	43
 *		15	14	29
 *	This says that, from loc 15, any of the verbs 29, 31, etc., will take
 *	him to 22 if he's carrying object 10, and otherwise will go to 14.
 *		11	303008	49
 *		11	9	50
 *	This says that, from 11, 49 takes him to 8 unless game.prop(3)=0, in which
 *	case he goes to 9.  Verb 50 takes him to 9 regardless of game.prop(3).
 *  Section 4: Vocabulary.  Each line contains a number (n), a tab, and a
 *	five-letter word.  Call M=N/1000.  If M=0, then the word is a motion
 *	verb for use in travelling (see section 3).  Else, if M=1, the word is
 *	an object.  Else, if M=2, the word is an action verb (such as "carry"
 *	or "attack").  Else, if M=3, the word is a special case verb (such as
 *	"dig") and N % 1000 is an index into section 6.  Objects from 50 to
 *	(currently, anyway) 79 are considered treasures (for pirate, closeout).
 *  Section 5: Object descriptions.  Each line contains a number (N), a tab,
 *	and a message.  If N is from 1 to 100, the message is the "inventory"
 *	message for object n.  Otherwise, N should be 000, 100, 200, etc., and
 *	the message should be the description of the preceding object when its
 *	prop value is N/100.  The N/100 is used only to distinguish multiple
 *	messages from multi-line messages; the prop info actually requires all
 *	messages for an object to be present and consecutive.  Properties which
 *	produce no message should be given the message ">$<". (The magic value
 *	100 is now mostly abstracted out as NOBJECTS.)
 *  Section 6: Arbitrary messages.  Same format as sections 1, 2, and 5, except
 *	the numbers bear no relation to anything (except for special verbs
 *	in section 4).
 *  Section 7: Object locations.  Each line contains an object number and its
 *	initial location (zero (or omitted) if none).  If the object is
 *	immovable, the location is followed by a "-1".  If it has two locations
 *	(e.g. the grate) the first location is followed with the second, and
 *	the object is assumed to be immovable.
 *  Section 8: Action defaults.  Each line contains an "action-verb" number and
 *	the index (in section 6) of the default message for the verb.
 *  Section 9: Location attributes.  Each line contains a number (n) and up to
 *	20 location numbers.  Bit N (where 0 is the units bit) is set in
 *	COND(LOC) for each loc given.  The cond bits currently assigned are:
 *		0	Light
 *		1	If bit 2 is on: on for oil, off for water
 *		2	Liquid asset, see bit 1
 *		3	Pirate doesn't go here unless following player
 *		4	Cannot use "back" to move away
 *	Bits past 10 indicate areas of interest to "hint" routines:
 *		11	Trying to get into cave
 *		12	Trying to catch bird
 *		13	Trying to deal with snake
 *		14	Lost in maze
 *		15	Pondering dark room
 *		16	At witt's end
 *		17	Cliff with urn
 *		18	Lost in forest
 *		19	Trying to deal with ogre
 *		20	Found all treasures except jade
 *	COND(LOC) is set to 2, overriding all other bits, if loc has forced
 *	motion.
 *  Section 10: Class messages.  Each line contains a number (n), a tab, and a
 *	message describing a classification of player.  The scoring section
 *	selects the appropriate message, where each message is considered to
 *	apply to players whose scores are higher than the previous N but not
 *	higher than this N.  Note that these scores probably change with every
 *	modification (and particularly expansion) of the program.
 *  Section 11: Hints.  Each line contains a hint number (add 10 to get cond
 *	bit; see section 9), the number of turns he must be at the right loc(s)
 *	before triggering the hint, the points deducted for taking the hint,
 *	the message number (section 6) of the question, and the message number
 *	of the hint.  These values are stashed in the "hints" array.  HNTMAX is
 *	set to the max hint number (<= HNTSIZ).
 *  Section 12: Unused in this version.
 *  Section 13: Sounds and text.  Each line contains either 2 or 3 numbers.  If
 *	2 (call them N and S), N is a location and message ABS(S) from section
 *	6 is the sound heard there.  If S<0, the sound there drowns out all
 *	other noises.  If 3 numbers (call them N, S, and T), N is an object
 *	number and S+game.prop(N) is the property message (from section 5) if he
 *	listens to the object, and T+game.prop(N) is the text if he reads it.  If
 *	S or T is -1, the object has no sound or text, respectively.  Neither
 *	S nor T is allowed to be 0.
 *  Section 14: Turn threshholds.  Each line contains a number (N), a tab, and
 *	a message berating the player for taking so many turns.  The messages
 *	must be in the proper (ascending) order.  The message gets printed if
 *	the player exceeds N % 100000 turns, at which time N/100000 points
 *	get deducted from his score.
 *  Section 0: End of database. */

/*  The various messages (sections 1, 2, 5, 6, etc.) may include certain
 *  special character sequences to denote that the program must provide
 *  parameters to insert into a message when the message is printed.  These
 *  sequences are:
 *	%S = The letter 'S' or nothing (if a given value is exactly 1)
 *	%W = A word (up to 10 characters)
 *	%L = A word mapped to lower-case letters
 *	%U = A word mapped to upper-case letters
 *	%C = A word mapped to lower-case, first letter capitalised
 *	%T = Several words of text, ending with a word of -1
 *	%1 = A 1-digit number
 *	%2 = A 2-digit number
 *	...
 *	%9 = A 9-digit number
 *	%B = Variable number of blanks
 *	%! = The entire message should be suppressed */

void initialise(void)
{
    if (oldstyle)
        printf("Initialising...\n");

    for (int i = 1; i <= NOBJECTS; i++) {
        game.place[i] = LOC_NOWHERE;
        game.prop[i] = 0;
        game.link[i + NOBJECTS] = game.link[i] = 0;
    }

    for (int i = 1; i <= LOCSIZ; i++) {
        game.abbrev[i] = 0;
        if (!(locations[i].description.big == 0 || KEY[i] == 0)) {
            int k = KEY[i];
            if (MOD(labs(TRAVEL[k]), 1000) == 1)COND[i] = 2;
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

    /*  Treasures, as noted earlier, are objects MINTRS through MAXTRS
     *  Their props are initially -1, and are set to 0 the first time
     *  they are described.  game.tally keeps track of how many are
     *  not yet found, so we know when to close the cave. */
    game.tally = 0;
    for (int treasure = MINTRS; treasure <= MAXTRS; treasure++) {
        if (object_descriptions[treasure].inventory != 0)
            game.prop[treasure] = -1;
        game.tally = game.tally - game.prop[treasure];
    }

    /*  Clear the hint stuff.  game.hintlc[i] is how long he's been at LOC
     *  with cond bit i.  game.hinted[i] is true iff hint i has been
     *  used. */
    for (int i = 1; i <= HNTMAX; i++) {
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

    /* Objects from MINTRS through MAXTRS are treasures.  Here are a few. */
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
     *	maximum_deaths		Number of reincarnation messages available (up to 5)
     *	game.numdie	Number of times killed so far
     *	game.thresh	Next #turns threshhold (-1 if none)
     *	game.trndex	Index in TRNVAL of next threshold (db section 14)
     *	game.trnluz	# points lost so far due to number of turns used
     *	game.turns	Tallies how many commands he's given (ignores yes/no)
     *	Logicals were explained earlier */
    game.turns = 0;
    game.trndex = 1;
    game.thresh = -1;
    if (TRNVLS > 0)
        game.thresh = MOD(TRNVAL[1], 100000) + 1;
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
