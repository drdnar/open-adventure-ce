#include "actions.h"

phase_codes_t pour(verb_t verb, obj_t obj)
/*  Pour.  If no object, or object is bottle, assume contents of bottle.
 *  special tests for pouring water or oil on plant or rusty door. */
{
    if (obj == BOTTLE ||
        obj == INTRANSITIVE)
        obj = LIQUID();
    if (obj == NO_OBJECT)
        return GO_UNKNOWN;
    if (!TOTING(obj)) {
        speak(get_action(verb)->message);
        return GO_CLEAROBJ;
    }

    if (obj != OIL && obj != WATER) {
        rspeak(CANT_POUR);
        return GO_CLEAROBJ;
    }
    if (HERE(URN) && game.prop[URN] == URN_EMPTY)
        return fill(verb, URN);
    game.prop[BOTTLE] = EMPTY_BOTTLE;
    game.place[obj] = LOC_NOWHERE;
    if (!(AT(PLANT) ||
          AT(DOOR))) {
        rspeak(GROUND_WET);
        return GO_CLEAROBJ;
    }
    if (!AT(DOOR)) {
        if (obj == WATER) {
            /* cycle through the three plant states */
            state_change(PLANT, MOD(game.prop[PLANT] + 1, 3));
            game.prop[PLANT2] = game.prop[PLANT];
            return GO_MOVE;
        } else {
            rspeak(SHAKING_LEAVES);
            return GO_CLEAROBJ;
        }
    } else {
        state_change(DOOR, (obj == OIL) ?
                     DOOR_UNRUSTED :
                     DOOR_RUSTED);
        return GO_CLEAROBJ;
    }
}

phase_codes_t quit(void)
/*  Quit.  Intransitive only.  Verify intent and exit if that's what he wants. */
{
    if (yes(get_arbitrary_message_index(REALLY_QUIT), get_arbitrary_message_index(OK_MAN), get_arbitrary_message_index(OK_MAN)))
        terminate(quitgame);
    return GO_CLEAROBJ;
}

phase_codes_t action_read(command_t command)
/*  Read.  Print stuff based on objtxt.  Oyster (?) is special case. */
{
    int i;
    if (command.obj == INTRANSITIVE) {
        command.obj = NO_OBJECT;
        for (i = 1; i <= NOBJECTS; i++) {
            if (HERE(i) && get_object_text(i, 0) != NULL && game.prop[i] >= 0)
                command.obj = command.obj * NOBJECTS + i;
        }
        if (command.obj > NOBJECTS ||
            command.obj == NO_OBJECT ||
            DARK(game.loc))
            return GO_UNKNOWN;
    }

    if (DARK(game.loc)) {
        sspeak(NO_SEE, command.word[0].raw);
    } else if (command.obj == OYSTER && !game.clshnt && game.closed) {
        game.clshnt = yes(get_arbitrary_message_index(CLUE_QUERY), get_arbitrary_message_index(WAYOUT_CLUE), get_arbitrary_message_index(OK_MAN));
    } else if (get_object_text(command.obj, 0) == NULL ||
               game.prop[command.obj] == STATE_NOTFOUND) {
        speak(get_action(command.verb)->message);
    } else
        pspeak(command.obj, study, true, game.prop[command.obj]);
    return GO_CLEAROBJ;
}

phase_codes_t reservoir(void)
/*  Z'ZZZ (word gets recomputed at startup; different each game). */
{
    if (!AT(RESER) && game.loc != LOC_RESBOTTOM) {
        rspeak(NOTHING_HAPPENS);
        return GO_CLEAROBJ;
    } else {
        state_change(RESER,
                     game.prop[RESER] == WATERS_PARTED ? WATERS_UNPARTED : WATERS_PARTED);
        if (AT(RESER))
            return GO_CLEAROBJ;
        else {
            game.oldlc2 = game.loc;
            game.newloc = LOC_NOWHERE;
            rspeak(NOT_BRIGHT);
            return GO_TERMINATE;
        }
    }
}

phase_codes_t rub(verb_t verb, obj_t obj)
/* Rub.  Yields various snide remarks except for lit urn. */
{
    if (obj == URN && game.prop[URN] == URN_LIT) {
        DESTROY(URN);
        drop(AMBER, game.loc);
        game.prop[AMBER] = AMBER_IN_ROCK;
        --game.tally;
        drop(CAVITY, game.loc);
        rspeak(URN_GENIES);
    } else if (obj != LAMP) {
        rspeak(PECULIAR_NOTHING);
    } else {
        speak(get_action(verb)->message);
    }
    return GO_CLEAROBJ;
}

phase_codes_t say(command_t command)
/* Say.  Echo WD2. Magic words override. */
{
    if (command.word[1].type == MOTION &&
        (command.word[1].id == XYZZY ||
         command.word[1].id == PLUGH ||
         command.word[1].id == PLOVER)) {
        return GO_WORD2;
    }
    if (command.word[1].type == ACTION && command.word[1].id == PART)
        return reservoir();

    if (command.word[1].type == ACTION &&
        (command.word[1].id == FEE ||
         command.word[1].id == FIE ||
         command.word[1].id == FOE ||
         command.word[1].id == FOO ||
         command.word[1].id == FUM ||
         command.word[1].id == PART)) {
        return bigwords(command.word[1].id);
    }
    sspeak(OKEY_DOKEY, command.word[1].raw);
    return GO_CLEAROBJ;
}

phase_codes_t throw_support(vocab_t spk)
{
    rspeak(spk);
    drop(AXE, game.loc);
    return GO_MOVE;
}

phase_codes_t throw (command_t command)
/*  Throw.  Same as discard unless axe.  Then same as attack except
 *  ignore bird, and if dwarf is present then one might be killed.
 *  (Only way to do so!)  Axe also special for dragon, bear, and
 *  troll.  Treasures special for troll. */
{
    int i;
    if (!TOTING(command.obj)) {
        speak(get_action(command.verb)->message);
        return GO_CLEAROBJ;
    }
    if (get_object(command.obj)->is_treasure && AT(TROLL)) {
        /*  Snarf a treasure for the troll. */
        drop(command.obj, LOC_NOWHERE);
        move(TROLL, LOC_NOWHERE);
        move(TROLL + NOBJECTS, IS_FREE);
        drop(TROLL2, get_object(TROLL)->plac);
        drop(TROLL2 + NOBJECTS, get_object(TROLL)->fixd);
        juggle(CHASM);
        rspeak(TROLL_SATISFIED);
        return GO_CLEAROBJ;
    }
    if (command.obj == FOOD && HERE(BEAR)) {
        /* But throwing food is another story. */
        command.obj = BEAR;
        return (feed(command.verb, command.obj));
    }
    if (command.obj != AXE)
        return (discard(command.verb, command.obj));
    else {
        if (atdwrf(game.loc) <= 0) {
            if (AT(DRAGON) && game.prop[DRAGON] == DRAGON_BARS)
                return throw_support(DRAGON_SCALES);
            if (AT(TROLL))
                return throw_support(TROLL_RETURNS);
            if (AT(OGRE))
                return throw_support(OGRE_DODGE);
            if (HERE(BEAR) && game.prop[BEAR] == UNTAMED_BEAR) {
                /* This'll teach him to throw the axe at the bear! */
                drop(AXE, game.loc);
                game.fixed[AXE] = IS_FIXED;
                juggle(BEAR);
                state_change(AXE, AXE_LOST);
                return GO_CLEAROBJ;
            }
            command.obj = INTRANSITIVE;
            return (attack(command));
        }

        if (randrange(NDWARVES + 1) < game.dflag) {
            return throw_support(DWARF_DODGES);
        } else {
            i = atdwrf(game.loc);
            game.dseen[i] = false;
            game.dloc[i] = LOC_NOWHERE;
            return throw_support((++game.dkill == 1) ?
                                 DWARF_SMOKE :
                                 KILLED_DWARF);
        }
    }
}

phase_codes_t wake(verb_t verb, obj_t obj)
/* Wake.  Only use is to disturb the dwarves. */
{
    if (obj != DWARF ||
        !game.closed) {
        speak(get_action(verb)->message);
        return GO_CLEAROBJ;
    } else {
        rspeak(PROD_DWARF);
        return GO_DWARFWAKE;
    }
}

#ifdef CALCULATOR
void print(char*);
#endif
phase_codes_t seed(verb_t verb, const char *arg)
/* Set seed */
{
    int32_t seed;
    seed = strtol(arg, NULL, 10);
    speak(get_action(verb)->message, seed);
    set_seed(seed);
    --game.turns;
    return GO_TOP;
}

