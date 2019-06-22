#include "actions.h"

int chain(verb_t verb)
/* Do something to the bear's chain */
{
    if (verb != LOCK) {
        if (game.prop[BEAR] == UNTAMED_BEAR) {
            rspeak(BEAR_BLOCKS);
            return GO_CLEAROBJ;
        }
        if (game.prop[CHAIN] == CHAIN_HEAP) {
            rspeak(ALREADY_UNLOCKED);
            return GO_CLEAROBJ;
        }
        game.prop[CHAIN] = CHAIN_HEAP;
        game.fixed[CHAIN] = IS_FREE;
        if (game.prop[BEAR] != BEAR_DEAD)
            game.prop[BEAR] = CONTENTED_BEAR;

        switch (game.prop[BEAR]) {
        // LCOV_EXCL_START
        case BEAR_DEAD:
            /* Can't be reached until the bear can die in some way other
             * than a bridge collapse. Leave in in case this changes, but
             * exclude from coverage testing. */
            game.fixed[BEAR] = IS_FIXED;
            break;
        // LCOV_EXCL_STOP
        default:
            game.fixed[BEAR] = IS_FREE;
        }
        rspeak(CHAIN_UNLOCKED);
        return GO_CLEAROBJ;
    }

    if (game.prop[CHAIN] != CHAIN_HEAP) {
        rspeak(ALREADY_LOCKED);
        return GO_CLEAROBJ;
    }
    if (game.loc != get_object(CHAIN)->plac) {
        rspeak(NO_LOCKSITE);
        return GO_CLEAROBJ;
    }

    game.prop[CHAIN] = CHAIN_FIXED;

    if (TOTING(CHAIN))
        drop(CHAIN, game.loc);
    game.fixed[CHAIN] = IS_FIXED;

    rspeak(CHAIN_LOCKED);
    return GO_CLEAROBJ;
}

phase_codes_t discard(verb_t verb, obj_t obj)
/*  Discard object.  "Throw" also comes here for most objects.  Special cases for
 *  bird (might attack snake or dragon) and cage (might contain bird) and vase.
 *  Drop coins at vending machine for extra batteries. */
{
    int k;
    if (obj == ROD && !TOTING(ROD) && TOTING(ROD2)) {
        obj = ROD2;
    }

    if (!TOTING(obj)) {
        speak(get_action(verb)->message);
        return GO_CLEAROBJ;
    }

    if (gstone(obj) && at(CAVITY) && game.prop[CAVITY] != CAVITY_FULL) {
        rspeak(GEM_FITS);
        game.prop[obj] = STATE_IN_CAVITY;
        game.prop[CAVITY] = CAVITY_FULL;
        if (here(RUG) && ((obj == EMERALD && game.prop[RUG] != RUG_HOVER) ||
                          (obj == RUBY && game.prop[RUG] == RUG_HOVER))) {
            if (obj == RUBY)
                rspeak(RUG_SETTLES);
            else if (TOTING(RUG))
                rspeak(RUG_WIGGLES);
            else
                rspeak(RUG_RISES);
            if (!TOTING(RUG) || obj == RUBY) {
                k = (game.prop[RUG] == RUG_HOVER) ? RUG_FLOOR : RUG_HOVER;
                game.prop[RUG] = k;
                if (k == RUG_HOVER)
                    k = get_object(SAPPH)->plac;
                move(RUG + NOBJECTS, k);
            }
        }
        drop(obj, game.loc);
        return GO_CLEAROBJ;
    }

    if (obj == COINS && here(VEND)) {
        DESTROY(COINS);
        drop(BATTERY, game.loc);
        pspeak(BATTERY, look, true, FRESH_BATTERIES);
        return GO_CLEAROBJ;
    }

    if (liquid() == obj)
        obj = BOTTLE;
    if (obj == BOTTLE && liquid() != NO_OBJECT) {
        game.place[liquid()] = LOC_NOWHERE;
    }

    if (obj == BEAR && at(TROLL)) {
        state_change(TROLL, TROLL_GONE);
        move(TROLL, LOC_NOWHERE);
        move(TROLL + NOBJECTS, IS_FREE);
        move(TROLL2, get_object(TROLL)->plac);
        move(TROLL2 + NOBJECTS, get_object(TROLL)->fixd);
        juggle(CHASM);
        drop(obj, game.loc);
        return GO_CLEAROBJ;
    }

    if (obj == VASE) {
        if (game.loc != get_object(PILLOW)->plac) {
            state_change(VASE, at(PILLOW)
                         ? VASE_WHOLE
                         : VASE_DROPPED);
            if (game.prop[VASE] != VASE_WHOLE)
                game.fixed[VASE] = IS_FIXED;
            drop(obj, game.loc);
            return GO_CLEAROBJ;
        }
    }

    if (obj == CAGE && game.prop[BIRD] == BIRD_CAGED) {
        drop(BIRD, game.loc);
    }

    if (obj == BIRD) {
        if (at(DRAGON) && game.prop[DRAGON] == DRAGON_BARS) {
            rspeak(BIRD_BURNT);
            DESTROY(BIRD);
            return GO_CLEAROBJ;
        }
        if (here(SNAKE)) {
            rspeak(BIRD_ATTACKS);
            if (game.closed)
                return GO_DWARFWAKE;
            DESTROY(SNAKE);
            /* Set game.prop for use by travel options */
            game.prop[SNAKE] = SNAKE_CHASED;
        } else
            rspeak(OK_MAN);

        game.prop[BIRD] = FOREST(game.loc) ? BIRD_FOREST_UNCAGED : BIRD_UNCAGED;
        drop(obj, game.loc);
        return GO_CLEAROBJ;
    }

    rspeak(OK_MAN);
    drop(obj, game.loc);
    return GO_CLEAROBJ;
}

phase_codes_t drink(verb_t verb, obj_t obj)
/*  Drink.  If no object, assume water and look for it here.  If water is in
 *  the bottle, drink that, else must be at a water loc, so drink stream. */
{
    if (obj == INTRANSITIVE && liqloc(game.loc) != WATER &&
        (liquid() != WATER || !here(BOTTLE))) {
        return GO_UNKNOWN;
    }

    if (obj == BLOOD) {
        DESTROY(BLOOD);
        state_change(DRAGON, DRAGON_BLOODLESS);
        game.blooded = true;
        return GO_CLEAROBJ;
    }

    if (obj != INTRANSITIVE && obj != WATER) {
        rspeak(RIDICULOUS_ATTEMPT);
        return GO_CLEAROBJ;
    }
    if (liquid() == WATER && here(BOTTLE)) {
        game.place[WATER] = LOC_NOWHERE;
        state_change(BOTTLE, EMPTY_BOTTLE);
        return GO_CLEAROBJ;
    }

    speak(get_action(verb)->message);
    return GO_CLEAROBJ;
}

phase_codes_t eat(verb_t verb, obj_t obj)
/*  Eat.  Intransitive: assume food if present, else ask what.  Transitive: food
 *  ok, some things lose appetite, rest are ridiculous. */
{
    switch (obj) {
    case INTRANSITIVE:
        if (!here(FOOD))
            return GO_UNKNOWN;
    /* FALLTHRU */
    case FOOD:
        DESTROY(FOOD);
        rspeak(THANKS_DELICIOUS);
        break;
    case BIRD:
    case SNAKE:
    case CLAM:
    case OYSTER:
    case DWARF:
    case DRAGON:
    case TROLL:
    case BEAR:
    case OGRE:
        rspeak(LOST_APPETITE);
        break;
    default:
        speak(get_action(verb)->message);
    }
    return GO_CLEAROBJ;
}

phase_codes_t extinguish(verb_t verb, obj_t obj)
/* Extinguish.  Lamp, urn, dragon/volcano (nice try). */
{
    if (obj == INTRANSITIVE) {
        if (here(LAMP) && game.prop[LAMP] == LAMP_BRIGHT)
            obj = LAMP;
        if (here(URN) && game.prop[URN] == URN_LIT)
            obj = URN;
        if (obj == INTRANSITIVE)
            return GO_UNKNOWN;
    }

    switch (obj) {
    case URN:
        if (game.prop[URN] != URN_EMPTY) {
            state_change(URN, URN_DARK);
        } else {
            pspeak(URN, change, true, URN_DARK);
        }
        break;
    case LAMP:
        state_change(LAMP, LAMP_DARK);
        rspeak(dark(game.loc) ?
               PITCH_DARK :
               NO_MESSAGE);
        break;
    case DRAGON:
    case VOLCANO:
        rspeak(BEYOND_POWER);
        break;
    default:
        speak(get_action(verb)->message);
    }
    return GO_CLEAROBJ;
}

phase_codes_t feed(verb_t verb, obj_t obj)
/*  Feed.  If bird, no seed.  Snake, dragon, troll: quip.  If dwarf, make him
 *  mad.  Bear, special. */
{
    switch (obj) {
    case BIRD:
        rspeak(BIRD_PINING);
        break;
    case DRAGON:
        if (game.prop[DRAGON] != DRAGON_BARS)
            rspeak(RIDICULOUS_ATTEMPT);
        else
            rspeak(NOTHING_EDIBLE);
        break;
    case SNAKE:
        if (!game.closed && here(BIRD)) {
            DESTROY(BIRD);
            rspeak(BIRD_DEVOURED);
        } else
            rspeak(NOTHING_EDIBLE);
        break;
    case TROLL:
        rspeak(TROLL_VICES);
        break;
    case DWARF:
        if (here(FOOD)) {
            game.dflag += 2;
            rspeak(REALLY_MAD);
        } else
            speak(get_action(verb)->message);
        break;
    case BEAR:
        if (game.prop[BEAR] == BEAR_DEAD) {
            rspeak(RIDICULOUS_ATTEMPT);
            break;
        }
        if (game.prop[BEAR] == UNTAMED_BEAR) {
            if (here(FOOD)) {
                DESTROY(FOOD);
                game.fixed[AXE] = IS_FREE;
                game.prop[AXE] = AXE_HERE;
                state_change(BEAR, SITTING_BEAR);
            } else
                rspeak(NOTHING_EDIBLE);
            break;
        }
        speak(get_action(verb)->message);
        break;
    case OGRE:
        if (here(FOOD))
            rspeak(OGRE_FULL);
        else
            speak(get_action(verb)->message);
        break;
    default:
        rspeak(AM_GAME);
    }
    return GO_CLEAROBJ;
}
