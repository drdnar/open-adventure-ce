#include "actions.h"

phase_codes_t attack(command_t* command)
/*  Attack.  Assume target if unambiguous.  "Throw" also links here.
 *  Attackable objects fall into two categories: enemies (snake,
 *  dwarf, etc.)  and others (bird, clam, machine).  Ambiguous if 2
 *  enemies, or no enemies but 2 others. */
{
    int changes, i, dwarves;
    obj_t i_obj;
    verb_t verb = command->verb;
    obj_t obj = command->obj;

    if (obj == INTRANSITIVE) {
        changes = 0;
        if (atdwrf(game.loc) > 0) {
            obj = DWARF;
            ++changes;
        }
        if (here(SNAKE)) {
            obj = SNAKE;
            ++changes;
        }
        if (at(DRAGON) && game.prop[DRAGON] == DRAGON_BARS) {
            obj = DRAGON;
            ++changes;
        }
        if (at(TROLL)) {
            obj = TROLL;
            ++changes;
        }
        if (at(OGRE)) {
            obj = OGRE;
            ++changes;
        }
        if (here(BEAR) && game.prop[BEAR] == UNTAMED_BEAR) {
            obj = BEAR;
            ++changes;
        }
        /* check for low-priority targets */
        if (obj == INTRANSITIVE) {
            /* Can't attack bird or machine by throwing axe. */
            if (here(BIRD) && verb != THROW) {
                obj = BIRD;
                ++changes;
            }
            if (here(VEND) && verb != THROW) {
                obj = VEND;
                ++changes;
            }
            /* Clam and oyster both treated as clam for intransitive case;
             * no harm done. */
            if (here(CLAM) || here(OYSTER)) {
                obj = CLAM;
                ++changes;
            }
        }
        if (changes >= 2)
            return GO_UNKNOWN;
    }

    if (obj == BIRD) {
        if (game.closed) {
            rspeak(UNHAPPY_BIRD);
        } else {
            DESTROY(BIRD);
            rspeak(BIRD_DEAD);
        }
        return GO_CLEAROBJ;
    }
    if (obj == VEND) {
        state_change(VEND,
                     game.prop[VEND] == VEND_BLOCKS ? VEND_UNBLOCKS : VEND_BLOCKS);

        return GO_CLEAROBJ;
    }

    if (obj == BEAR) {
        switch (game.prop[BEAR]) {
        case UNTAMED_BEAR:
            rspeak(BEAR_HANDS);
            break;
        case SITTING_BEAR:
            rspeak(BEAR_CONFUSED);
            break;
        case CONTENTED_BEAR:
            rspeak(BEAR_CONFUSED);
            break;
        case BEAR_DEAD:
            rspeak(ALREADY_DEAD);
            break;
        }
        return GO_CLEAROBJ;
    }
    if (obj == DRAGON && game.prop[DRAGON] == DRAGON_BARS) {
        /*  Fun stuff for dragon.  If he insists on attacking it, win!
         *  Set game.prop to dead, move dragon to central loc (still
         *  fixed), move rug there (not fixed), and move him there,
         *  too.  Then do a null motion to get new description. */
        rspeak(BARE_HANDS_QUERY);
        if (!silent_yes()) {
            speak(get_arbitrary_message_index(NASTY_DRAGON));
            return GO_MOVE;
        }
        state_change(DRAGON, DRAGON_DEAD);
        game.prop[RUG] = RUG_FLOOR;
        /* Hardcoding LOC_SECRET5 as the dragon's death location is ugly.
         * The way it was computed before was worse; it depended on the
         * two dragon locations being LOC_SECRET4 and LOC_SECRET6 and
         * LOC_SECRET5 being right between them.
         */
        move(DRAGON + NOBJECTS, IS_FIXED);
        move(RUG + NOBJECTS, IS_FREE);
        move(DRAGON, LOC_SECRET5);
        move(RUG, LOC_SECRET5);
        drop(BLOOD, LOC_SECRET5);
        for (i_obj = 1; i_obj <= NOBJECTS; i_obj++) {
            if (game.place[i_obj] == get_object(DRAGON)->plac ||
                game.place[i_obj] == get_object(DRAGON)->fixd)
                move(i_obj, LOC_SECRET5);
        }
        game.loc = LOC_SECRET5;
        return GO_MOVE;
    }

    if (obj == OGRE) {
        rspeak(OGRE_DODGE);
        if (atdwrf(game.loc) == 0)
            return GO_CLEAROBJ;

        rspeak(KNIFE_THROWN);
        DESTROY(OGRE);
        dwarves = 0;
        for (i = 1; i < PIRATE; i++) {
            if (game.dloc[i] == game.loc) {
                ++dwarves;
                game.dloc[i] = LOC_LONGWEST;
                game.dseen[i] = false;
            }
        }
        rspeak((dwarves > 1) ?
               OGRE_PANIC1 :
               OGRE_PANIC2);
        return GO_CLEAROBJ;
    }

    switch (obj) {
    case INTRANSITIVE:
        rspeak(NO_TARGET);
        break;
    case CLAM:
    case OYSTER:
        rspeak(SHELL_IMPERVIOUS);
        break;
    case SNAKE:
        rspeak(SNAKE_WARNING);
        break;
    case DWARF:
        if (game.closed) {
            return GO_DWARFWAKE;
        }
        rspeak(BARE_HANDS_QUERY);
        break;
    case DRAGON:
        rspeak(ALREADY_DEAD);
        break;
    case TROLL:
        rspeak(ROCKY_TROLL);
        break;
    default:
        speak(get_action(verb)->message);
    }
    return GO_CLEAROBJ;
}

phase_codes_t bigwords(vocab_t id)
/*  FEE FIE FOE FOO (AND FUM).  Advance to next state if given in proper order.
 *  Look up foo in special section of vocab to determine which word we've got.
 *  Last word zips the eggs back to the giant room (unless already there). */
{
    if ((game.foobar == WORD_EMPTY && id == FEE) ||
        (game.foobar == FEE && id == FIE) ||
        (game.foobar == FIE && id == FOE) ||
        (game.foobar == FOE && id == FOO) ||
        (game.foobar == FOE && id == FUM)) {
        game.foobar = id;
        if ((id != FOO) && (id != FUM)) {
            rspeak(OK_MAN);
            return GO_CLEAROBJ;
        }
        game.foobar = WORD_EMPTY;
        if (game.place[EGGS] == get_object(EGGS)->plac ||
            (TOTING(EGGS) && game.loc == get_object(EGGS)->plac)) {
            rspeak(NOTHING_HAPPENS);
            return GO_CLEAROBJ;
        } else {
            /*  Bring back troll if we steal the eggs back from him before
             *  crossing. */
            if (game.place[EGGS] == LOC_NOWHERE && game.place[TROLL] == LOC_NOWHERE && game.prop[TROLL] == TROLL_UNPAID)
                game.prop[TROLL] = TROLL_PAIDONCE;
            if (here(EGGS))
                pspeak(EGGS, look, true, EGGS_VANISHED);
            else if (game.loc == get_object(EGGS)->plac)
                pspeak(EGGS, look, true, EGGS_HERE);
            else
                pspeak(EGGS, look, true, EGGS_DONE);
            move(EGGS, get_object(EGGS)->plac);

            return GO_CLEAROBJ;
        }
    } else {
        if (game.loc == LOC_GIANTROOM) {
            rspeak(START_OVER);
        } else {
            /* This is new begavior in Open Adventure - sounds better when
             * player isn't in the Giant Room. */
            rspeak(WELL_POINTLESS);
        }
        game.foobar = WORD_EMPTY;
        return GO_CLEAROBJ;
    }
}

void blast(void)
/*  Blast.  No effect unless you've got dynamite, which is a neat trick! */
{
    if (game.prop[ROD2] == STATE_NOTFOUND ||
        !game.closed)
        rspeak(REQUIRES_DYNAMITE);
    else {
        if (here(ROD2)) {
            game.bonus = splatter;
            rspeak(SPLATTER_MESSAGE);
        } else if (game.loc == LOC_NE) {
            game.bonus = defeat;
            rspeak(DEFEAT_MESSAGE);
        } else {
            game.bonus = victory;
            rspeak(VICTORY_MESSAGE);
        }
        terminate(endgame);
    }
}

phase_codes_t vbreak(verb_t verb, obj_t obj)
/*  Break.  Only works for mirror in repository and, of course, the vase. */
{
    switch (obj) {
    case MIRROR:
        if (game.closed) {
            state_change(MIRROR, MIRROR_BROKEN);
            return GO_DWARFWAKE;
        } else {
            rspeak(TOO_FAR);
            break;
        }
    case VASE:
        if (game.prop[VASE] == VASE_WHOLE) {
            if (TOTING(VASE))
                drop(VASE, game.loc);
            state_change(VASE, VASE_BROKEN);
            game.fixed[VASE] = IS_FIXED;
            break;
        }
    /* FALLTHRU */
    default:
        speak(get_action(verb)->message);
    }
    return (GO_CLEAROBJ);
}

phase_codes_t brief(void)
/*  Brief.  Intransitive only.  Suppress full descriptions after first time. */
{
    game.abbnum = 10000;
    game.detail = 3;
    rspeak(BRIEF_CONFIRM);
    return GO_CLEAROBJ;
}

phase_codes_t vcarry(verb_t verb, obj_t obj)
/*  Carry an object.  Special cases for bird and cage (if bird in cage, can't
 *  take one without the other).  Liquids also special, since they depend on
 *  status of bottle.  Also various side effects, etc. */
{
    if (obj == INTRANSITIVE) {
        /*  Carry, no object given yet.  OK if only one object present. */
        if (game.atloc[game.loc] == NO_OBJECT ||
            game.link[game.atloc[game.loc]] != 0 ||
            atdwrf(game.loc) > 0)
            return GO_UNKNOWN;
        obj = game.atloc[game.loc];
    }

    if (TOTING(obj)) {
        speak(get_action(verb)->message);
        return GO_CLEAROBJ;
    }

    if (obj == MESSAG) {
        rspeak(REMOVE_MESSAGE);
        DESTROY(MESSAG);
        return GO_CLEAROBJ;
    }

    if (game.fixed[obj] != IS_FREE) {
        switch (obj) {
        case PLANT:
            /* Next guard tests whether plant is tiny or stashed */
            rspeak(game.prop[PLANT] <= PLANT_THIRSTY ? DEEP_ROOTS : YOU_JOKING);
            break;
        case BEAR:
            rspeak( game.prop[BEAR] == SITTING_BEAR ? BEAR_CHAINED : YOU_JOKING);
            break;
        case CHAIN:
            rspeak( game.prop[BEAR] != UNTAMED_BEAR ? STILL_LOCKED : YOU_JOKING);
            break;
        case RUG:
            rspeak(game.prop[RUG] == RUG_HOVER ? RUG_HOVERS : YOU_JOKING);
            break;
        case URN:
            rspeak(URN_NOBUDGE);
            break;
        case CAVITY:
            rspeak(DOUGHNUT_HOLES);
            break;
        case BLOOD:
            rspeak(FEW_DROPS);
            break;
        case SIGN:
            rspeak(HAND_PASSTHROUGH);
            break;
        default:
            rspeak(YOU_JOKING);
        }
        return GO_CLEAROBJ;
    }

    if (obj == WATER ||
        obj == OIL) {
        if (!here(BOTTLE) ||
            liquid() != obj) {
            if (!TOTING(BOTTLE)) {
                rspeak(NO_CONTAINER);
                return GO_CLEAROBJ;
            }
            if (game.prop[BOTTLE] == EMPTY_BOTTLE) {
                return (fill(verb, BOTTLE));
            } else
                rspeak(BOTTLE_FULL);
            return GO_CLEAROBJ;
        }
        obj = BOTTLE;
    }

    if (game.holdng >= INVLIMIT) {
        rspeak(CARRY_LIMIT);
        return GO_CLEAROBJ;

    }

    if (obj == BIRD && game.prop[BIRD] != BIRD_CAGED && STASHED(BIRD) != BIRD_CAGED) {
        if (game.prop[BIRD] == BIRD_FOREST_UNCAGED) {
            DESTROY(BIRD);
            rspeak(BIRD_CRAP);
            return GO_CLEAROBJ;
        }
        if (!TOTING(CAGE)) {
            rspeak(CANNOT_CARRY);
            return GO_CLEAROBJ;
        }
        if (TOTING(ROD)) {
            rspeak(BIRD_EVADES);
            return GO_CLEAROBJ;
        }
        game.prop[BIRD] = BIRD_CAGED;
    }
    if ((obj == BIRD ||
         obj == CAGE) &&
        (game.prop[BIRD] == BIRD_CAGED || STASHED(BIRD) == BIRD_CAGED)) {
        /* expression maps BIRD to CAGE and CAGE to BIRD */
        carry(BIRD + CAGE - obj, game.loc);
    }

    carry(obj, game.loc);

    if (obj == BOTTLE && liquid() != NO_OBJECT)
        game.place[liquid()] = CARRIED;

    if (gstone(obj) && game.prop[obj] != STATE_FOUND) {
        game.prop[obj] = STATE_FOUND;
        game.prop[CAVITY] = CAVITY_EMPTY;
    }
    rspeak(OK_MAN);
    return GO_CLEAROBJ;
}

