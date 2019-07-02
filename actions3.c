/*
 * Actions for the dungeon-running code.
 * 
 * Zilog's C compiler is a bit . . . special, so this file is broken up into
 * smaller units.
 *
 * Copyright (c) 1977, 2005 by Will Crowther and Don Woods
 * Copyright (c) 2017 by Eric S. Raymond
 * SPDX-License-Identifier: BSD-2-clause
 */

#include "actions.h"

phase_codes_t fill(verb_t verb, obj_t obj)
/*  Fill.  Bottle or urn must be empty, and liquid available.  (Vase
 *  is nasty.) */
{
    int k;
    if (obj == VASE) {
        if (liqloc(game.loc) == NO_OBJECT) {
            rspeak(FILL_INVALID);
            return GO_CLEAROBJ;
        }
        if (!TOTING(VASE)) {
            rspeak(ARENT_CARRYING);
            return GO_CLEAROBJ;
        }
        rspeak(SHATTER_VASE);
        game.prop[VASE] = VASE_BROKEN;
        game.fixed[VASE] = IS_FIXED;
        drop(VASE, game.loc);
        return GO_CLEAROBJ;
    }

    if (obj == URN) {
        if (game.prop[URN] != URN_EMPTY) {
            rspeak(FULL_URN);
            return GO_CLEAROBJ;
        }
        if (!here(BOTTLE)) {
            rspeak(FILL_INVALID);
            return GO_CLEAROBJ;
        }
        k = liquid();
        switch (k) {
        case WATER:
            game.prop[BOTTLE] = EMPTY_BOTTLE;
            rspeak(WATER_URN);
            break;
        case OIL:
            game.prop[URN] = URN_DARK;
            game.prop[BOTTLE] = EMPTY_BOTTLE;
            rspeak(OIL_URN);
            break;
        case NO_OBJECT:
        default:
            rspeak(FILL_INVALID);
            return GO_CLEAROBJ;
        }
        game.place[k] = LOC_NOWHERE;
        return GO_CLEAROBJ;
    }
    if (obj != INTRANSITIVE && obj != BOTTLE) {
        speak(get_action(verb)->message);
        return GO_CLEAROBJ;
    }
    if (obj == INTRANSITIVE && !here(BOTTLE))
        return GO_UNKNOWN;

    if (here(URN) && game.prop[URN] != URN_EMPTY) {
        rspeak(URN_NOPOUR);
        return GO_CLEAROBJ;
    }
    if (liquid() != NO_OBJECT) {
        rspeak(BOTTLE_FULL);
        return GO_CLEAROBJ;
    }
    if (liqloc(game.loc) == NO_OBJECT) {
        rspeak(NO_LIQUID);
        return GO_CLEAROBJ;
    }
    
    state_change(BOTTLE, (liqloc(game.loc) == OIL)
                 ? OIL_BOTTLE
                 : WATER_BOTTLE);
    if (TOTING(BOTTLE))
        game.place[liquid()] = CARRIED;
    return GO_CLEAROBJ;
}

phase_codes_t find(verb_t verb, obj_t obj)
/* Find.  Might be carrying it, or it might be here.  Else give caveat. */
{
    if (TOTING(obj)) {
        rspeak(ALREADY_CARRYING);
        return GO_CLEAROBJ;
    }

    if (game.closed) {
        rspeak(NEEDED_NEARBY);
        return GO_CLEAROBJ;
    }

    if (at(obj) ||
        (liquid() == obj && at(BOTTLE)) ||
        obj == liqloc(game.loc) ||
        (obj == DWARF && atdwrf(game.loc) > 0)) {
        rspeak(YOU_HAVEIT);
        return GO_CLEAROBJ;
    }


    speak(get_action(verb)->message);
    return GO_CLEAROBJ;
}

phase_codes_t fly(verb_t verb, obj_t obj)
/* Fly.  Snide remarks unless hovering rug is here. */
{
    if (obj == INTRANSITIVE) {
        if (!here(RUG)) {
            rspeak(FLAP_ARMS);
            return GO_CLEAROBJ;
        }
        if (game.prop[RUG] != RUG_HOVER) {
            rspeak(RUG_NOTHING2);
            return GO_CLEAROBJ;
        }
        obj = RUG;
    }

    if (obj != RUG) {
        speak(get_action(verb)->message);
        return GO_CLEAROBJ;
    }
    if (game.prop[RUG] != RUG_HOVER) {
        rspeak(RUG_NOTHING1);
        return GO_CLEAROBJ;
    }
    game.oldlc2 = game.oldloc;
    game.oldloc = game.loc;

    if (game.prop[SAPPH] == STATE_NOTFOUND) {
        game.newloc = game.place[SAPPH];
        rspeak(RUG_GOES);
    } else {
        game.newloc = LOC_CLIFF;
        rspeak(RUG_RETURNS);
    }
    return GO_TERMINATE;
}

phase_codes_t inven(void)
/* Inventory. If object, treat same as find.  Else report on current burden. */
{
    obj_t i;
    bool empty = true;
    for (i = 1; i <= NOBJECTS; i++) {
        if (i == BEAR ||
            !TOTING(i))
            continue;
        if (empty) {
            rspeak(NOW_HOLDING);
            empty = false;
        }
#ifndef CALCULATOR
        pspeak(i, touch, false, -1);
#else
        pspeak(i, touch, true, -1);
#endif
    }
    if (TOTING(BEAR))
        rspeak(TAME_BEAR);
    if (empty)
        rspeak(NO_CARRY);
    return GO_CLEAROBJ;
}

phase_codes_t light(verb_t verb, obj_t obj)
/*  Light.  Applicable only to lamp and urn. */
{
    int selects;
    if (obj == INTRANSITIVE) {
        selects = 0;
        if (here(LAMP) && game.prop[LAMP] == LAMP_DARK && game.limit >= 0) {
            obj = LAMP;
            selects++;
        }
        if (here(URN) && game.prop[URN] == URN_DARK) {
            obj =  URN;
            selects++;
        }
        if (selects != 1)
            return GO_UNKNOWN;
    }

    switch (obj) {
    case URN:
        state_change(URN, game.prop[URN] == URN_EMPTY ?
                     URN_EMPTY :
                     URN_LIT);
        break;
    case LAMP:
        if (game.limit < 0) {
            rspeak(LAMP_OUT);
            break;
        }
        state_change(LAMP, LAMP_BRIGHT);
        if (game.wzdark)
            return GO_TOP;
        break;
    default:
        speak(get_action(verb)->message);
    }
    return GO_CLEAROBJ;
}

phase_codes_t listen(void)
/*  Listen.  Intransitive only.  Print stuff based on object sound proprties. */
{
    obj_t i;
    int mi;
    vocab_t sound = get_location(game.loc)->sound;
    if (sound != SILENT) {
        rspeak(sound);
        if (!get_location(game.loc)->loud)
            rspeak(NO_MESSAGE);
        return GO_CLEAROBJ;
    }
    for (i = 1; i <= NOBJECTS; i++) {
        if (!here(i) ||
            get_object_sound(i, 0) == NULL ||
            game.prop[i] < 0)
            continue;
        mi =  game.prop[i];
        /* (ESR) Some unpleasant magic on object states here. Ideally
         * we'd have liked the bird to be a normal object that we can
         * use state_change() on; can't do it, because there are
         * actually two different series of per-state birdsounds
         * depending on whether player has drunk dragon's blood. */
        if (i == BIRD)
            mi += 3 * game.blooded;
        pspeak(i, hear, true, mi, game.zzword);
        rspeak(NO_MESSAGE);
        if (i == BIRD && mi == BIRD_ENDSTATE)
            DESTROY(BIRD);
        return GO_CLEAROBJ;
    }
    rspeak(ALL_SILENT);
    return GO_CLEAROBJ;
}

phase_codes_t lock(verb_t verb, obj_t obj)
/* Lock, unlock, no object given.  Assume various things if present. */
{
    if (obj == INTRANSITIVE) {
        if (here(CLAM))
            obj = CLAM;
        if (here(OYSTER))
            obj = OYSTER;
        if (at(DOOR))
            obj = DOOR;
        if (at(GRATE))
            obj = GRATE;
        if (here(CHAIN))
            obj = CHAIN;
        if (obj == INTRANSITIVE) {
            rspeak(NOTHING_LOCKED);
            return GO_CLEAROBJ;
        }
    }

    /*  Lock, unlock object.  Special stuff for opening clam/oyster
     *  and for chain. */

    switch (obj) {
    case CHAIN:
        if (here(KEYS)) {
            return chain(verb);
        } else
            rspeak(NO_KEYS);
        break;
    case GRATE:
        if (here(KEYS)) {
            if (game.closng) {
                rspeak(EXIT_CLOSED);
                if (!game.panic)
                    game.clock2 = PANICTIME;
                game.panic = true;
            } else {
                state_change(GRATE, (verb == LOCK) ?
                             GRATE_CLOSED :
                             GRATE_OPEN);
            }
        } else
            rspeak(NO_KEYS);
        break;
    case CLAM:
        if (verb == LOCK)
            rspeak(HUH_MAN);
        else if (!TOTING(TRIDENT))
            rspeak(CLAM_OPENER);
        else {
            DESTROY(CLAM);
            drop(OYSTER, game.loc);
            drop(PEARL, LOC_CULDESAC);
            rspeak(PEARL_FALLS);
        }
        break;
    case OYSTER:
        if (verb == LOCK)
            rspeak(HUH_MAN);
        else if (TOTING(OYSTER))
            rspeak(DROP_OYSTER);
        else if (!TOTING(TRIDENT))
            rspeak(OYSTER_OPENER);
        else
            rspeak(OYSTER_OPENS);
        break;
    case DOOR:
        rspeak((game.prop[DOOR] == DOOR_UNRUSTED) ? OK_MAN : RUSTY_DOOR);
        break;
    case CAGE:
        rspeak( NO_LOCK);
        break;
    case KEYS:
        rspeak(CANNOT_UNLOCK);
        break;
    default:
        speak(get_action(verb)->message);
    }

    return GO_CLEAROBJ;
}
