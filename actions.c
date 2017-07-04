#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "advent.h"
#include "dungeon.h"

static int fill(token_t, token_t);

static void state_change(long obj, long state)
{
    game.prop[obj] = state;
    pspeak(obj, change, state, true);
}

static int attack(struct command_t *command)
/*  Attack.  Assume target if unambiguous.  "Throw" also links here.
 *  Attackable objects fall into two categories: enemies (snake,
 *  dwarf, etc.)  and others (bird, clam, machine).  Ambiguous if 2
 *  enemies, or no enemies but 2 others. */
{
    vocab_t verb = command->verb;
    vocab_t obj = command->obj;

    if (obj == NO_OBJECT ||
        obj == INTRANSITIVE) {
        int changes = 0;
        if (atdwrf(game.loc) > 0) {
            obj = DWARF;
            ++changes;
        }
        if (HERE(SNAKE)) {
            obj = SNAKE;
            ++changes;
        }
        if (AT(DRAGON) && game.prop[DRAGON] == DRAGON_BARS) {
            obj = DRAGON;
            ++changes;
        }
        if (AT(TROLL)) {
            obj = TROLL;
            ++changes;
        }
        if (AT(OGRE)) {
            obj = OGRE;
            ++changes;
        }
        if (HERE(BEAR) && game.prop[BEAR] == UNTAMED_BEAR) {
            obj = BEAR;
            ++changes;
        }
        /* check for low-priority targets */
        if (obj == NO_OBJECT) {
            /* Can't attack bird or machine by throwing axe. */
            if (HERE(BIRD) && verb != THROW) {
                obj = BIRD;
                ++changes;
            }
            if (HERE(VEND) && verb != THROW) {
                obj = VEND;
                ++changes;
            }
            /* Clam and oyster both treated as clam for intransitive case;
             * no harm done. */
            if (HERE(CLAM) || HERE(OYSTER)) {
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
        if (silent_yes()) {
            // FIXME: setting wd1 is a workaround for broken logic
            command->wd1 = token_to_packed("Y");
        } else {
            // FIXME: setting wd1 is a workaround for broken logic
            command->wd1 = token_to_packed("N");
            return GO_CHECKFOO;
        }
        state_change(DRAGON, DRAGON_DEAD);
        game.prop[RUG] = RUG_FLOOR;
        /* FIXME: Arithmetic on location values */
        int k = (objects[DRAGON].plac + objects[DRAGON].fixd) / 2;
        move(DRAGON + NOBJECTS, -1);
        move(RUG + NOBJECTS, 0);
        move(DRAGON, k);
        move(RUG, k);
        drop(BLOOD, k);
        for (obj = 1; obj <= NOBJECTS; obj++) {
            if (game.place[obj] == objects[DRAGON].plac ||
                game.place[obj] == objects[DRAGON].fixd)
                move(obj, k);
        }
        game.loc = k;
        return GO_MOVE;
    }

    if (obj == OGRE) {
        rspeak(OGRE_DODGE);
        if (atdwrf(game.loc) == 0)
            return GO_CLEAROBJ;

        rspeak(KNIFE_THROWN);
        DESTROY(OGRE);
        int dwarves = 0;
        for (int i = 1; i < PIRATE; i++) {
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
    case NO_OBJECT:
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
        rspeak(actions[verb].message);
    }
    return GO_CLEAROBJ;
}

static int bigwords(token_t foo)
/*  FEE FIE FOE FOO (AND FUM).  Advance to next state if given in proper order.
 *  Look up foo in special section of vocab to determine which word we've got.
 *  Last word zips the eggs back to the giant room (unless already there). */
{
    char word[TOKLEN + 1];
    packed_to_token(foo, word);
    int k = (int) get_special_vocab_id(word);
    if (game.foobar != 1 - k) {
        if (game.foobar != 0 && game.loc == LOC_GIANTROOM) {
            rspeak( START_OVER);
        } else {
            rspeak(NOTHING_HAPPENS);
        }
        return GO_CLEAROBJ;
    } else {
        game.foobar = k;
        if (k != 4) {
            rspeak(OK_MAN);
            return GO_CLEAROBJ;
        }
        game.foobar = 0;
        if (game.place[EGGS] == objects[EGGS].plac ||
            (TOTING(EGGS) && game.loc == objects[EGGS].plac)) {
            rspeak(NOTHING_HAPPENS);
            return GO_CLEAROBJ;
        } else {
            /*  Bring back troll if we steal the eggs back from him before
             *  crossing. */
            if (game.place[EGGS] == LOC_NOWHERE && game.place[TROLL] == LOC_NOWHERE && game.prop[TROLL] == TROLL_UNPAID)
                game.prop[TROLL] = TROLL_PAIDONCE;
            k = EGGS_DONE;
            if (HERE(EGGS))
                k = EGGS_VANISHED;
            if (game.loc == objects[EGGS].plac)
                k = EGGS_HERE;
            move(EGGS, objects[EGGS].plac);
            pspeak(EGGS, look, k, true);
            return GO_CLEAROBJ;
        }
    }
}

static void blast(void)
/*  Blast.  No effect unless you've got dynamite, which is a neat trick! */
{
    if (game.prop[ROD2] < 0 ||
        !game.closed)
        rspeak(REQUIRES_DYNAMITE);
    else {
        game.bonus = VICTORY_MESSAGE;
        if (game.loc == LOC_NE)
            game.bonus = DEFEAT_MESSAGE;
        if (HERE(ROD2))
            game.bonus = SPLATTER_MESSAGE;
        rspeak(game.bonus);
        terminate(endgame);
    }
}

static int vbreak(token_t verb, token_t obj)
/*  Break.  Only works for mirror in repository and, of course, the vase. */
{
    if (obj == MIRROR) {
        if (game.closed) {
            rspeak(BREAK_MIRROR);
            return GO_DWARFWAKE;
        } else {
            rspeak(TOO_FAR);
            return GO_CLEAROBJ;
        }
    }
    if (obj == VASE && game.prop[VASE] == VASE_WHOLE) {
        if (TOTING(VASE))
            drop(VASE, game.loc);
        state_change(VASE, VASE_BROKEN);
        game.fixed[VASE] = -1;
        return GO_CLEAROBJ;
    }
    rspeak(actions[verb].message);
    return (GO_CLEAROBJ);
}

static int brief(void)
/*  Brief.  Intransitive only.  Suppress long descriptions after first time. */
{
    game.abbnum = 10000;
    game.detail = 3;
    rspeak(BRIEF_CONFIRM);
    return GO_CLEAROBJ;
}

static int vcarry(token_t verb, token_t obj)
/*  Carry an object.  Special cases for bird and cage (if bird in cage, can't
 *  take one without the other).  Liquids also special, since they depend on
 *  status of bottle.  Also various side effects, etc. */
{
    if (obj == INTRANSITIVE) {
        /*  Carry, no object given yet.  OK if only one object present. */
        if (game.atloc[game.loc] == 0 ||
            game.link[game.atloc[game.loc]] != 0 ||
            atdwrf(game.loc) > 0)
            return GO_UNKNOWN;
        obj = game.atloc[game.loc];
    }

    if (TOTING(obj)) {
        rspeak(ALREADY_CARRYING);
        return GO_CLEAROBJ;
    }

    if (obj == MESSAG) {
        rspeak(REMOVE_MESSAGE);
        DESTROY(MESSAG);
        return GO_CLEAROBJ;
    }

    if (game.fixed[obj] != 0) {
        if (obj == PLANT && game.prop[PLANT] <= 0) {
            rspeak(DEEP_ROOTS);
            return GO_CLEAROBJ;
        }
        if (obj == BEAR && game.prop[BEAR] == SITTING_BEAR) {
            rspeak(BEAR_CHAINED);
            return GO_CLEAROBJ;
        }
        if (obj == CHAIN && game.prop[BEAR] != UNTAMED_BEAR) {
            rspeak(STILL_LOCKED);
            return GO_CLEAROBJ;
        }
        if (obj == URN) {
            rspeak(URN_NOBUDGE);
            return GO_CLEAROBJ;
        }
        if (obj == CAVITY) {
            rspeak(DOUGHNUT_HOLES);
            return GO_CLEAROBJ;
        }
        if (obj == BLOOD) {
            rspeak(FEW_DROPS);
            return GO_CLEAROBJ;
        }
        if (obj == RUG && game.prop[RUG] == RUG_HOVER) {
            rspeak(RUG_HOVERS);
            return GO_CLEAROBJ;
        }
        if (obj == SIGN) {
            rspeak(HAND_PASSTHROUGH);
            return GO_CLEAROBJ;
        }
        rspeak(YOU_JOKING);
        return GO_CLEAROBJ;
    }

    if (obj == WATER ||
        obj == OIL) {
        if (!HERE(BOTTLE) ||
            LIQUID() != obj) {
            if (TOTING(BOTTLE)) {
                if (game.prop[BOTTLE] == EMPTY_BOTTLE) {
                    return (fill(verb, BOTTLE));
                } else if (game.prop[BOTTLE] != EMPTY_BOTTLE)
                    rspeak(BOTTLE_FULL);
                return GO_CLEAROBJ;
            }
            rspeak(NO_CONTAINER);
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
    /* FIXME: Arithmetic on state numbers */
    if ((obj == BIRD ||
         obj == CAGE) &&
        (game.prop[BIRD] == BIRD_CAGED || STASHED(BIRD) == BIRD_CAGED))
        carry(BIRD + CAGE - obj, game.loc);
    carry(obj, game.loc);
    if (obj == BOTTLE && LIQUID() != NO_OBJECT)
        game.place[LIQUID()] = CARRIED;
    if (GSTONE(obj) && game.prop[obj] != 0) {
        game.prop[obj]
            = STATE_GROUND;
        game.prop[CAVITY] = CAVITY_EMPTY;
    }
    rspeak(OK_MAN);
    return GO_CLEAROBJ;
}

static int chain(token_t verb)
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
        game.fixed[CHAIN] = CHAIN_HEAP;
        if (game.prop[BEAR] != BEAR_DEAD)
            game.prop[BEAR] = CONTENTED_BEAR;

        switch (game.prop[BEAR]) {
        case BEAR_DEAD:
            game.fixed[BEAR] = -1;
            break;
        default:
            game.fixed[BEAR] = 0;
        }
        rspeak(CHAIN_UNLOCKED);
        return GO_CLEAROBJ;
    }

    if (game.prop[CHAIN] != CHAIN_HEAP) {
        rspeak(ALREADY_LOCKED);
        return GO_CLEAROBJ;
    }
    if (game.loc != objects[CHAIN].plac) {
        rspeak(NO_LOCKSITE);
        return GO_CLEAROBJ;
    }

    game.prop[CHAIN] = CHAIN_FIXED;

    if (TOTING(CHAIN))
        drop(CHAIN, game.loc);
    game.fixed[CHAIN] = -1;

    rspeak(CHAIN_LOCKED);
    return GO_CLEAROBJ;
}

static int discard(token_t verb, token_t obj, bool just_do_it)
/*  Discard object.  "Throw" also comes here for most objects.  Special cases for
 *  bird (might attack snake or dragon) and cage (might contain bird) and vase.
 *  Drop coins at vending machine for extra batteries. */
{
    if (!just_do_it) {
        if (TOTING(ROD2) && obj == ROD && !TOTING(ROD))
            obj = ROD2;
        if (!TOTING(obj)) {
            rspeak(actions[verb].message);
            return GO_CLEAROBJ;
        }
        if (obj == BIRD && HERE(SNAKE)) {
            rspeak(BIRD_ATTACKS);
            if (game.closed)
                return GO_DWARFWAKE;
            DESTROY(SNAKE);
            /* Set game.prop for use by travel options */
            game.prop[SNAKE] = SNAKE_CHASED;

        } else if ((GSTONE(obj) && AT(CAVITY) && game.prop[CAVITY] != CAVITY_FULL)) {
            rspeak(GEM_FITS);
            game.prop[obj] = 1;
            game.prop[CAVITY] = CAVITY_FULL;
            if (HERE(RUG) && ((obj == EMERALD && game.prop[RUG] != RUG_HOVER) ||
                              (obj == RUBY && game.prop[RUG] == RUG_HOVER))) {
                int spk = RUG_RISES;
                if (TOTING(RUG))
                    spk = RUG_WIGGLES;
                if (obj == RUBY)
                    spk = RUG_SETTLES;
                rspeak(spk);
                if (spk != RUG_WIGGLES) {
                    /* FIXME: Arithmetic on state numbers */
                    int k = 2 - game.prop[RUG];
                    game.prop[RUG] = k;
                    if (k == 2)
                        k = objects[SAPPH].plac;
                    move(RUG + NOBJECTS, k);
                }
            }
        } else if (obj == COINS && HERE(VEND)) {
            DESTROY(COINS);
            drop(BATTERY, game.loc);
            pspeak(BATTERY, look, FRESH_BATTERIES, true);
            return GO_CLEAROBJ;
        } else if (obj == BIRD && AT(DRAGON) && game.prop[DRAGON] == DRAGON_BARS) {
            rspeak(BIRD_BURNT);
            DESTROY(BIRD);
            return GO_CLEAROBJ;
        } else if (obj == BEAR && AT(TROLL)) {
            state_change(TROLL, TROLL_GONE);
            move(TROLL, LOC_NOWHERE);
            move(TROLL + NOBJECTS, LOC_NOWHERE);
            move(TROLL2, objects[TROLL].plac);
            move(TROLL2 + NOBJECTS, objects[TROLL].fixd);
            juggle(CHASM);
        } else if (obj != VASE ||
                   game.loc == objects[PILLOW].plac) {
            rspeak(OK_MAN);
        } else {
            game.prop[VASE] = VASE_BROKEN;
            if (AT(PILLOW))
                game.prop[VASE] = VASE_WHOLE;
            pspeak(VASE, look, game.prop[VASE] + 1, true);
            if (game.prop[VASE] != VASE_WHOLE)
                game.fixed[VASE] = -1;
        }
    }
    int k = LIQUID();
    if (k == obj)
        obj = BOTTLE;
    if (obj == BOTTLE && k != NO_OBJECT)
        game.place[k] = LOC_NOWHERE;
    if (obj == CAGE && game.prop[BIRD] == BIRD_CAGED)
        drop(BIRD, game.loc);
    drop(obj, game.loc);
    if (obj != BIRD)
        return GO_CLEAROBJ;
    game.prop[BIRD] = BIRD_UNCAGED;
    if (FOREST(game.loc))
        game.prop[BIRD] = BIRD_FOREST_UNCAGED;
    return GO_CLEAROBJ;
}

static int drink(token_t verb, token_t obj)
/*  Drink.  If no object, assume water and look for it here.  If water is in
 *  the bottle, drink that, else must be at a water loc, so drink stream. */
{
    if (obj == NO_OBJECT && LIQLOC(game.loc) != WATER &&
        (LIQUID() != WATER || !HERE(BOTTLE))) {
        return GO_UNKNOWN;
    }

    if (obj == BLOOD) {
        DESTROY(BLOOD);
        state_change(DRAGON, DRAGON_BLOODLESS);
        game.blooded = true;
        return GO_CLEAROBJ;
    }

    if (obj != NO_OBJECT && obj != WATER) {
        rspeak(RIDICULOUS_ATTEMPT);
        return GO_CLEAROBJ;
    }
    if (LIQUID() == WATER && HERE(BOTTLE)) {
        game.prop[BOTTLE] = EMPTY_BOTTLE;
        game.place[WATER] = LOC_NOWHERE;
        rspeak(BOTTLE_EMPTY);
        return GO_CLEAROBJ;
    }

    rspeak(actions[verb].message);
    return GO_CLEAROBJ;
}

static int eat(token_t verb, token_t obj)
/*  Eat.  Intransitive: assume food if present, else ask what.  Transitive: food
 *  ok, some things lose appetite, rest are ridiculous. */
{
    if (obj == INTRANSITIVE) {
        if (!HERE(FOOD))
            return GO_UNKNOWN;
        DESTROY(FOOD);
        rspeak(THANKS_DELICIOUS);
        return GO_CLEAROBJ;
    }
    if (obj == FOOD) {
        DESTROY(FOOD);
        rspeak(THANKS_DELICIOUS);
        return GO_CLEAROBJ;
    }
    if (obj == BIRD ||
        obj == SNAKE ||
        obj == CLAM ||
        obj == OYSTER ||
        obj == DWARF ||
        obj == DRAGON ||
        obj == TROLL ||
        obj == BEAR ||
        obj == OGRE) {
        rspeak(LOST_APPETITE);
        return GO_CLEAROBJ;
    }
    rspeak(actions[verb].message);
    return GO_CLEAROBJ;
}

static int extinguish(token_t verb, int obj)
/* Extinguish.  Lamp, urn, dragon/volcano (nice try). */
{
    if (obj == INTRANSITIVE) {
        if (HERE(LAMP) && game.prop[LAMP] == LAMP_BRIGHT)
            obj = LAMP;
        if (HERE(URN) && game.prop[URN] == URN_LIT)
            obj = URN;
        if (obj == INTRANSITIVE)
            return GO_UNKNOWN;
    }

    if (obj == URN) {
        if (game.prop[URN] != URN_EMPTY) {
            state_change(URN, URN_DARK);
        } else {
            pspeak(URN, change, URN_DARK, true);
        }
        return GO_CLEAROBJ;
    }

    if (obj == LAMP) {
        state_change(LAMP, LAMP_DARK);
        rspeak(DARK(game.loc) ?
               PITCH_DARK :
               NO_MESSAGE);
        return GO_CLEAROBJ;
    }

    if (obj == DRAGON ||
        obj == VOLCANO) {
        rspeak(BEYOND_POWER);
        return GO_CLEAROBJ;
    }

    rspeak(actions[verb].message);
    return GO_CLEAROBJ;
}

static int feed(token_t verb, token_t obj)
/*  Feed.  If bird, no seed.  Snake, dragon, troll: quip.  If dwarf, make him
 *  mad.  Bear, special. */
{
    int spk = actions[verb].message;
    if (obj == BIRD) {
        rspeak(BIRD_PINING);
        return GO_CLEAROBJ;
    } else if (obj == SNAKE ||
               obj == DRAGON ||
               obj == TROLL) {
        spk = NOTHING_EDIBLE;
        if (obj == DRAGON && game.prop[DRAGON] != DRAGON_BARS)
            spk = RIDICULOUS_ATTEMPT;
        if (obj == TROLL)
            spk = TROLL_VICES;
        if (obj == SNAKE && !game.closed && HERE(BIRD)) {
            DESTROY(BIRD);
            spk = BIRD_DEVOURED;
        }
    } else if (obj == DWARF) {
        if (HERE(FOOD)) {
            game.dflag += 2;
            spk = REALLY_MAD;
        }
    } else if (obj == BEAR) {
        if (game.prop[BEAR] == UNTAMED_BEAR)
            spk = NOTHING_EDIBLE;
        if (game.prop[BEAR] == BEAR_DEAD)
            spk = RIDICULOUS_ATTEMPT;
        if (HERE(FOOD)) {
            DESTROY(FOOD);
            game.prop[BEAR] = SITTING_BEAR;
            game.fixed[AXE] = 0;
            game.prop[AXE] = 0;
            spk = BEAR_TAMED;
        }
    } else if (obj == OGRE) {
        if (HERE(FOOD))
            spk = OGRE_FULL;
    } else {
        spk = AM_GAME;
    }
    rspeak(spk);
    return GO_CLEAROBJ;
}

int fill(token_t verb, token_t obj)
/*  Fill.  Bottle or urn must be empty, and liquid available.  (Vase
 *  is nasty.) */
{
    if (obj == VASE) {
        if (LIQLOC(game.loc) == NO_OBJECT) {
            rspeak(FILL_INVALID);
            return GO_CLEAROBJ;
        }
        if (!TOTING(VASE)) {
            rspeak(ARENT_CARRYING);
            return GO_CLEAROBJ;
        }
        rspeak(SHATTER_VASE);
        game.prop[VASE] = VASE_BROKEN;
        game.fixed[VASE] = -1;
        return (discard(verb, VASE, true));
    }

    if (obj == URN) {
        if (game.prop[URN] != URN_EMPTY) {
            rspeak(FULL_URN);
            return GO_CLEAROBJ;
        }
        if (!HERE(BOTTLE)) {
            rspeak(FILL_INVALID);
            return GO_CLEAROBJ;
        }
        int k = LIQUID();
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
    if (obj != NO_OBJECT && obj != BOTTLE) {
        rspeak(actions[verb].message);
        return GO_CLEAROBJ;
    }
    if (obj == NO_OBJECT && !HERE(BOTTLE))
        return GO_UNKNOWN;

    if (HERE(URN) && game.prop[URN] != URN_EMPTY) {
        rspeak(URN_NOPOUR);
        return GO_CLEAROBJ;
    }
    if (LIQUID() != NO_OBJECT) {
        rspeak(BOTTLE_FULL);
        return GO_CLEAROBJ;
    }
    if (LIQLOC(game.loc) == NO_OBJECT) {
        rspeak(NO_LIQUID);
        return GO_CLEAROBJ;
    }

    game.prop[BOTTLE] = (LIQLOC(game.loc) == OIL) ? OIL_BOTTLE : WATER_BOTTLE;
    if (TOTING(BOTTLE))
        game.place[LIQUID()] = CARRIED;
    if (LIQUID() == OIL)
        rspeak(BOTTLED_OIL);
    else
        rspeak(BOTTLED_WATER);
    return GO_CLEAROBJ;
}

static int find(token_t verb, token_t obj)
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

    if (AT(obj) ||
        (LIQUID() == obj && AT(BOTTLE)) ||
        obj == LIQLOC(game.loc) ||
        (obj == DWARF && atdwrf(game.loc) > 0)) {
        rspeak(YOU_HAVEIT);
        return GO_CLEAROBJ;
    }


    rspeak(actions[verb].message);
    return GO_CLEAROBJ;
}

static int fly(token_t verb, token_t obj)
/* Fly.  Snide remarks unless hovering rug is here. */
{
    if (obj == INTRANSITIVE) {
        if (!HERE(RUG)) {
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
        rspeak(actions[verb].message);
        return GO_CLEAROBJ;
    }
    if (game.prop[RUG] != RUG_HOVER) {
        rspeak(RUG_NOTHING1);
        return GO_CLEAROBJ;
    }
    game.oldlc2 = game.oldloc;
    game.oldloc = game.loc;
    /* FIXME: Arithmetic on location values */
    game.newloc = game.place[RUG] + game.fixed[RUG] - game.loc;

    if (game.prop[SAPPH] >= 0) {
        rspeak(RUG_RETURNS);
    } else {
        rspeak(RUG_GOES);
    }
    return GO_TERMINATE;
}

static int inven(void)
/* Inventory. If object, treat same as find.  Else report on current burden. */
{
    bool empty = true;
    for (int i = 1; i <= NOBJECTS; i++) {
        if (i == BEAR ||
            !TOTING(i))
            continue;
        if (empty) {
            rspeak(NOW_HOLDING);
            empty = false;
        }
        pspeak(i, touch, -1, false);
    }
    if (TOTING(BEAR))
        rspeak(TAME_BEAR);
    if (empty)
        rspeak(NO_CARRY);
    return GO_CLEAROBJ;
}

static int light(token_t verb, token_t obj)
/*  Light.  Applicable only to lamp and urn. */
{
    if (obj == INTRANSITIVE) {
        if (HERE(LAMP) && game.prop[LAMP] == LAMP_DARK && game.limit >= 0)
            obj = LAMP;
        if (HERE(URN) && game.prop[URN] == URN_DARK)
            obj =  URN;
        if (obj == INTRANSITIVE ||
            (HERE(LAMP) && game.prop[LAMP] == LAMP_DARK && game.limit >= 0 &&
             HERE(URN) && game.prop[URN] == URN_DARK))
            return GO_UNKNOWN;
    }

    if (obj == URN) {
        state_change(URN, game.prop[URN] == URN_EMPTY ?
                     URN_EMPTY :
                     URN_LIT);
        return GO_CLEAROBJ;
    } else {
        if (obj != LAMP) {
            rspeak(actions[verb].message);
            return GO_CLEAROBJ;
        }
        if (game.limit < 0) {
            rspeak(LAMP_OUT);
            return GO_CLEAROBJ;
        }
        state_change(LAMP, LAMP_BRIGHT);
        if (game.wzdark)
            return GO_TOP;
        else
            return GO_CLEAROBJ;
    }
}

static int listen(void)
/*  Listen.  Intransitive only.  Print stuff based on objsnd/locsnd. */
{
    long sound = locations[game.loc].sound;
    if (sound != SILENT) {
        rspeak(sound);
        if (!locations[game.loc].loud)
            rspeak(NO_MESSAGE);
        return GO_CLEAROBJ;
    }
    for (int i = 1; i <= NOBJECTS; i++) {
        if (!HERE(i) ||
            objects[i].sounds[0] == NULL ||
            game.prop[i] < 0)
            continue;
        int mi =  game.prop[i];
        if (i == BIRD)
            mi += 3 * game.blooded;
        long packed_zzword = token_to_packed(game.zzword);
        pspeak(i, hear, mi, true, packed_zzword);
        rspeak(NO_MESSAGE);
        /* FIXME: Magic number, sensitive to bird state logic */
        if (i == BIRD && game.prop[i] == 5)
            DESTROY(BIRD);
        return GO_CLEAROBJ;
    }
    rspeak(ALL_SILENT);
    return GO_CLEAROBJ;
}

static int lock(token_t verb, token_t obj)
/* Lock, unlock, no object given.  Assume various things if present. */
{
    if (obj == INTRANSITIVE) {
        if (HERE(CLAM))
            obj = CLAM;
        if (HERE(OYSTER))
            obj = OYSTER;
        if (AT(DOOR))
            obj = DOOR;
        if (AT(GRATE))
            obj = GRATE;
        if (HERE(CHAIN))
            obj = CHAIN;
        if (obj == INTRANSITIVE) {
            rspeak(NOTHING_LOCKED);
            return GO_CLEAROBJ;
        }
    }

    /*  Lock, unlock object.  Special stuff for opening clam/oyster
     *  and for chain. */
    if (obj == GRATE ||
        obj == CHAIN) {
        if (HERE(KEYS)) {
            if (obj == CHAIN)
                return chain(verb);
            if (game.closng) {
                rspeak(EXIT_CLOSED);
                if (!game.panic)
                    game.clock2 = PANICTIME;
                game.panic = true;
                return GO_CLEAROBJ ;
            } else {
                state_change(GRATE, (verb == LOCK) ?
                             GRATE_CLOSED :
                             GRATE_OPEN);
                return GO_CLEAROBJ;
            }
        }
        rspeak(NO_KEYS);
        return GO_CLEAROBJ;
    }

    switch (obj) {
    case CLAM:
        if (verb == LOCK)
            rspeak(HUH_MAN);
        else if (!TOTING(TRIDENT))
            rspeak(OYSTER_OPENER);
        else {
            DESTROY(CLAM);
            drop(OYSTER, game.loc);
            drop(PEARL, LOC_CULDESAC);
            rspeak(PEARL_FALLS);
        }
        return GO_CLEAROBJ;
    case OYSTER:
        if (verb == LOCK)
            rspeak(HUH_MAN);
        else
            rspeak(OYSTER_OPENER);

        return GO_CLEAROBJ;
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
        rspeak(actions[verb].message);
    }

    return GO_CLEAROBJ;
}

static int pour(token_t verb, token_t obj)
/*  Pour.  If no object, or object is bottle, assume contents of bottle.
 *  special tests for pouring water or oil on plant or rusty door. */
{
    if (obj == BOTTLE ||
        obj == NO_OBJECT)
        obj = LIQUID();
    if (obj == NO_OBJECT)
        return GO_UNKNOWN;
    if (!TOTING(obj)) {
        rspeak(actions[verb].message);
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

static int quit(void)
/*  Quit.  Intransitive only.  Verify intent and exit if that's what he wants. */
{
    if (yes(arbitrary_messages[REALLY_QUIT], arbitrary_messages[OK_MAN], arbitrary_messages[OK_MAN]))
        terminate(quitgame);
    return GO_CLEAROBJ;
}

static int read(struct command_t command)
/*  Read.  Print stuff based on objtxt.  Oyster (?) is special case. */
{
    if (command.obj == INTRANSITIVE) {
        command.obj = 0;
        for (int i = 1; i <= NOBJECTS; i++) {
            if (HERE(i) && objects[i].texts[0] != NULL && game.prop[i] >= 0)
                command.obj = command.obj * NOBJECTS + i;
        }
        if (command.obj > NOBJECTS ||
            command.obj == 0 ||
            DARK(game.loc))
            return GO_UNKNOWN;
    }

    if (DARK(game.loc)) {
        sspeak(NO_SEE, command.raw1);
    } else if (command.obj == OYSTER && !game.clshnt && game.closed) {
        game.clshnt = yes(arbitrary_messages[CLUE_QUERY], arbitrary_messages[WAYOUT_CLUE], arbitrary_messages[OK_MAN]);
    } else if (objects[command.obj].texts[0] == NULL ||
               game.prop[command.obj] < 0) {
        rspeak(actions[command.verb].message);
    } else
        pspeak(command.obj, study, game.prop[command.obj], true);
    return GO_CLEAROBJ;
}

static int reservoir(void)
/*  Z'ZZZ (word gets recomputed at startup; different each game). */
{
    /* FIXME: Arithmetic on state numbers */
    if (!AT(RESER) && game.loc != game.fixed[RESER] - 1) {
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

static int rub(token_t verb, token_t obj)
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
        rspeak(actions[verb].message);
    }
    return GO_CLEAROBJ;
}

static int say(struct command_t *command)
/* Say.  Echo WD2 (or WD1 if no WD2 (SAY WHAT?, etc.).)  Magic words override. */
{
    if (command->wd2 > 0) {
        command->wd1 = command->wd2;
        strcpy(command->raw1, command->raw2);
    }
    char word1[TOKLEN + 1];
    packed_to_token(command->wd1, word1);
    int wd = (int) get_vocab_id(word1);
    /* FIXME: magic numbers */
    if (wd == MOTION_WORD(XYZZY) ||
        wd == MOTION_WORD(PLUGH) ||
        wd == MOTION_WORD(PLOVER) ||
        wd == ACTION_WORD(GIANTWORDS) ||
        wd == ACTION_WORD(PART)) {
        /* FIXME: scribbles on the interpreter's command block */
        wordclear(&command->wd2);
        return GO_LOOKUP;
    }
    sspeak(OKEY_DOKEY, command->raw1);
    return GO_CLEAROBJ;
}

static int throw_support(long spk)
{
    rspeak(spk);
    drop(AXE, game.loc);
    return GO_MOVE;
}

static int throw (struct command_t *command)
/*  Throw.  Same as discard unless axe.  Then same as attack except
 *  ignore bird, and if dwarf is present then one might be killed.
 *  (Only way to do so!)  Axe also special for dragon, bear, and
 *  troll.  Treasures special for troll. */
{
    if (TOTING(ROD2) && command->obj == ROD && !TOTING(ROD))
        command->obj = ROD2;
    if (!TOTING(command->obj)) {
        rspeak(actions[command->verb].message);
        return GO_CLEAROBJ;
    }
    if (objects[command->obj].is_treasure && AT(TROLL)) {
        /*  Snarf a treasure for the troll. */
        drop(command->obj, LOC_NOWHERE);
        move(TROLL, LOC_NOWHERE);
        move(TROLL + NOBJECTS, LOC_NOWHERE);
        drop(TROLL2, objects[TROLL].plac);
        drop(TROLL2 + NOBJECTS, objects[TROLL].fixd);
        juggle(CHASM);
        rspeak(TROLL_SATISFIED);
        return GO_CLEAROBJ;
    }
    if (command->obj == FOOD && HERE(BEAR)) {
        /* But throwing food is another story. */
        command->obj = BEAR;
        return (feed(command->verb, command->obj));
    }
    if (command->obj != AXE)
        return (discard(command->verb, command->obj, false));
    else {
        if (atdwrf(game.loc) <= 0) {
            if (AT(DRAGON) && game.prop[DRAGON] == DRAGON_BARS)
                return throw_support(DRAGON_SCALES);
            if (AT(TROLL))
                return throw_support(TROLL_RETURNS);
            else if (AT(OGRE))
                return throw_support(OGRE_DODGE);
            else if (HERE(BEAR) && game.prop[BEAR] == UNTAMED_BEAR) {
                /* This'll teach him to throw the axe at the bear! */
                drop(AXE, game.loc);
                game.fixed[AXE] = -1;
                juggle(BEAR);
                state_change(AXE, AXE_LOST);
                return GO_CLEAROBJ;
            }
            command->obj = NO_OBJECT;
            return (attack(command));
        }

        if (randrange(NDWARVES + 1) < game.dflag) {
            return throw_support(DWARF_DODGES);
        } else {
            long i = atdwrf(game.loc);
            game.dseen[i] = false;
            game.dloc[i] = LOC_NOWHERE;
            return throw_support((++game.dkill == 1) ?
                                 DWARF_SMOKE :
                                 KILLED_DWARF);
        }
    }
}

static int wake(token_t verb, token_t obj)
/* Wake.  Only use is to disturb the dwarves. */
{
    if (obj != DWARF ||
        !game.closed) {
        rspeak(actions[verb].message);
        return GO_CLEAROBJ;
    } else {
        rspeak(PROD_DWARF);
        return GO_DWARFWAKE;
    }
}

static int wave(token_t verb, token_t obj)
/* Wave.  No effect unless waving rod at fissure or at bird. */
{
    if (obj != ROD ||
        !TOTING(obj) ||
        (!HERE(BIRD) &&
         (game.closng ||
          !AT(FISSURE)))) {
        rspeak(((!TOTING(obj)) && (obj != ROD ||
                                   !TOTING(ROD2))) ?
               ARENT_CARRYING :
               actions[verb].message);
        return GO_CLEAROBJ;
    }

    if (game.prop[BIRD] == BIRD_UNCAGED && game.loc == game.place[STEPS] && game.prop[JADE] < 0) {
        drop(JADE, game.loc);
        game.prop[JADE] = 0;
        --game.tally;
        rspeak(NECKLACE_FLY);
        return GO_CLEAROBJ;
    } else {
        if (game.closed) {
            rspeak((game.prop[BIRD] == BIRD_CAGED) ?
                   CAGE_FLY :
                   FREE_FLY);
            return GO_DWARFWAKE;
        }
        if (game.closng ||
            !AT(FISSURE)) {
            rspeak((game.prop[BIRD] == BIRD_CAGED) ?
                   CAGE_FLY :
                   FREE_FLY);
            return GO_CLEAROBJ;
        }
        if (HERE(BIRD))
            rspeak((game.prop[BIRD] == BIRD_CAGED) ?
                   CAGE_FLY :
                   FREE_FLY);

        state_change(FISSURE,
                     game.prop[FISSURE] == BRIDGED ? UNBRIDGED : BRIDGED);
        return GO_CLEAROBJ;
    }
}

int action(struct command_t *command)
/*  Analyse a verb.  Remember what it was, go back for object if second word
 *  unless verb is "say", which snarfs arbitrary second word.
 */
{
    if (command->part == unknown) {
        /*  Analyse an object word.  See if the thing is here, whether
         *  we've got a verb yet, and so on.  Object must be here
         *  unless verb is "find" or "invent(ory)" (and no new verb
         *  yet to be analysed).  Water and oil are also funny, since
         *  they are never actually dropped at any location, but might
         *  be here inside the bottle or urn or as a feature of the
         *  location. */
        if (HERE(command->obj))
            /* FALL THROUGH */;
        else if (command->obj == GRATE) {
            if (game.loc == LOC_START ||
                game.loc == LOC_VALLEY ||
                game.loc == LOC_SLIT) {
                command->obj = DPRSSN;
            }
            if (game.loc == LOC_COBBLE ||
                game.loc == LOC_DEBRIS ||
                game.loc == LOC_AWKWARD ||
                game.loc == LOC_BIRD ||
                game.loc == LOC_PITTOP) {
                command->obj = ENTRNC;
            }
        } else if (command->obj == DWARF && atdwrf(game.loc) > 0)
            /* FALL THROUGH */;
        else if ((LIQUID() == command->obj && HERE(BOTTLE)) ||
                 command->obj == LIQLOC(game.loc))
            /* FALL THROUGH */;
        else if (command->obj == OIL && HERE(URN) && game.prop[URN] != 0) {
            command->obj = URN;
            /* FALL THROUGH */;
        } else if (command->obj == PLANT && AT(PLANT2) && game.prop[PLANT2] != 0) {
            command->obj = PLANT2;
            /* FALL THROUGH */;
        } else if (command->obj == KNIFE && game.knfloc == game.loc) {
            game.knfloc = -1;
            rspeak(KNIVES_VANISH);
            return GO_CLEAROBJ;
        } else if (command->obj == ROD && HERE(ROD2)) {
            command->obj = ROD2;
            /* FALL THROUGH */;
        } else if ((command->verb == FIND ||
                    command->verb == INVENTORY) && command->wd2 <= 0)
            /* FALL THROUGH */;
        else {
            sspeak(NO_SEE, command->raw1);
            return GO_CLEAROBJ;
        }

        if (command->wd2 > 0)
            return GO_WORD2;
        if (command->verb != 0)
            command->part = transitive;
    }

    switch (command->part) {
    case intransitive:
        if (command->wd2 > 0 && command->verb != SAY)
            return GO_WORD2;
        if (command->verb == SAY)
            command->obj = command->wd2;
        if (command->obj == NO_OBJECT ||
            command->obj == INTRANSITIVE) {
            /*  Analyse an intransitive verb (ie, no object given yet). */
            switch (command->verb) {
            case CARRY:
                return vcarry(command->verb, INTRANSITIVE);
            case  DROP:
                return GO_UNKNOWN;
            case  SAY:
                return GO_UNKNOWN;
            case  UNLOCK:
                return lock(command->verb, INTRANSITIVE);
            case  NOTHING: {
                rspeak(OK_MAN);
                return (GO_CLEAROBJ);
            }
            case  LOCK:
                return lock(command->verb, INTRANSITIVE);
            case  LIGHT:
                return light(command->verb, INTRANSITIVE);
            case  EXTINGUISH:
                return extinguish(command->verb, INTRANSITIVE);
            case  WAVE:
                return GO_UNKNOWN;
            case  TAME:
                return GO_UNKNOWN;
            case GO: {
                rspeak(actions[command->verb].message);
                return GO_CLEAROBJ;
            }
            case ATTACK:
                return attack(command);
            case POUR:
                return pour(command->verb, command->obj);
            case EAT:
                return eat(command->verb, INTRANSITIVE);
            case DRINK:
                return drink(command->verb, command->obj);
            case RUB:
                return GO_UNKNOWN;
            case THROW:
                return GO_UNKNOWN;
            case QUIT:
                return quit();
            case FIND:
                return GO_UNKNOWN;
            case INVENTORY:
                return inven();
            case FEED:
                return GO_UNKNOWN;
            case FILL:
                return fill(command->verb, command->obj);
            case BLAST:
                blast();
                return GO_CLEAROBJ;
            case SCORE:
                score(scoregame);
                return GO_CLEAROBJ;
            case GIANTWORDS:
                return bigwords(command->wd1);
            case BRIEF:
                return brief();
            case READ:
                command->obj = INTRANSITIVE;
                return read(*command);
            case BREAK:
                return GO_UNKNOWN;
            case WAKE:
                return GO_UNKNOWN;
            case SAVE:
                return suspend();
            case RESUME:
                return resume();
            case FLY:
                return fly(command->verb, INTRANSITIVE);
            case LISTEN:
                return listen();
            case PART:
                return reservoir();
            default:
                BUG(INTRANSITIVE_ACTION_VERB_EXCEEDS_GOTO_LIST); // LCOV_EXCL_LINE
            }
        }
    /* FALLTHRU */
    case transitive:
        /*  Analyse a transitive verb. */
        switch (command->verb) {
        case  CARRY:
            return vcarry(command->verb, command->obj);
        case  DROP:
            return discard(command->verb, command->obj, false);
        case  SAY:
            return say(command);
        case  UNLOCK:
            return lock(command->verb, command->obj);
        case  NOTHING: {
            rspeak(OK_MAN);
            return (GO_CLEAROBJ);
        }
        case  LOCK:
            return lock(command->verb, command->obj);
        case LIGHT:
            return light(command->verb, command->obj);
        case EXTINGUISH:
            return extinguish(command->verb, command->obj);
        case WAVE:
            return wave(command->verb, command->obj);
        case TAME: {
            rspeak(actions[command->verb].message);
            return GO_CLEAROBJ;
        }
        case GO: {
            rspeak(actions[command->verb].message);
            return GO_CLEAROBJ;
        }
        case ATTACK:
            return attack(command);
        case POUR:
            return pour(command->verb, command->obj);
        case EAT:
            return eat(command->verb, command->obj);
        case DRINK:
            return drink(command->verb, command->obj);
        case RUB:
            return rub(command->verb, command->obj);
        case THROW:
            return throw (command);
        case QUIT: {
            rspeak(actions[command->verb].message);
            return GO_CLEAROBJ;
        }
        case FIND:
            return find(command->verb, command->obj);
        case INVENTORY:
            return find(command->verb, command->obj);
        case FEED:
            return feed(command->verb, command->obj);
        case FILL:
            return fill(command->verb, command->obj);
        case BLAST:
            blast();
            return GO_CLEAROBJ;
        case SCORE: {
            rspeak(actions[command->verb].message);
            return GO_CLEAROBJ;
        }
        case GIANTWORDS: {
            rspeak(actions[command->verb].message);
            return GO_CLEAROBJ;
        }
        case BRIEF: {
            rspeak(actions[command->verb].message);
            return GO_CLEAROBJ;
        }
        case READ:
            return read(*command);
        case BREAK:
            return vbreak(command->verb, command->obj);
        case WAKE:
            return wake(command->verb, command->obj);
        case SAVE: {
            rspeak(actions[command->verb].message);
            return GO_CLEAROBJ;
        }
        case RESUME: {
            rspeak(actions[command->verb].message);
            return GO_CLEAROBJ;
        }
        case FLY:
            return fly(command->verb, command->obj);
        case LISTEN: {
            rspeak(actions[command->verb].message);
            return GO_CLEAROBJ;
        }
        case PART:
            return reservoir();
        default:
            BUG(TRANSITIVE_ACTION_VERB_EXCEEDS_GOTO_LIST); // LCOV_EXCL_LINE
        }
    case unknown:
        /* Unknown verb, couldn't deduce object - might need hint */
        sspeak(WHAT_DO, command->raw1);
        return GO_CHECKHINT;
    default:
        BUG(SPEECHPART_NOT_TRANSITIVE_OR_INTRANSITIVE_OR_UNKNOWN); // LCOV_EXCL_LINE
    }
}
