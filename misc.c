#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/time.h>
#include <ctype.h>
#include <editline/readline.h>

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

void tokenize(char* raw, struct command_t *cmd)
{
    memset(cmd, '\0', sizeof(struct command_t));

    /* Bound prefix on the %s would be needed to prevent buffer
     * overflow.  but we shortstop this more simply by making each
     * raw-input buffer as long as the enrire inout buffer. */
    sscanf(raw, "%s%s", cmd->raw1, cmd->raw2);

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
        cmd->raw1[TOKLEN + TOKLEN] = cmd->raw2[TOKLEN + TOKLEN] = '\0';
        for (size_t i = 0; i < strlen(cmd->raw1); i++)
            cmd->raw1[i] = toupper(cmd->raw1[i]);
        for (size_t i = 0; i < strlen(cmd->raw2); i++)
            cmd->raw2[i] = toupper(cmd->raw2[i]);
    }
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
            // Integer specifier. In order to accommodate the fact
            // that PARMS can have both legitimate integers *and*
            // packed tokens, stringify everything. Future work may
            // eliminate the need for this.
            if (msg[i] == 'd') {
		long arg = va_arg(ap, long);
                int ret = snprintf(renderp, size, "%ld", arg);
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

            /* Version specifier */
            if (msg[i] == 'V') {
                strcpy(renderp, VERSION);
                size_t len = strlen(VERSION);
                renderp += len;
                size -= len;
            }
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

void sspeak(const long msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    fputc('\n', stdout);
    vprintf(arbitrary_messages[msg], ap);
    fputc('\n', stdout);
    va_end(ap);
}

void pspeak(vocab_t msg, enum speaktype mode, int skip, bool blank, ...)
/* Find the skip+1st message from msg and print it.  Modes are:
 * feel = for inventory, what you can touch
 * look = the long description for the state the object is in
 * listen = the sound for the state the object is in
 * study = text on the object. */
{
    va_list ap;
    va_start(ap, blank);
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

int word_count(char* str)
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

char* get_input()
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

bool silent_yes()
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

static int get_special_vocab_id(const char* word)
// Return the first special number that has 'word' as one of its words.
{
    for (int i = 0; i < NSPECIALS; ++i) {
        for (int j = 0; j < specials[i].words.n; ++j) {
            if (strncasecmp(word, specials[i].words.strs[j], TOKLEN) == 0)
                return (i);
        }
    }
    // If execution reaches here, we didn't find the word.
    return (WORD_NOT_FOUND);
}

void get_vocab_metadata(const char* word, long* id, enum wordtype* type)
{
    /* Check for an empty string */
    if (strncmp(word, "", sizeof("")) == 0) {
        *id = WORD_EMPTY;
        *type = NO_WORD_TYPE;
        return;
    }

    long ref_num;

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

    ref_num = get_special_vocab_id(word);
    if (ref_num != WORD_NOT_FOUND) {
        *id = ref_num;
        *type = SPECIAL;
        return;
    }

    // Check for the reservoir magic word.
    if (strcasecmp(word, game.zzword) == 0) {
        *id = PART;
        *type = ACTION;
        return;
    }

    *id = WORD_NOT_FOUND;
    *type = NO_WORD_TYPE;
    return;
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
    long from;

    if (object > NOBJECTS)
        from = game.fixed[object - NOBJECTS];
    else
        from = game.place[object];
    /* (ESR) Used to check for !SPECIAL(from). I *think* that was wrong... */
    if (from != LOC_NOWHERE && from != CARRIED)
        carry(object, from);
    drop(object, where);
}

long put(obj_t object, loc_t where, long pval)
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
            --game.holdng;
        game.place[object] = where;
    }
    if (where == LOC_NOWHERE ||
        where == CARRIED)
        return;
    game.link[object] = game.atloc[where];
    game.atloc[where] = object;
}

long atdwrf(loc_t where)
/*  Return the index of first dwarf at the given location, zero if no dwarf is
 *  there (or if dwarves not active yet), -1 if all dwarves are dead.  Ignore
 *  the pirate (6th dwarf). */
{
    long at;

    at = 0;
    if (game.dflag < 2)
        return (at);
    at = -1;
    for (long i = 1; i <= NDWARVES - 1; i++) {
        if (game.dloc[i] == where)
            return i;
        if (game.dloc[i] != 0)
            at = 0;
    }
    return (at);
}

/*  Utility routines (setbit, tstbit, set_seed, get_next_lcg_value,
 *  randrange) */

long setbit(long bit)
/*  Returns 2**bit for use in constructing bit-masks. */
{
    return (1L << bit);
}

bool tstbit(long mask, int bit)
/*  Returns true if the specified bit is set in the mask. */
{
    return (mask & (1 << bit)) != 0;
}

void set_seed(long seedval)
/* Set the LCG seed */
{
    game.lcg_x = (unsigned long) seedval % game.lcg_m;

    // once seed is set, we need to generate the Z`ZZZ word
    make_zzword(game.zzword);
}

unsigned long get_next_lcg_value(void)
/* Return the LCG's current value, and then iterate it. */
{
    unsigned long old_x = game.lcg_x;
    game.lcg_x = (game.lcg_a * game.lcg_x + game.lcg_c) % game.lcg_m;
    return old_x;
}

long randrange(long range)
/* Return a random integer from [0, range). */
{
    return range * get_next_lcg_value() / game.lcg_m;
}

void make_zzword(char zzword[TOKLEN + 1])
{
    for (int i = 0; i < 5; ++i) {
        zzword[i] = 'A' + randrange(26);
    }
    zzword[1] = '\''; // force second char to apostrophe
    zzword[5] = '\0';
}

// LCOV_EXCL_START
void bug(enum bugtype num, const char *error_string)
{
    fprintf(stderr, "Fatal error %d, %s.\n", num, error_string);
    exit(EXIT_FAILURE);
}
// LCOV_EXCL_STOP

/* end */

void state_change(obj_t obj, long state)
/* Object must have a change-message list for this to be useful; only some do */
{
    game.prop[obj] = state;
    pspeak(obj, change, state, true);
}

/* end */
