/*
 * I/O and support riutines.
 *
 * Copyright (c) 1977, 2005 by Will Crowther and Don Woods
 * Copyright (c) 2017 by Eric S. Raymond
 * SPDX-License-Identifier: BSD-2-clause
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/time.h>
#include <ctype.h>
#include <editline/readline.h>
#include <inttypes.h>

#include "advent.h"
#include "dungeon.h"

static void* xcalloc(size_t size)
{
    void* ptr = calloc(size, 1);
    if (ptr == NULL) {
        // LCOV_EXCL_START
        // exclude from coverage analysis because we can't simulate an out of memory error in testing
        fprintf(stderr, "Out of memory!\n");
        exit(EXIT_FAILURE);
        // LCOV_EXCL_STOP
    }
    return (ptr);
}

/*  I/O routines (speak, pspeak, rspeak, sspeak, get_input, yes) */

static void vspeak(const char* msg, bool blank, va_list ap)
{
    // Do nothing if we got a null pointer.
    if (msg == NULL)
        return;

    // Do nothing if we got an empty string.
    if (strlen(msg) == 0)
        return;

    if (blank == true)
        printf("\n");

    int msglen = strlen(msg);

    // Rendered string
    ssize_t size = 2000; /* msglen > 50 ? msglen*2 : 100; */
    char* rendered = xcalloc(size);
    char* renderp = rendered;

    // Handle format specifiers (including the custom %S) by
    // adjusting the parameter accordingly, and replacing the
    // specifier with %s.
    bool pluralize = false;
    for (int i = 0; i < msglen; i++) {
        if (msg[i] != '%') {
            /* Ugh.  Least obtrusive way to deal with artifacts "on the floor"
             * being dropped outside of both cave and building. */
            if (strncmp(msg + i, "floor", 5) == 0 && strchr(" .", msg[i + 5]) && !INSIDE(game.loc)) {
                strcpy(renderp, "ground");
                renderp += 6;
                i += 4;
                size -= 5;
            } else {
                *renderp++ = msg[i];
                size--;
            }
        } else {
            i++;
            // Integer specifier.
            if (msg[i] == 'd') {
                int32_t arg = va_arg(ap, int32_t);
                int ret = snprintf(renderp, size, "%" PRId32, arg);
                if (ret < size) {
                    renderp += ret;
                    size -= ret;
                }
                pluralize = (arg != 1);
            }

            // Unmodified string specifier.
            if (msg[i] == 's') {
                char *arg = va_arg(ap, char *);
                strncat(renderp, arg, size - 1);
                size_t len = strlen(renderp);
                renderp += len;
                size -= len;
            }

            // Singular/plural specifier.
            if (msg[i] == 'S') {
                // look at the *previous* numeric parameter
                if (pluralize) {
                    *renderp++ = 's';
                    size--;
                }
            }

            // LCOV_EXCL_START - doesn't occur in test suite.
            /* Version specifier */
            if (msg[i] == 'V') {
                strcpy(renderp, VERSION);
                size_t len = strlen(VERSION);
                renderp += len;
                size -= len;
            }
            // LCOV_EXCL_STOP
        }
    }
    *renderp = 0;

    // Print the message.
    printf("%s\n", rendered);

    free(rendered);
}

void speak(const char* msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    vspeak(msg, true, ap);
    va_end(ap);
}

void sspeak(const int msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    fputc('\n', stdout);
    vprintf(arbitrary_messages[msg], ap);
    fputc('\n', stdout);
    va_end(ap);
}

void pspeak(vocab_t msg, enum speaktype mode, bool blank, int skip, ...)
/* Find the skip+1st message from msg and print it.  Modes are:
 * feel = for inventory, what you can touch
 * look = the full description for the state the object is in
 * listen = the sound for the state the object is in
 * study = text on the object. */
{
    va_list ap;
    va_start(ap, skip);
    switch (mode) {
    case touch:
        vspeak(objects[msg].inventory, blank, ap);
        break;
    case look:
        vspeak(objects[msg].descriptions[skip], blank, ap);
        break;
    case hear:
        vspeak(objects[msg].sounds[skip], blank, ap);
        break;
    case study:
        vspeak(objects[msg].texts[skip], blank, ap);
        break;
    case change:
        vspeak(objects[msg].changes[skip], blank, ap);
        break;
    }
    va_end(ap);
}

void rspeak(vocab_t i, ...)
/* Print the i-th "random" message (section 6 of database). */
{
    va_list ap;
    va_start(ap, i);
    vspeak(arbitrary_messages[i], true, ap);
    va_end(ap);
}

void echo_input(FILE* destination, const char* input_prompt, const char* input)
{
    size_t len = strlen(input_prompt) + strlen(input) + 1;
    char* prompt_and_input = (char*) xcalloc(len);
    strcpy(prompt_and_input, input_prompt);
    strcat(prompt_and_input, input);
    fprintf(destination, "%s\n", prompt_and_input);
    free(prompt_and_input);
}

static int word_count(char* str)
{
    char delims[] = " \t";
    int count = 0;
    int inblanks = true;

    for (char *s = str; *s; s++)
        if (inblanks) {
            if (strchr(delims, *s) == 0) {
                ++count;
                inblanks = false;
            }
        } else {
            if (strchr(delims, *s) != 0) {
                inblanks = true;
            }
        }

    return (count);
}

static char* get_input(void)
{
    // Set up the prompt
    char input_prompt[] = "> ";
    if (!settings.prompt)
        input_prompt[0] = '\0';

    // Print a blank line
    printf("\n");

    char* input;
    while (true) {
        input = readline(input_prompt);

        if (input == NULL) // Got EOF; return with it.
            return (input);
        if (input[0] == '#') { // Ignore comments.
            free(input);
            continue;
        }
        // We have a 'normal' line; leave the loop.
        break;
    }

    // Strip trailing newlines from the input
    input[strcspn(input, "\n")] = 0;

    add_history(input);

    if (!isatty(0))
        echo_input(stdout, input_prompt, input);

    if (settings.logfp)
        echo_input(settings.logfp, "", input);

    return (input);
}

bool silent_yes(void)
{
    bool outcome = false;

    for (;;) {
        char* reply = get_input();
        if (reply == NULL) {
            // LCOV_EXCL_START
            // Should be unreachable. Reply should never be NULL
            free(reply);
            exit(EXIT_SUCCESS);
            // LCOV_EXCL_STOP
        }
        if (strlen(reply) == 0) {
            free(reply);
            rspeak(PLEASE_ANSWER);
            continue;
        }

        char* firstword = (char*) xcalloc(strlen(reply) + 1);
        sscanf(reply, "%s", firstword);

        free(reply);

        for (int i = 0; i < (int)strlen(firstword); ++i)
            firstword[i] = tolower(firstword[i]);

        int yes = strncmp("yes", firstword, sizeof("yes") - 1);
        int y = strncmp("y", firstword, sizeof("y") - 1);
        int no = strncmp("no", firstword, sizeof("no") - 1);
        int n = strncmp("n", firstword, sizeof("n") - 1);

        free(firstword);

        if (yes == 0 ||
            y == 0) {
            outcome = true;
            break;
        } else if (no == 0 ||
                   n == 0) {
            outcome = false;
            break;
        } else
            rspeak(PLEASE_ANSWER);
    }
    return (outcome);
}


bool yes(const char* question, const char* yes_response, const char* no_response)
/*  Print message X, wait for yes/no answer.  If yes, print Y and return true;
 *  if no, print Z and return false. */
{
    bool outcome = false;

    for (;;) {
        speak(question);

        char* reply = get_input();
        if (reply == NULL) {
            // LCOV_EXCL_START
            // Should be unreachable. Reply should never be NULL
            free(reply);
            exit(EXIT_SUCCESS);
            // LCOV_EXCL_STOP
        }

        if (strlen(reply) == 0) {
            free(reply);
            rspeak(PLEASE_ANSWER);
            continue;
        }

        char* firstword = (char*) xcalloc(strlen(reply) + 1);
        sscanf(reply, "%s", firstword);

        free(reply);

        for (int i = 0; i < (int)strlen(firstword); ++i)
            firstword[i] = tolower(firstword[i]);

        int yes = strncmp("yes", firstword, sizeof("yes") - 1);
        int y = strncmp("y", firstword, sizeof("y") - 1);
        int no = strncmp("no", firstword, sizeof("no") - 1);
        int n = strncmp("n", firstword, sizeof("n") - 1);

        free(firstword);

        if (yes == 0 ||
            y == 0) {
            speak(yes_response);
            outcome = true;
            break;
        } else if (no == 0 ||
                   n == 0) {
            speak(no_response);
            outcome = false;
            break;
        } else
            rspeak(PLEASE_ANSWER);

    }

    return (outcome);
}

/*  Data structure  routines */

static int get_motion_vocab_id(const char* word)
// Return the first motion number that has 'word' as one of its words.
{
    for (int i = 0; i < NMOTIONS; ++i) {
        for (int j = 0; j < motions[i].words.n; ++j) {
            if (strncasecmp(word, motions[i].words.strs[j], TOKLEN) == 0 && (strlen(word) > 1 ||
                    strchr(ignore, word[0]) == NULL ||
                    !settings.oldstyle))
                return (i);
        }
    }
    // If execution reaches here, we didn't find the word.
    return (WORD_NOT_FOUND);
}

static int get_object_vocab_id(const char* word)
// Return the first object number that has 'word' as one of its words.
{
    for (int i = 0; i < NOBJECTS + 1; ++i) { // FIXME: the + 1 should go when 1-indexing for objects is removed
        for (int j = 0; j < objects[i].words.n; ++j) {
            if (strncasecmp(word, objects[i].words.strs[j], TOKLEN) == 0)
                return (i);
        }
    }
    // If execution reaches here, we didn't find the word.
    return (WORD_NOT_FOUND);
}

static int get_action_vocab_id(const char* word)
// Return the first motion number that has 'word' as one of its words.
{
    for (int i = 0; i < NACTIONS; ++i) {
        for (int j = 0; j < actions[i].words.n; ++j) {
            if (strncasecmp(word, actions[i].words.strs[j], TOKLEN) == 0 && (strlen(word) > 1 ||
                    strchr(ignore, word[0]) == NULL ||
                    !settings.oldstyle))
                return (i);
        }
    }
    // If execution reaches here, we didn't find the word.
    return (WORD_NOT_FOUND);
}

static bool is_valid_int(const char *str)
/* Returns true if the string passed in is represents a valid integer,
 * that could then be parsed by atoi() */
{
    // Handle negative number
    if (*str == '-')
        ++str;

    // Handle empty string or just "-". Should never reach this
    // point, because this is only used with transitive verbs.
    if (!*str)
        return false; // LCOV_EXCL_LINE

    // Check for non-digit chars in the rest of the stirng.
    while (*str) {
        if (!isdigit(*str))
            return false;
        else
            ++str;
    }

    return true;
}

static void get_vocab_metadata(const char* word, vocab_t* id, word_type_t* type)
{
    /* Check for an empty string */
    if (strncmp(word, "", sizeof("")) == 0) {
        *id = WORD_EMPTY;
        *type = NO_WORD_TYPE;
        return;
    }

    vocab_t ref_num;

    ref_num = get_motion_vocab_id(word);
    if (ref_num != WORD_NOT_FOUND) {
        *id = ref_num;
        *type = MOTION;
        return;
    }

    ref_num = get_object_vocab_id(word);
    if (ref_num != WORD_NOT_FOUND) {
        *id = ref_num;
        *type = OBJECT;
        return;
    }

    ref_num = get_action_vocab_id(word);
    if (ref_num != WORD_NOT_FOUND) {
        *id = ref_num;
        *type = ACTION;
        return;
    }

    // Check for the reservoir magic word.
    if (strcasecmp(word, game.zzword) == 0) {
        *id = PART;
        *type = ACTION;
        return;
    }

    // Check words that are actually numbers.
    if (is_valid_int(word)) {
        *id = WORD_EMPTY;
        *type = NUMERIC;
        return;
    }

    *id = WORD_NOT_FOUND;
    *type = NO_WORD_TYPE;
    return;
}

static void tokenize(char* raw, command_t *cmd)
{
    /*
     * Be caereful about modifing this. We do not want to nuke the
     * the speech part or ID from the previous turn.
     */
    memset(&cmd->word[0].raw, '\0', sizeof(cmd->word[0].raw));
    memset(&cmd->word[1].raw, '\0', sizeof(cmd->word[1].raw));

    /* Bound prefix on the %s would be needed to prevent buffer
     * overflow.  but we shortstop this more simply by making each
     * raw-input buffer as long as the entire input buffer. */
    sscanf(raw, "%s%s", cmd->word[0].raw, cmd->word[1].raw);

    /* (ESR) In oldstyle mode, simulate the uppercasing and truncating
     * effect on raw tokens of packing them into sixbit characters, 5
     * to a 32-bit word.  This is something the FORTRAN version did
     * becuse archaic FORTRAN had no string types.  Don Wood's
     * mechanical translation of 2.5 to C retained the packing and
     * thus this misfeature.
     *
     * It's philosophically questionable whether this is the right
     * thing to do even in oldstyle mode.  On one hand, the text
     * mangling was not authorial intent, but a result of limitations
     * in their tools. On the other, not simulating this misbehavior
     * goes against the goal of making oldstyle as accurate as
     * possible an emulation of the original UI.
     */
    if (settings.oldstyle) {
        cmd->word[0].raw[TOKLEN + TOKLEN] = cmd->word[1].raw[TOKLEN + TOKLEN] = '\0';
        for (size_t i = 0; i < strlen(cmd->word[0].raw); i++)
            cmd->word[0].raw[i] = toupper(cmd->word[0].raw[i]);
        for (size_t i = 0; i < strlen(cmd->word[1].raw); i++)
            cmd->word[1].raw[i] = toupper(cmd->word[1].raw[i]);
    }

    /* populate command with parsed vocabulary metadata */
    get_vocab_metadata(cmd->word[0].raw, &(cmd->word[0].id), &(cmd->word[0].type));
    get_vocab_metadata(cmd->word[1].raw, &(cmd->word[1].id), &(cmd->word[1].type));
    cmd->state = TOKENIZED;
}

bool get_command_input(command_t *command)
/* Get user input on stdin, parse and map to command */
{
    char inputbuf[LINESIZE];
    char* input;

    for (;;) {
        input = get_input();
        if (input == NULL)
            return false;
        if (word_count(input) > 2) {
            rspeak(TWO_WORDS);
            free(input);
            continue;
        }
        if (strcmp(input, "") != 0)
            break;
        free(input);
    }

    strncpy(inputbuf, input, LINESIZE - 1);
    free(input);

    tokenize(inputbuf, command);

#ifdef GDEBUG
    /* Needs to stay synced with enum word_type_t */
    const char *types[] = {"NO_WORD_TYPE", "MOTION", "OBJECT", "ACTION", "NUMERIC"};
    /* needs to stay synced with enum speechpart */
    const char *roles[] = {"unknown", "intransitive", "transitive"};
    printf("Command: role = %s type1 = %s, id1 = %ld, type2 = %s, id2 = %ld\n",
           roles[command->part],
           types[command->word[0].type],
           command->word[0].id,
           types[command->word[1].type],
           command->word[1].id);
#endif

    command->state = GIVEN;
    return true;
}

void juggle(obj_t object)
/*  Juggle an object by picking it up and putting it down again, the purpose
 *  being to get the object to the front of the chain of things at its loc. */
{
    loc_t i, j;

    i = game.place[object];
    j = game.fixed[object];
    move(object, i);
    move(object + NOBJECTS, j);
}

void move(obj_t object, loc_t where)
/*  Place any object anywhere by picking it up and dropping it.  May
 *  already be toting, in which case the carry is a no-op.  Mustn't
 *  pick up objects which are not at any loc, since carry wants to
 *  remove objects from game.atloc chains. */
{
    loc_t from;

    if (object > NOBJECTS)
        from = game.fixed[object - NOBJECTS];
    else
        from = game.place[object];
    /* (ESR) Used to check for !SPECIAL(from). I *think* that was wrong... */
    if (from != LOC_NOWHERE && from != CARRIED)
        carry(object, from);
    drop(object, where);
}

loc_t put(obj_t object, loc_t where, long pval)
/*  put() is the same as move(), except it returns a value used to set up the
 *  negated game.prop values for the repository objects. */
{
    move(object, where);
    return STASHED(pval);
}

void carry(obj_t object, loc_t where)
/*  Start toting an object, removing it from the list of things at its former
 *  location.  Incr holdng unless it was already being toted.  If object>NOBJECTS
 *  (moving "fixed" second loc), don't change game.place or game.holdng. */
{
    long temp;

    if (object <= NOBJECTS) {
        if (game.place[object] == CARRIED)
            return;
        game.place[object] = CARRIED;

        if (object != BIRD)
            ++game.holdng;
    }
    if (game.atloc[where] == object) {
        game.atloc[where] = game.link[object];
        return;
    }
    temp = game.atloc[where];
    while (game.link[temp] != object) {
        temp = game.link[temp];
    }
    game.link[temp] = game.link[object];
}

void drop(obj_t object, loc_t where)
/*  Place an object at a given loc, prefixing it onto the game.atloc list.  Decr
 *  game.holdng if the object was being toted. */
{
    if (object > NOBJECTS)
        game.fixed[object - NOBJECTS] = where;
    else {
        if (game.place[object] == CARRIED)
            if (object != BIRD)
                /* The bird has to be weightless.  This ugly hack (and the
                 * corresponding code in the drop function) brought to you
                 * by the fact that when the bird is caged, we need to be able
                 * to either 'take bird' or 'take cage' and have the right thing
                 * happen.
                 */
                --game.holdng;
        game.place[object] = where;
    }
    if (where == LOC_NOWHERE ||
        where == CARRIED)
        return;
    game.link[object] = game.atloc[where];
    game.atloc[where] = object;
}

int atdwrf(loc_t where)
/*  Return the index of first dwarf at the given location, zero if no dwarf is
 *  there (or if dwarves not active yet), -1 if all dwarves are dead.  Ignore
 *  the pirate (6th dwarf). */
{
    int at;

    at = 0;
    if (game.dflag < 2)
        return at;
    at = -1;
    for (long i = 1; i <= NDWARVES - 1; i++) {
        if (game.dloc[i] == where)
            return i;
        if (game.dloc[i] != 0)
            at = 0;
    }
    return at;
}

/*  Utility routines (setbit, tstbit, set_seed, get_next_lcg_value,
 *  randrange) */

long setbit(int bit)
/*  Returns 2**bit for use in constructing bit-masks. */
{
    return (1L << bit);
}

bool tstbit(long mask, int bit)
/*  Returns true if the specified bit is set in the mask. */
{
    return (mask & (1 << bit)) != 0;
}

void set_seed(int32_t seedval)
/* Set the LCG seed */
{
    game.lcg_x = seedval % LCG_M;
    if (game.lcg_x < 0) {
        game.lcg_x = LCG_M + game.lcg_x;
    }
    // once seed is set, we need to generate the Z`ZZZ word
    for (int i = 0; i < 5; ++i) {
        game.zzword[i] = 'A' + randrange(26);
    }
    game.zzword[1] = '\''; // force second char to apostrophe
    game.zzword[5] = '\0';
}

static int32_t get_next_lcg_value(void)
/* Return the LCG's current value, and then iterate it. */
{
    int32_t old_x = game.lcg_x;
    game.lcg_x = (LCG_A * game.lcg_x + LCG_C) % LCG_M;
    return old_x;
}

int32_t randrange(int32_t range)
/* Return a random integer from [0, range). */
{
    return range * get_next_lcg_value() / LCG_M;
}

// LCOV_EXCL_START
void bug(enum bugtype num, const char *error_string)
{
    fprintf(stderr, "Fatal error %d, %s.\n", num, error_string);
    exit(EXIT_FAILURE);
}
// LCOV_EXCL_STOP

/* end */

void state_change(obj_t obj, int state)
/* Object must have a change-message list for this to be useful; only some do */
{
    game.prop[obj] = state;
    pspeak(obj, change, true, state);
}

/* end */
