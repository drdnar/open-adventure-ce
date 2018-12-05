/*
 * Copyright (c) 1977, 2005 by Will Crowther and Don Woods
 * Copyright (c) 2017 by Eric S. Raymond
 * SPDX-License-Identifier: BSD-2-clause
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include "advent.h"
#include "dungeon.h"

#define DIM(a) (sizeof(a)/sizeof(a[0]))

// LCOV_EXCL_START
// exclude from coverage analysis because it requires interactivity to test
static void sig_handler(int signo)
{
    if (signo == SIGINT) {
        if (settings.logfp != NULL)
            fflush(settings.logfp);
    }
    exit(EXIT_FAILURE);
}
// LCOV_EXCL_STOP

/*
 * MAIN PROGRAM
 *
 *  Adventure (rev 2: 20 treasures)
 *  History: Original idea & 5-treasure version (adventures) by Willie Crowther
 *           15-treasure version (adventure) by Don Woods, April-June 1977
 *           20-treasure version (rev 2) by Don Woods, August 1978
 *		Errata fixed: 78/12/25
 *	     Revived 2017 as Open Adventure.
 */

static bool do_command(void);

int main(int argc, char *argv[])
{
    int ch;

    /*  Options. */

#ifndef ADVENT_NOSAVE
    const char* opts = "l:or:";
    const char* usage = "Usage: %s [-l logfilename] [-o] [-r restorefilename]\n";
    FILE *rfp = NULL;
#else
    const char* opts = "l:o";
    const char* usage = "Usage: %s [-l logfilename] [-o]\n";
#endif
    while ((ch = getopt(argc, argv, opts)) != EOF) {
        switch (ch) {
        case 'l':
            settings.logfp = fopen(optarg, "w");
            if (settings.logfp == NULL)
                fprintf(stderr,
                        "advent: can't open logfile %s for write\n",
                        optarg);
            signal(SIGINT, sig_handler);
            break;
        case 'o':
            settings.oldstyle = true;
            settings.prompt = false;
            break;
#ifndef ADVENT_NOSAVE
        case 'r':
            rfp = fopen(optarg, "r");
            if (rfp == NULL)
                fprintf(stderr,
                        "advent: can't open save file %s for read\n",
                        optarg);
            break;
#endif
        default:
            fprintf(stderr,
                    usage, argv[0]);
            fprintf(stderr,
                    "        -l create a log file of your game named as specified'\n");
            fprintf(stderr,
                    "        -o 'oldstyle' (no prompt, no command editing, displays 'Initialising...')\n");
#ifndef ADVENT_NOSAVE
            fprintf(stderr,
                    "        -r restore from specified saved game file\n");
#endif
            exit(EXIT_FAILURE);
            break;
        }
    }

    /*  Initialize game variables */
    long seedval = initialise();

#ifndef ADVENT_NOSAVE
    if (!rfp) {
        game.novice = yes(arbitrary_messages[WELCOME_YOU], arbitrary_messages[CAVE_NEARBY], arbitrary_messages[NO_MESSAGE]);
        if (game.novice)
            game.limit = NOVICELIMIT;
    } else {
        restore(rfp);
    }
#else
    game.novice = yes(arbitrary_messages[WELCOME_YOU], arbitrary_messages[CAVE_NEARBY], arbitrary_messages[NO_MESSAGE]);
    if (game.novice)
        game.limit = NOVICELIMIT;
#endif

    if (settings.logfp)
        fprintf(settings.logfp, "seed %ld\n", seedval);

    /* interpret commands until EOF or interrupt */
    for (;;) {
        if (!do_command())
            break;
    }
    /* show score and exit */
    terminate(quitgame);
}

/*  Check if this loc is eligible for any hints.  If been here long
 *  enough, display.  Ignore "HINTS" < 4 (special stuff, see database
 *  notes). */
static void checkhints(void)
{
    if (conditions[game.loc] >= game.conds) {
        for (int hint = 0; hint < NHINTS; hint++) {
            if (game.hinted[hint])
                continue;
            if (!CNDBIT(game.loc, hint + 1 + COND_HBASE))
                game.hintlc[hint] = -1;
            ++game.hintlc[hint];
            /*  Come here if he's been long enough at required loc(s) for some
             *  unused hint. */
            if (game.hintlc[hint] >= hints[hint].turns) {
                int i;

                switch (hint) {
                case 0:
                    /* cave */
                    if (game.prop[GRATE] == GRATE_CLOSED && !HERE(KEYS))
                        break;
                    game.hintlc[hint] = 0;
                    return;
                case 1:	/* bird */
                    if (game.place[BIRD] == game.loc && TOTING(ROD) && game.oldobj == BIRD)
                        break;
                    return;
                case 2:	/* snake */
                    if (HERE(SNAKE) && !HERE(BIRD))
                        break;
                    game.hintlc[hint] = 0;
                    return;
                case 3:	/* maze */
                    if (game.atloc[game.loc] == NO_OBJECT &&
                        game.atloc[game.oldloc] == NO_OBJECT &&
                        game.atloc[game.oldlc2] == NO_OBJECT &&
                        game.holdng > 1)
                        break;
                    game.hintlc[hint] = 0;
                    return;
                case 4:	/* dark */
                    if (game.prop[EMERALD] != STATE_NOTFOUND && game.prop[PYRAMID] == STATE_NOTFOUND)
                        break;
                    game.hintlc[hint] = 0;
                    return;
                case 5:	/* witt */
                    break;
                case 6:	/* urn */
                    if (game.dflag == 0)
                        break;
                    game.hintlc[hint] = 0;
                    return;
                case 7:	/* woods */
                    if (game.atloc[game.loc] == NO_OBJECT &&
                        game.atloc[game.oldloc] == NO_OBJECT &&
                        game.atloc[game.oldlc2] == NO_OBJECT)
                        break;
                    return;
                case 8:	/* ogre */
                    i = atdwrf(game.loc);
                    if (i < 0) {
                        game.hintlc[hint] = 0;
                        return;
                    }
                    if (HERE(OGRE) && i == 0)
                        break;
                    return;
                case 9:	/* jade */
                    if (game.tally == 1 && game.prop[JADE] < 0)
                        break;
                    game.hintlc[hint] = 0;
                    return;
                default: // LCOV_EXCL_LINE
                    BUG(HINT_NUMBER_EXCEEDS_GOTO_LIST); // LCOV_EXCL_LINE
                }

                /* Fall through to hint display */
                game.hintlc[hint] = 0;
                if (!yes(hints[hint].question, arbitrary_messages[NO_MESSAGE], arbitrary_messages[OK_MAN]))
                    return;
                rspeak(HINT_COST, hints[hint].penalty, hints[hint].penalty);
                game.hinted[hint] = yes(arbitrary_messages[WANT_HINT], hints[hint].hint, arbitrary_messages[OK_MAN]);
                if (game.hinted[hint] && game.limit > WARNTIME)
                    game.limit += WARNTIME * hints[hint].penalty;
            }
        }
    }


}

static bool spotted_by_pirate(int i)
{
    if (i != PIRATE)
        return false;

    /*  The pirate's spotted him.  Pirate leaves him alone once we've
     *  found chest.  K counts if a treasure is here.  If not, and
     *  tally=1 for an unseen chest, let the pirate be spotted.  Note
     *  that game.place[CHEST] = LOC_NOWHERE might mean that he's thrown
     *  it to the troll, but in that case he's seen the chest
     *  (game.prop[CHEST] == STATE_FOUND). */
    if (game.loc == game.chloc ||
        game.prop[CHEST] != STATE_NOTFOUND)
        return true;
    int snarfed = 0;
    bool movechest = false, robplayer = false;
    for (int treasure = 1; treasure <= NOBJECTS; treasure++) {
        if (!objects[treasure].is_treasure)
            continue;
        /*  Pirate won't take pyramid from plover room or dark
         *  room (too easy!). */
        if (treasure == PYRAMID && (game.loc == objects[PYRAMID].plac ||
                                    game.loc == objects[EMERALD].plac)) {
            continue;
        }
        if (TOTING(treasure) ||
            HERE(treasure))
            ++snarfed;
        if (TOTING(treasure)) {
            movechest = true;
            robplayer = true;
        }
    }
    /* Force chest placement before player finds last treasure */
    if (game.tally == 1 && snarfed == 0 && game.place[CHEST] == LOC_NOWHERE && HERE(LAMP) && game.prop[LAMP] == LAMP_BRIGHT) {
        rspeak(PIRATE_SPOTTED);
        movechest = true;
    }
    /* Do things in this order (chest move before robbery) so chest is listed
     * last at the maze location. */
    if (movechest) {
        move(CHEST, game.chloc);
        move(MESSAG, game.chloc2);
        game.dloc[PIRATE] = game.chloc;
        game.odloc[PIRATE] = game.chloc;
        game.dseen[PIRATE] = false;
    } else {
        /* You might get a hint of the pirate's presence even if the
         * chest doesn't move... */
        if (game.odloc[PIRATE] != game.dloc[PIRATE] && PCT(20))
            rspeak(PIRATE_RUSTLES);
    }
    if (robplayer) {
        rspeak(PIRATE_POUNCES);
        for (int treasure = 1; treasure <= NOBJECTS; treasure++) {
            if (!objects[treasure].is_treasure)
                continue;
            if (!(treasure == PYRAMID && (game.loc == objects[PYRAMID].plac ||
                                          game.loc == objects[EMERALD].plac))) {
                if (AT(treasure) && game.fixed[treasure] == IS_FREE)
                    carry(treasure, game.loc);
                if (TOTING(treasure))
                    drop(treasure, game.chloc);
            }
        }
    }

    return true;
}

static bool dwarfmove(void)
/* Dwarves move.  Return true if player survives, false if he dies. */
{
    int kk, stick, attack;
    loc_t tk[21];

    /*  Dwarf stuff.  See earlier comments for description of
     *  variables.  Remember sixth dwarf is pirate and is thus
     *  very different except for motion rules. */

    /*  First off, don't let the dwarves follow him into a pit or a
     *  wall.  Activate the whole mess the first time he gets as far
     *  as the Hall of Mists (what INDEEP() tests).  If game.newloc
     *  is forbidden to pirate (in particular, if it's beyond the
     *  troll bridge), bypass dwarf stuff.  That way pirate can't
     *  steal return toll, and dwarves can't meet the bear.  Also
     *  means dwarves won't follow him into dead end in maze, but
     *  c'est la vie.  They'll wait for him outside the dead end. */
    if (game.loc == LOC_NOWHERE ||
        FORCED(game.loc) ||
        CNDBIT(game.newloc, COND_NOARRR))
        return true;

    /* Dwarf activity level ratchets up */
    if (game.dflag == 0) {
        if (INDEEP(game.loc))
            game.dflag = 1;
        return true;
    }

    /*  When we encounter the first dwarf, we kill 0, 1, or 2 of
     *  the 5 dwarves.  If any of the survivors is at game.loc,
     *  replace him with the alternate. */
    if (game.dflag == 1) {
        if (!INDEEP(game.loc) ||
            (PCT(95) && (!CNDBIT(game.loc, COND_NOBACK) ||
                         PCT(85))))
            return true;
        game.dflag = 2;
        for (int i = 1; i <= 2; i++) {
            int j = 1 + randrange(NDWARVES - 1);
            if (PCT(50))
                game.dloc[j] = 0;
        }

        /* Alternate initial loc for dwarf, in case one of them
        *  starts out on top of the adventurer. */
        for (int i = 1; i <= NDWARVES - 1; i++) {
            if (game.dloc[i] == game.loc)
                game.dloc[i] = DALTLC; //
            game.odloc[i] = game.dloc[i];
        }
        rspeak(DWARF_RAN);
        drop(AXE, game.loc);
        return true;
    }

    /*  Things are in full swing.  Move each dwarf at random,
     *  except if he's seen us he sticks with us.  Dwarves stay
     *  deep inside.  If wandering at random, they don't back up
     *  unless there's no alternative.  If they don't have to
     *  move, they attack.  And, of course, dead dwarves don't do
     *  much of anything. */
    game.dtotal = 0;
    attack = 0;
    stick = 0;
    for (int i = 1; i <= NDWARVES; i++) {
        if (game.dloc[i] == 0)
            continue;
        /*  Fill tk array with all the places this dwarf might go. */
        unsigned int j = 1;
        kk = tkey[game.dloc[i]];
        if (kk != 0)
            do {
                enum desttype_t desttype = travel[kk].desttype;
                game.newloc = travel[kk].destval;
                /* Have we avoided a dwarf encounter? */
                if (desttype != dest_goto)
                    continue;
                else if (!INDEEP(game.newloc))
                    continue;
                else if (game.newloc == game.odloc[i])
                    continue;
                else if (j > 1 && game.newloc == tk[j - 1])
                    continue;
                else if (j >= DIM(tk) - 1)
                    /* This can't actually happen. */
                    continue; // LCOV_EXCL_LINE
                else if (game.newloc == game.dloc[i])
                    continue;
                else if (FORCED(game.newloc))
                    continue;
                else if (i == PIRATE && CNDBIT(game.newloc, COND_NOARRR))
                    continue;
                else if (travel[kk].nodwarves)
                    continue;
                tk[j++] = game.newloc;
            } while
            (!travel[kk++].stop);
        tk[j] = game.odloc[i];
        if (j >= 2)
            --j;
        j = 1 + randrange(j);
        game.odloc[i] = game.dloc[i];
        game.dloc[i] = tk[j];
        game.dseen[i] = (game.dseen[i] && INDEEP(game.loc)) ||
                        (game.dloc[i] == game.loc ||
                         game.odloc[i] == game.loc);
        if (!game.dseen[i])
            continue;
        game.dloc[i] = game.loc;
        if (spotted_by_pirate(i))
            continue;
        /* This threatening little dwarf is in the room with him! */
        ++game.dtotal;
        if (game.odloc[i] == game.dloc[i]) {
            ++attack;
            if (game.knfloc >= 0)
                game.knfloc = game.loc;
            if (randrange(1000) < 95 * (game.dflag - 2))
                ++stick;
        }
    }

    /*  Now we know what's happening.  Let's tell the poor sucker about it. */
    if (game.dtotal == 0)
        return true;
    rspeak(game.dtotal == 1 ? DWARF_SINGLE : DWARF_PACK, game.dtotal);
    if (attack == 0)
        return true;
    if (game.dflag == 2)
        game.dflag = 3;
    if (attack > 1) {
        rspeak(THROWN_KNIVES, attack);
        rspeak(stick > 1 ? MULTIPLE_HITS : (stick == 1 ? ONE_HIT : NONE_HIT), stick);
    } else {
        rspeak(KNIFE_THROWN);
        rspeak(stick ? GETS_YOU : MISSES_YOU);
    }
    if (stick == 0)
        return true;
    game.oldlc2 = game.loc;
    return false;
}

/*  "You're dead, Jim."
 *
 *  If the current loc is zero, it means the clown got himself killed.
 *  We'll allow this maxdie times.  NDEATHS is automatically set based
 *  on the number of snide messages available.  Each death results in
 *  a message (obituaries[n]) which offers reincarnation; if accepted,
 *  this results in message obituaries[0], obituaries[2], etc.  The
 *  last time, if he wants another chance, he gets a snide remark as
 *  we exit.  When reincarnated, all objects being carried get dropped
 *  at game.oldlc2 (presumably the last place prior to being killed)
 *  without change of props.  The loop runs backwards to assure that
 *  the bird is dropped before the cage.  (This kluge could be changed
 *  once we're sure all references to bird and cage are done by
 *  keywords.)  The lamp is a special case (it wouldn't do to leave it
 *  in the cave). It is turned off and left outside the building (only
 *  if he was carrying it, of course).  He himself is left inside the
 *  building (and heaven help him if he tries to xyzzy back into the
 *  cave without the lamp!).  game.oldloc is zapped so he can't just
 *  "retreat". */
static void croak(void)
/*  Okay, he's dead.  Let's get on with it. */
{
    if (game.numdie < 0)
        game.numdie = 0;  // LCOV_EXCL_LINE
    const char* query = obituaries[game.numdie].query;
    const char* yes_response = obituaries[game.numdie].yes_response;
    ++game.numdie;
    if (game.closng) {
        /*  He died during closing time.  No resurrection.  Tally up a
         *  death and exit. */
        rspeak(DEATH_CLOSING);
        terminate(endgame);
    } else if ( !yes(query, yes_response, arbitrary_messages[OK_MAN])
                || game.numdie == NDEATHS)
        terminate(endgame);
    else {
        game.place[WATER] = game.place[OIL] = LOC_NOWHERE;
        if (TOTING(LAMP))
            game.prop[LAMP] = LAMP_DARK;
        for (int j = 1; j <= NOBJECTS; j++) {
            int i = NOBJECTS + 1 - j;
            if (TOTING(i)) {
                /* Always leave lamp where it's accessible aboveground */
                drop(i, (i == LAMP) ? LOC_START : game.oldlc2);
            }
        }
        game.oldloc = game.loc = game.newloc = LOC_BUILDING;
    }
}

static void describe_location(void) {
    const char* msg = locations[game.loc].description.small;
    if (MOD(game.abbrev[game.loc], game.abbnum) == 0 ||
        msg == NO_MESSAGE)
        msg = locations[game.loc].description.big;

    if (!FORCED(game.loc) && DARK(game.loc)) {
        msg = arbitrary_messages[PITCH_DARK];
    }

    if (TOTING(BEAR))
        rspeak(TAME_BEAR);

    speak(msg);
    
    if (game.loc == LOC_Y2 && PCT(25) && !game.closng) // FIXME: magic number
        rspeak(SAYS_PLUGH);
}


static bool traveleq(int a, int b)
/* Are two travel entries equal for purposes of skip after failed condition? */
{
    return (travel[a].condtype == travel[b].condtype)
           && (travel[a].condarg1 == travel[b].condarg1)
           && (travel[a].condarg2 == travel[b].condarg2)
           && (travel[a].desttype == travel[b].desttype)
           && (travel[a].destval == travel[b].destval);
}

/*  Given the current location in "game.loc", and a motion verb number in
 *  "motion", put the new location in "game.newloc".  The current loc is saved
 *  in "game.oldloc" in case he wants to retreat.  The current
 *  game.oldloc is saved in game.oldlc2, in case he dies.  (if he
 *  does, game.newloc will be limbo, and game.oldloc will be what killed
 *  him, so we need game.oldlc2, which is the last place he was
 *  safe.) */
static void playermove(int motion)
{
    int scratchloc, travel_entry = tkey[game.loc];
    game.newloc = game.loc;
    if (travel_entry == 0)
        BUG(LOCATION_HAS_NO_TRAVEL_ENTRIES); // LCOV_EXCL_LINE
    if (motion == NUL)
        return;
    else if (motion == BACK) {
        /*  Handle "go back".  Look for verb which goes from game.loc to
         *  game.oldloc, or to game.oldlc2 If game.oldloc has forced-motion.
         *  te_tmp saves entry -> forced loc -> previous loc. */
        motion = game.oldloc;
        if (FORCED(motion))
            motion = game.oldlc2;
        game.oldlc2 = game.oldloc;
        game.oldloc = game.loc;
        if (CNDBIT(game.loc, COND_NOBACK)) {
            rspeak(TWIST_TURN);
            return;
        }
        if (motion == game.loc) {
            rspeak(FORGOT_PATH);
            return;
        }

        int te_tmp = 0;
        for (;;) {
            enum desttype_t desttype = travel[travel_entry].desttype;
            scratchloc = travel[travel_entry].destval;
            if (desttype != dest_goto || scratchloc != motion) {
                if (desttype == dest_goto) {
                    if (FORCED(scratchloc) && travel[tkey[scratchloc]].destval == motion)
                        te_tmp = travel_entry;
                }
                if (!travel[travel_entry].stop) {
                    ++travel_entry;	/* go to next travel entry for this location */
                    continue;
                }
                /* we've reached the end of travel entries for game.loc */
                travel_entry = te_tmp;
                if (travel_entry == 0) {
                    rspeak(NOT_CONNECTED);
                    return;
                }
            }

            motion = travel[travel_entry].motion;
            travel_entry = tkey[game.loc];
            break; /* fall through to ordinary travel */
        }
    } else if (motion == LOOK) {
        /*  Look.  Can't give more detail.  Pretend it wasn't dark
         *  (though it may now be dark) so he won't fall into a
         *  pit while staring into the gloom. */
        if (game.detail < 3)
            rspeak(NO_MORE_DETAIL);
        ++game.detail;
        game.wzdark = false;
        game.abbrev[game.loc] = 0;
        return;
    } else if (motion == CAVE) {
        /*  Cave.  Different messages depending on whether above ground. */
        rspeak((OUTSID(game.loc) && game.loc != LOC_GRATE) ? FOLLOW_STREAM : NEED_DETAIL);
        return;
    } else {
        /* none of the specials */
        game.oldlc2 = game.oldloc;
        game.oldloc = game.loc;
    }

    /* Look for a way to fulfil the motion verb passed in - travel_entry indexes
     * the beginning of the motion entries for here (game.loc). */
    for (;;) {
        if (T_TERMINATE(travel[travel_entry]) ||
            travel[travel_entry].motion == motion)
            break;
        if (travel[travel_entry].stop) {
            /*  Couldn't find an entry matching the motion word passed
             *  in.  Various messages depending on word given. */
            switch (motion) {
            case EAST:
            case WEST:
            case SOUTH:
            case NORTH:
            case NE:
            case NW:
            case SW:
            case SE:
            case UP:
            case DOWN:
                rspeak(BAD_DIRECTION);
                break;
            case FORWARD:
            case LEFT:
            case RIGHT:
                rspeak(UNSURE_FACING);
                break;
            case OUTSIDE:
            case INSIDE:
                rspeak(NO_INOUT_HERE);
                break;
            case XYZZY:
            case PLUGH:
                rspeak(NOTHING_HAPPENS);
                break;
            case CRAWL:
                rspeak(WHICH_WAY);
                break;
            default:
                rspeak(CANT_APPLY);
            }
            return;
        }
        ++travel_entry;
    }

    /* (ESR) We've found a destination that goes with the motion verb.
     * Next we need to check any conditional(s) on this destination, and
     * possibly on following entries. */
    do {
        for (;;) { /* L12 loop */
            for (;;) {
                enum condtype_t condtype = travel[travel_entry].condtype;
                long condarg1 = travel[travel_entry].condarg1;
                long condarg2 = travel[travel_entry].condarg2;
                if (condtype < cond_not) {
                    /* YAML N and [pct N] conditionals */
                    if (condtype == cond_goto || condtype == cond_pct) {
                        if (condarg1 == 0 ||
                            PCT(condarg1))
                            break;
                        /* else fall through */
                    }
                    /* YAML [with OBJ] clause */
                    else if (TOTING(condarg1) ||
                             (condtype == cond_with && AT(condarg1)))
                        break;
                    /* else fall through to check [not OBJ STATE] */
                } else if (game.prop[condarg1] != condarg2)
                    break;

                /* We arrive here on conditional failure.
                 * Skip to next non-matching destination */
                int te_tmp = travel_entry;
                do {
                    if (travel[te_tmp].stop)
                        BUG(CONDITIONAL_TRAVEL_ENTRY_WITH_NO_ALTERATION); // LCOV_EXCL_LINE
                    ++te_tmp;
                } while
                (traveleq(travel_entry, te_tmp));
                travel_entry = te_tmp;
            }

            /* Found an eligible rule, now execute it */
            enum desttype_t desttype = travel[travel_entry].desttype;
            game.newloc = travel[travel_entry].destval;
            if (desttype == dest_goto)
                return;

            if (desttype == dest_speak) {
                /* Execute a speak rule */
                rspeak(game.newloc);
                game.newloc = game.loc;
                return;
            } else {
                switch (game.newloc) {
                case 1:
                    /* Special travel 1.  Plover-alcove passage.  Can carry only
                     * emerald.  Note: travel table must include "useless"
                     * entries going through passage, which can never be used
                     * for actual motion, but can be spotted by "go back". */
                    game.newloc = (game.loc == LOC_PLOVER)
                                  ? LOC_ALCOVE
                                  : LOC_PLOVER;
                    if (game.holdng > 1 ||
                        (game.holdng == 1 && !TOTING(EMERALD))) {
                        game.newloc = game.loc;
                        rspeak(MUST_DROP);
                    }
                    return;
                case 2:
                    /* Special travel 2.  Plover transport.  Drop the
                     * emerald (only use special travel if toting
                     * it), so he's forced to use the plover-passage
                     * to get it out.  Having dropped it, go back and
                     * pretend he wasn't carrying it after all. */
                    drop(EMERALD, game.loc);
                    {
                        int te_tmp = travel_entry;
                        do {
                            if (travel[te_tmp].stop)
                                BUG(CONDITIONAL_TRAVEL_ENTRY_WITH_NO_ALTERATION); // LCOV_EXCL_LINE
                            ++te_tmp;
                        } while
                        (traveleq(travel_entry, te_tmp));
                        travel_entry = te_tmp;
                    }
                    continue; /* goto L12 */
                case 3:
                    /* Special travel 3.  Troll bridge.  Must be done
                     * only as special motion so that dwarves won't
                     * wander across and encounter the bear.  (They
                     * won't follow the player there because that
                     * region is forbidden to the pirate.)  If
                     * game.prop[TROLL]=TROLL_PAIDONCE, he's crossed
                     * since paying, so step out and block him.
                     * (standard travel entries check for
                     * game.prop[TROLL]=TROLL_UNPAID.)  Special stuff
                     * for bear. */
                    if (game.prop[TROLL] == TROLL_PAIDONCE) {
                        pspeak(TROLL, look, true, TROLL_PAIDONCE);
                        game.prop[TROLL] = TROLL_UNPAID;
                        move(TROLL2, LOC_NOWHERE);
                        move(TROLL2 + NOBJECTS, IS_FREE);
                        move(TROLL, objects[TROLL].plac);
                        move(TROLL + NOBJECTS, objects[TROLL].fixd);
                        juggle(CHASM);
                        game.newloc = game.loc;
                        return;
                    } else {
                        game.newloc = objects[TROLL].plac + objects[TROLL].fixd - game.loc;
                        if (game.prop[TROLL] == TROLL_UNPAID)
                            game.prop[TROLL] = TROLL_PAIDONCE;
                        if (!TOTING(BEAR))
                            return;
                        state_change(CHASM, BRIDGE_WRECKED);
                        game.prop[TROLL] = TROLL_GONE;
                        drop(BEAR, game.newloc);
                        game.fixed[BEAR] = IS_FIXED;
                        game.prop[BEAR] = BEAR_DEAD;
                        game.oldlc2 = game.newloc;
                        croak();
                        return;
                    }
                default: // LCOV_EXCL_LINE
                    BUG(SPECIAL_TRAVEL_500_GT_L_GT_300_EXCEEDS_GOTO_LIST); // LCOV_EXCL_LINE
                }
            }
            break; /* Leave L12 loop */
        }
    } while
    (false);
}

static void lampcheck(void)
/* Check game limit and lamp timers */
{
    if (game.prop[LAMP] == LAMP_BRIGHT)
        --game.limit;

    /*  Another way we can force an end to things is by having the
     *  lamp give out.  When it gets close, we come here to warn him.
     *  First following arm checks if the lamp and fresh batteries are
     *  here, in which case we replace the batteries and continue.
     *  Second is for other cases of lamp dying.  Even after it goes
     *  out, he can explore outside for a while if desired. */
    if (game.limit <= WARNTIME) {
        if (HERE(BATTERY) && game.prop[BATTERY] == FRESH_BATTERIES && HERE(LAMP)) {
            rspeak(REPLACE_BATTERIES);
            game.prop[BATTERY] = DEAD_BATTERIES;
#ifdef __unused__
            /* This code from the original game seems to have been faulty.
             * No tests ever passed the guard, and with the guard removed
             * the game hangs when the lamp limit is reached.
             */
            if (TOTING(BATTERY))
                drop(BATTERY, game.loc);
#endif
            game.limit += BATTERYLIFE;
            game.lmwarn = false;
        } else if (!game.lmwarn && HERE(LAMP)) {
            game.lmwarn = true;
            if (game.prop[BATTERY] == DEAD_BATTERIES)
                rspeak(MISSING_BATTERIES);
            else if (game.place[BATTERY] == LOC_NOWHERE)
                rspeak(LAMP_DIM);
            else
                rspeak(GET_BATTERIES);
        }
    }
    if (game.limit == 0) {
        game.limit = -1;
        game.prop[LAMP] = LAMP_DARK;
        if (HERE(LAMP))
            rspeak(LAMP_OUT);
    }
}

static bool closecheck(void)
/*  Handle the closing of the cave.  The cave closes "clock1" turns
 *  after the last treasure has been located (including the pirate's
 *  chest, which may of course never show up).  Note that the
 *  treasures need not have been taken yet, just located.  Hence
 *  clock1 must be large enough to get out of the cave (it only ticks
 *  while inside the cave).  When it hits zero, we branch to 10000 to
 *  start closing the cave, and then sit back and wait for him to try
 *  to get out.  If he doesn't within clock2 turns, we close the cave;
 *  if he does try, we assume he panics, and give him a few additional
 *  turns to get frantic before we close.  When clock2 hits zero, we
 *  transport him into the final puzzle.  Note that the puzzle depends
 *  upon all sorts of random things.  For instance, there must be no
 *  water or oil, since there are beanstalks which we don't want to be
 *  able to water, since the code can't handle it.  Also, we can have
 *  no keys, since there is a grate (having moved the fixed object!)
 *  there separating him from all the treasures.  Most of these
 *  problems arise from the use of negative prop numbers to suppress
 *  the object descriptions until he's actually moved the objects. */
{
    /* If a turn threshold has been met, apply penalties and tell
     * the player about it. */
    for (int i = 0; i < NTHRESHOLDS; ++i) {
        if (game.turns == turn_thresholds[i].threshold + 1) {
            game.trnluz += turn_thresholds[i].point_loss;
            speak(turn_thresholds[i].message);
        }
    }

    /*  Don't tick game.clock1 unless well into cave (and not at Y2). */
    if (game.tally == 0 && INDEEP(game.loc) && game.loc != LOC_Y2)
        --game.clock1;

    /*  When the first warning comes, we lock the grate, destroy
     *  the bridge, kill all the dwarves (and the pirate), remove
     *  the troll and bear (unless dead), and set "closng" to
     *  true.  Leave the dragon; too much trouble to move it.
     *  from now until clock2 runs out, he cannot unlock the
     *  grate, move to any location outside the cave, or create
     *  the bridge.  Nor can he be resurrected if he dies.  Note
     *  that the snake is already gone, since he got to the
     *  treasure accessible only via the hall of the mountain
     *  king. Also, he's been in giant room (to get eggs), so we
     *  can refer to it.  Also also, he's gotten the pearl, so we
     *  know the bivalve is an oyster.  *And*, the dwarves must
     *  have been activated, since we've found chest. */
    if (game.clock1 == 0) {
        game.prop[GRATE] = GRATE_CLOSED;
        game.prop[FISSURE] = UNBRIDGED;
        for (int i = 1; i <= NDWARVES; i++) {
            game.dseen[i] = false;
            game.dloc[i] = LOC_NOWHERE;
        }
        move(TROLL, LOC_NOWHERE);
        move(TROLL + NOBJECTS, IS_FREE);
        move(TROLL2, objects[TROLL].plac);
        move(TROLL2 + NOBJECTS, objects[TROLL].fixd);
        juggle(CHASM);
        if (game.prop[BEAR] != BEAR_DEAD)
            DESTROY(BEAR);
        game.prop[CHAIN] = CHAIN_HEAP;
        game.fixed[CHAIN] = IS_FREE;
        game.prop[AXE] = AXE_HERE;
        game.fixed[AXE] = IS_FREE;
        rspeak(CAVE_CLOSING);
        game.clock1 = -1;
        game.closng = true;
        return game.closed;
    } else if (game.clock1 < 0)
        --game.clock2;
    if (game.clock2 == 0) {
        /*  Once he's panicked, and clock2 has run out, we come here
         *  to set up the storage room.  The room has two locs,
         *  hardwired as LOC_NE and LOC_SW.  At the ne end, we
         *  place empty bottles, a nursery of plants, a bed of
         *  oysters, a pile of lamps, rods with stars, sleeping
         *  dwarves, and him.  At the sw end we place grate over
         *  treasures, snake pit, covey of caged birds, more rods, and
         *  pillows.  A mirror stretches across one wall.  Many of the
         *  objects come from known locations and/or states (e.g. the
         *  snake is known to have been destroyed and needn't be
         *  carried away from its old "place"), making the various
         *  objects be handled differently.  We also drop all other
         *  objects he might be carrying (lest he have some which
         *  could cause trouble, such as the keys).  We describe the
         *  flash of light and trundle back. */
        game.prop[BOTTLE] = put(BOTTLE, LOC_NE, EMPTY_BOTTLE);
        game.prop[PLANT] = put(PLANT, LOC_NE, PLANT_THIRSTY);
        game.prop[OYSTER] = put(OYSTER, LOC_NE, STATE_FOUND);
        game.prop[LAMP] = put(LAMP, LOC_NE, LAMP_DARK);
        game.prop[ROD] = put(ROD, LOC_NE, STATE_FOUND);
        game.prop[DWARF] = put(DWARF, LOC_NE, 0);
        game.loc = LOC_NE;
        game.oldloc = LOC_NE;
        game.newloc = LOC_NE;
        /*  Leave the grate with normal (non-negative) property.
         *  Reuse sign. */
        put(GRATE, LOC_SW, 0);
        put(SIGN, LOC_SW, 0);
        game.prop[SIGN] = ENDGAME_SIGN;
        game.prop[SNAKE] = put(SNAKE, LOC_SW, SNAKE_CHASED);
        game.prop[BIRD] = put(BIRD, LOC_SW, BIRD_CAGED);
        game.prop[CAGE] = put(CAGE, LOC_SW, STATE_FOUND);
        game.prop[ROD2] = put(ROD2, LOC_SW, STATE_FOUND);
        game.prop[PILLOW] = put(PILLOW, LOC_SW, STATE_FOUND);

        game.prop[MIRROR] = put(MIRROR, LOC_NE, STATE_FOUND);
        game.fixed[MIRROR] = LOC_SW;

        for (int i = 1; i <= NOBJECTS; i++) {
            if (TOTING(i))
                DESTROY(i);
        }

        rspeak(CAVE_CLOSED);
        game.closed = true;
        return game.closed;
    }

    lampcheck();
    return false;
}

static void listobjects(void)
/*  Print out descriptions of objects at this location.  If
 *  not closing and property value is negative, tally off
 *  another treasure.  Rug is special case; once seen, its
 *  game.prop is RUG_DRAGON (dragon on it) till dragon is killed.
 *  Similarly for chain; game.prop is initially CHAINING_BEAR (locked to
 *  bear).  These hacks are because game.prop=0 is needed to
 *  get full score. */
{
    if (!DARK(game.loc)) {
        ++game.abbrev[game.loc];
        for (int i = game.atloc[game.loc]; i != 0; i = game.link[i]) {
            obj_t obj = i;
            if (obj > NOBJECTS)
                obj = obj - NOBJECTS;
            if (obj == STEPS && TOTING(NUGGET))
                continue;
            if (game.prop[obj] < 0) {
                if (game.closed)
                    continue;
                game.prop[obj] = STATE_FOUND;
                if (obj == RUG)
                    game.prop[RUG] = RUG_DRAGON;
                if (obj == CHAIN)
                    game.prop[CHAIN] = CHAINING_BEAR;
                --game.tally;
                /*  Note: There used to be a test here to see whether the
                 *  player had blown it so badly that he could never ever see
                 *  the remaining treasures, and if so the lamp was zapped to
                 *  35 turns.  But the tests were too simple-minded; things
                 *  like killing the bird before the snake was gone (can never
                 *  see jewelry), and doing it "right" was hopeless.  E.G.,
                 *  could cross troll bridge several times, using up all
                 *  available treasures, breaking vase, using coins to buy
                 *  batteries, etc., and eventually never be able to get
                 *  across again.  If bottle were left on far side, could then
                 *  never get eggs or trident, and the effects propagate.  So
                 *  the whole thing was flushed.  anyone who makes such a
                 *  gross blunder isn't likely to find everything else anyway
                 *  (so goes the rationalisation). */
            }
            int kk = game.prop[obj];
            if (obj == STEPS)
                kk = (game.loc == game.fixed[STEPS])
                     ? STEPS_UP
                     : STEPS_DOWN;
            pspeak(obj, look, true, kk);
        }
    }
}

void clear_command(command_t *cmd)
{
    cmd->verb = ACT_NULL;
    cmd->part = unknown;
    game.oldobj = cmd->obj;
    cmd->obj = NO_OBJECT;
    cmd->state = EMPTY;
}

bool preprocess_command(command_t *command) 
/* Pre-processes a command input to see if we need to tease out a few specific cases:
 * - "enter water" or "enter stream": 
 *   wierd specific case that gets the user wet, and then kicks us back to get another command
 * - <object> <verb>:
 *   Irregular form of input, but should be allowed. We switch back to <verb> <object> form for 
 *   furtherprocessing.
 * - "grate":
 *   If in location with grate, we move to that grate. If we're in a number of other places, 
 *   we move to the entrance.
 * - "water plant", "oil plant", "water door", "oil door":
 *   Change to "pour water" or "pour oil" based on context
 * - "cage bird":
 *   If bird is present, we change to "carry bird"
 *
 * Returns true if pre-processing is complete, and we're ready to move to the primary command 
 * processing, false otherwise. */
{
    if (command->word[0].type == MOTION && command->word[0].id == ENTER
        && (command->word[1].id == STREAM || command->word[1].id == WATER)) {
        if (LIQLOC(game.loc) == WATER)
            rspeak(FEET_WET);
        else
            rspeak(WHERE_QUERY);
    } else {
        if (command->word[0].type == OBJECT) {
            /* From OV to VO form */
            if (command->word[1].type == ACTION) {
                command_word_t stage = command->word[0];
                command->word[0] = command->word[1];
                command->word[1] = stage;
            }

            if (command->word[0].id == GRATE) {
                command->word[0].type = MOTION;
                if (game.loc == LOC_START ||
                    game.loc == LOC_VALLEY ||
                    game.loc == LOC_SLIT) {
                    command->word[0].id = DEPRESSION;
                }
                if (game.loc == LOC_COBBLE ||
                    game.loc == LOC_DEBRIS ||
                    game.loc == LOC_AWKWARD ||
                    game.loc == LOC_BIRD ||
                    game.loc == LOC_PITTOP) {
                    command->word[0].id = ENTRANCE;
                }
            }
            if ((command->word[0].id == WATER || command->word[0].id == OIL) && 
                (command->word[1].id == PLANT || command->word[1].id == DOOR)) {
                if (AT(command->word[1].id)) {
                    command->word[1] = command->word[0];
                    command->word[0].id = POUR;
                    command->word[0].type = ACTION;
                    strncpy(command->word[0].raw, "pour", LINESIZE - 1);
                }
            }
            if (command->word[0].id == CAGE && command->word[1].id == BIRD && HERE(CAGE) && HERE(BIRD)) {
                command->word[0].id = CARRY;
                command->word[0].type = ACTION;
            }
        }
        command->state = PREPROCESSED;
        return true;
    }
    return false;
}

static bool do_command()
/* Get and execute a command */
{
    static command_t command;
    command.state = EMPTY;

    /*  Can't leave cave once it's closing (except by main office). */
    if (OUTSID(game.newloc) && game.newloc != 0 && game.closng) {
        rspeak(EXIT_CLOSED);
        game.newloc = game.loc;
        if (!game.panic)
            game.clock2 = PANICTIME;
        game.panic = true;
    }

    /*  See if a dwarf has seen him and has come from where he
     *  wants to go.  If so, the dwarf's blocking his way.  If
     *  coming from place forbidden to pirate (dwarves rooted in
     *  place) let him get out (and attacked). */
    if (game.newloc != game.loc && !FORCED(game.loc) && !CNDBIT(game.loc, COND_NOARRR)) {
        for (size_t i = 1; i <= NDWARVES - 1; i++) {
            if (game.odloc[i] == game.newloc && game.dseen[i]) {
                game.newloc = game.loc;
                rspeak(DWARF_BLOCK);
                break;
            }
        }
    }
    game.loc = game.newloc;

    if (!dwarfmove())
        croak();

    if (game.loc == LOC_NOWHERE) {
        croak();
    }

    /* The easiest way to get killed is to fall into a pit in
     * pitch darkness. */
    if (!FORCED(game.loc) && DARK(game.loc) && game.wzdark && PCT(35)) { // FIXME: magic number
        rspeak(PIT_FALL);
        game.oldlc2 = game.loc;
        croak();
        return true;
    }

    /* Describe the current location and (maybe) get next command. */
    while (command.state != EXECUTED) {
        describe_location();

        if (FORCED(game.loc)) {
            playermove(HERE);
            return true;
        }

        listobjects();

        /* Command not yet given; keep getting commands from user  
         * until valid command is both given and executed. */
        clear_command(&command);
        while (command.state <= GIVEN) {

            if (game.closed) {
            /*  If closing time, check for any objects being toted with
             *  game.prop < 0 and stash them.  This way objects won't be
             *  described until they've been picked up and put down
             *  separate from their respective piles. */
                if (game.prop[OYSTER] < 0 && TOTING(OYSTER))
                    pspeak(OYSTER, look, true, 1);
                for (size_t i = 1; i <= NOBJECTS; i++) {
                    if (TOTING(i) && game.prop[i] < 0)
                        game.prop[i] = STASHED(i);
                }
            }

            /* Check to see if the room is dark. If the knife is here, 
             * and it's dark, the knife permanently disappears */
            game.wzdark = DARK(game.loc);
            if (game.knfloc != LOC_NOWHERE && game.knfloc != game.loc)
                game.knfloc = LOC_NOWHERE;

            /* Check some for hints, get input from user, increment
             * turn, and pre-process commands. Keep going until
             * pre-processing is done. */
            while ( command.state < PREPROCESSED ) {
                checkhints();

                /* Get command input from user */
                if (!get_command_input(&command))
                    return false;

                ++game.turns;
                preprocess_command(&command);
            }

            /* check if game is closed, and exit if it is */
            if (closecheck() ) 
                return true;

            /* loop until all words in command are procesed */
            while (command.state == PREPROCESSED ) {
                command.state = PROCESSING;

                if (command.word[0].id == WORD_NOT_FOUND) {
                    /* Gee, I don't understand. */
                    sspeak(DONT_KNOW, command.word[0].raw);
                    clear_command(&command);
                    continue;
                }

                /* Give user hints of shortcuts */
                if (strncasecmp(command.word[0].raw, "west", sizeof("west")) == 0) {
                    if (++game.iwest == 10)
                        rspeak(W_IS_WEST);
                }
                if (strncasecmp(command.word[0].raw, "go", sizeof("go")) == 0 && command.word[1].id != WORD_EMPTY) {
                    if (++game.igo == 10)
                        rspeak(GO_UNNEEDED);
                }

                switch (command.word[0].type) {
                case NO_WORD_TYPE: // FIXME: treating NO_WORD_TYPE as a motion word is confusing
                case MOTION:
                    playermove(command.word[0].id);
                    command.state = EXECUTED;
                    continue;
                case OBJECT:
                    command.part = unknown;
                    command.obj = command.word[0].id;
                    break;
                case ACTION:
                    if (command.word[1].type == NUMERIC)
                        command.part = transitive;
                    else
                        command.part = intransitive;
                    command.verb = command.word[0].id;
                    break;
                case NUMERIC:
                    if (!settings.oldstyle) {
                        sspeak(DONT_KNOW, command.word[0].raw);
                        clear_command(&command);
                        continue;
                    }
                    break;
                default: // LCOV_EXCL_LINE
                    BUG(VOCABULARY_TYPE_N_OVER_1000_NOT_BETWEEN_0_AND_3); // LCOV_EXCL_LINE
                }

                switch (action(command)) {
                case GO_TERMINATE:
                    command.state = EXECUTED;
                    break;
                case GO_MOVE:
                    playermove(NUL);
                    command.state = EXECUTED;
                    break;
                case GO_WORD2:
#ifdef GDEBUG
                    printf("Word shift\n");
#endif /* GDEBUG */
                    /* Get second word for analysis. */
                    command.word[0] = command.word[1];
                    command.word[1] = empty_command_word;
                    command.state = PREPROCESSED;
                    break;
                case GO_UNKNOWN:
                    /*  Random intransitive verbs come here.  Clear obj just in case
                     *  (see attack()). */
                    command.word[0].raw[0] = toupper(command.word[0].raw[0]);
                    sspeak(DO_WHAT, command.word[0].raw);
                    command.obj = NO_OBJECT;

                    /* object cleared; we need to go back to the preprocessing step */
                    command.state = GIVEN;
                    break;
                case GO_DWARFWAKE:
                    /*  Oh dear, he's disturbed the dwarves. */
                    rspeak(DWARVES_AWAKEN);
                    terminate(endgame);
                case GO_CLEAROBJ: // FIXME: re-name to be more contextual; this was previously a label
                    clear_command(&command);
                    break;
                case GO_TOP: // FIXME: re-name to be more contextual; this was previously a label
                    break;
                case GO_CHECKHINT: // FIXME: re-name to be more contextual; this was previously a label
                    command.state = GIVEN;
                    break;
                default: // LCOV_EXCL_LINE
                    BUG(ACTION_RETURNED_PHASE_CODE_BEYOND_END_OF_SWITCH); // LCOV_EXCL_LINE
                }
            } /* while command has nob been fully processed */
        } /* while command is not yet given */
    } /* while command is not executed */

    /* command completely executed; we return true. */
    return true;
}

/* end */
