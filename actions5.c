#include "actions.h"

phase_codes_t waste(verb_t verb, turn_t turns)
/* Burn turns */
{
    game.limit -= turns;
    speak(get_action(verb)->message, (int)game.limit);
    return GO_TOP;
}

phase_codes_t wave(verb_t verb, obj_t obj)
/* Wave.  No effect unless waving rod at fissure or at bird. */
{
    if (obj != ROD ||
        !TOTING(obj) ||
        (!HERE(BIRD) &&
         (game.closng ||
          !AT(FISSURE)))) {
        speak(((!TOTING(obj)) && (obj != ROD ||
                                  !TOTING(ROD2))) ?
              get_arbitrary_message_index(ARENT_CARRYING) :
              get_action(verb)->message);
        return GO_CLEAROBJ;
    }

    if (game.prop[BIRD] == BIRD_UNCAGED && game.loc == game.place[STEPS] && game.prop[JADE] == STATE_NOTFOUND) {
        drop(JADE, game.loc);
        game.prop[JADE] = STATE_FOUND;
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

phase_codes_t action(command_t* command)
/*  Analyse a verb.  Remember what it was, go back for object if second word
 *  unless verb is "say", which snarfs arbitrary second word.
 */
{
    /* Previously, actions that result in a message, but don't do anything
     * further were called "specials". Now they're handled here as normal
     * actions. If noaction is true, then we spit out the message and return */
    if (get_action(command->verb)->noaction) {
        speak(get_action(command->verb)->message);
        return GO_CLEAROBJ;
    }

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
        else if (command->obj == DWARF && atdwrf(game.loc) > 0)
            /* FALL THROUGH */;
        else if ((LIQUID() == command->obj && HERE(BOTTLE)) ||
                 command->obj == LIQLOC(game.loc))
            /* FALL THROUGH */;
        else if (command->obj == OIL && HERE(URN) && game.prop[URN] != URN_EMPTY) {
            command->obj = URN;
            /* FALL THROUGH */;
        } else if (command->obj == PLANT && AT(PLANT2) && game.prop[PLANT2] != PLANT_THIRSTY) {
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
                    command->verb == INVENTORY) && (command->word[1].id == WORD_EMPTY || command->word[1].id == WORD_NOT_FOUND))
            /* FALL THROUGH */;
        else {
            sspeak(NO_SEE, command->word[0].raw);
            return GO_CLEAROBJ;
        }

        if (command->verb != 0)
            command->part = transitive;
    }

    switch (command->part) {
    case intransitive:
        if (command->word[1].raw[0] != '\0' && command->verb != SAY)
            return GO_WORD2;
        if (command->verb == SAY)
            /* KEYS is not special, anything not NO_OBJECT or INTRANSITIVE
             * will do here. We're preventing interpretation as an intransitive
             * verb when the word is unknown. */
            command->obj = command->word[1].raw[0] != '\0' ? KEYS : NO_OBJECT;
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
                speak(get_action(command->verb)->message);
                return GO_CLEAROBJ;
            }
            case ATTACK:
                command->obj = INTRANSITIVE;
                return attack(command);
            case POUR:
                return pour(command->verb, INTRANSITIVE);
            case EAT:
                return eat(command->verb, INTRANSITIVE);
            case DRINK:
                return drink(command->verb, INTRANSITIVE);
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
                return fill(command->verb, INTRANSITIVE);
            case BLAST:
                blast();
                return GO_CLEAROBJ;
            case SCORE:
                score(scoregame);
                return GO_CLEAROBJ;
            case FEE:
            case FIE:
            case FOE:
            case FOO:
            case FUM:
                return bigwords(command->word[0].id);
            case BRIEF:
                return brief();
            case READ:
                command->obj = INTRANSITIVE;
                return action_read(command);
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
            case SEED:
            case WASTE:
                rspeak(NUMERIC_REQUIRED);
                return GO_TOP;
#ifndef CALCULATOR
            default: // LCOV_EXCL_LINE
                BUG(INTRANSITIVE_ACTION_VERB_EXCEEDS_GOTO_LIST); // LCOV_EXCL_LINE
#endif
            }
        }
    /* FALLTHRU */
    case transitive:
        /*  Analyse a transitive verb. */
        switch (command->verb) {
        case  CARRY:
            return vcarry(command->verb, command->obj);
        case  DROP:
            return discard(command->verb, command->obj);
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
            speak(get_action(command->verb)->message);
            return GO_CLEAROBJ;
        }
        case GO: {
            speak(get_action(command->verb)->message);
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
            speak(get_action(command->verb)->message);
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
            speak(get_action(command->verb)->message);
            return GO_CLEAROBJ;
        }
        case FEE:
        case FIE:
        case FOE:
        case FOO:
        case FUM: {
            speak(get_action(command->verb)->message);
            return GO_CLEAROBJ;
        }
        case BRIEF: {
            speak(get_action(command->verb)->message);
            return GO_CLEAROBJ;
        }
        case READ:
            return action_read(command);
        case BREAK:
            return vbreak(command->verb, command->obj);
        case WAKE:
            return wake(command->verb, command->obj);
        case SAVE: {
            speak(get_action(command->verb)->message);
            return GO_CLEAROBJ;
        }
        case RESUME: {
            speak(get_action(command->verb)->message);
            return GO_CLEAROBJ;
        }
        case FLY:
            return fly(command->verb, command->obj);
        case LISTEN: {
            speak(get_action(command->verb)->message);
            return GO_CLEAROBJ;
        }
        // LCOV_EXCL_START
        // This case should never happen - here only as placeholder
        case PART:
            return reservoir();
        // LCOV_EXCL_STOP
        case SEED:
            return seed(command->verb, command->word[1].raw);
        case WASTE:
            return waste(command->verb, (turn_t)atol(command->word[1].raw));
#ifndef CALCULATOR
        default: // LCOV_EXCL_LINE
            BUG(TRANSITIVE_ACTION_VERB_EXCEEDS_GOTO_LIST); // LCOV_EXCL_LINE
#endif
        }
    case unknown:
        /* Unknown verb, couldn't deduce object - might need hint */
        sspeak(WHAT_DO, command->word[0].raw);
        return GO_CHECKHINT;
    default: // LCOV_EXCL_LINE
#ifndef CALCULATOR
        BUG(SPEECHPART_NOT_TRANSITIVE_OR_INTRANSITIVE_OR_UNKNOWN); // LCOV_EXCL_LINE
#else
        return 0;
#endif
    }
}
