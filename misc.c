#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/time.h>
#include <ctype.h>

#include "advent.h"
#include "linenoise/linenoise.h"
#include "dungeon.h"

const char new_advent_to_ascii[] = {
    ' ', '!', '"', '#', '$', '%', '&', '\'',
    '(', ')', '*', '+', ',', '-', '.', '/',
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', ':', ';', '<', '=', '>', '?',
    '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
    'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
    'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
    'X', 'Y', 'Z', '\0', '\0', '\0', '\0', '\0',
};

const char new_ascii_to_advent[] = {
    63, 63, 63, 63, 63, 63, 63, 63,
    63, 63, 63, 63, 63, 63, 63, 63,
    63, 63, 63, 63, 63, 63, 63, 63,
    63, 63, 63, 63, 63, 63, 63, 63,

    0, 1, 2, 3, 4, 5, 6, 7,
    8, 9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23,
    24, 25, 26, 27, 28, 29, 30, 31,
    32, 33, 34, 35, 36, 37, 38, 39,
    40, 41, 42, 43, 44, 45, 46, 47,
    48, 49, 50, 51, 52, 53, 54, 55,
    56, 57, 58, 59, 60, 61, 62, 63,

    63, 63, 63, 63, 63, 63, 63, 63,
    63, 63, 63, 63, 63, 63, 63, 63,
    63, 63, 63, 63, 63, 63, 63, 63,
    63, 63, 63, 63, 63, 63, 63, 63,
};

char* xstrdup(const char* s)
{
    char* ptr = strdup(s);
    if (ptr == NULL) {
        // LCOV_EXCL_START
        // exclude from coverage analysis because we can't simulate an out of memory error in testing
        fprintf(stderr, "Out of memory!\n");
        exit(EXIT_FAILURE);
    }
    return (ptr);
}

void* xmalloc(size_t size)
{
    void* ptr = malloc(size);
    if (ptr == NULL) {
        // LCOV_EXCL_START
        // exclude from coverage analysis because we can't simulate an out of memory error in testing
        fprintf(stderr, "Out of memory!\n");
        exit(EXIT_FAILURE);
        // LCOV_EXCL_STOP
    }
    return (ptr);
}

void packed_to_token(long packed, char token[6])
{
    // Unpack and map back to ASCII.
    for (int i = 0; i < 5; ++i) {
        char advent = (packed >> i * 6) & 63;
        token[i] = new_advent_to_ascii[(int) advent];
    }

    // Ensure the last character is \0.
    token[5] = '\0';

    // Replace trailing whitespace with \0.
    for (int i = 4; i >= 0; --i) {
        if (token[i] == ' ' || token[i] == '\t')
            token[i] = '\0';
        else
            break;
    }
}

long token_to_packed(const char token[6])
{
    size_t t_len = strlen(token);
    long packed = 0;
    for (size_t i = 0; i < t_len; ++i) {
        char mapped = new_ascii_to_advent[(int) token[i]];
        packed |= (mapped << (6 * i));
    }
    return (packed);
}

void tokenize(char* raw, long tokens[4])
{
    // set each token to 0
    for (int i = 0; i < 4; ++i)
        tokens[i] = 0;

    // grab the first two words
    char* words[2];
    words[0] = (char*) xmalloc(strlen(raw));
    words[1] = (char*) xmalloc(strlen(raw));
    int word_count = sscanf(raw, "%s%s", words[0], words[1]);

    // make space for substrings and zero it out
    char chunk_data[][6] = {
        {"\0\0\0\0\0"},
        {"\0\0\0\0\0"},
        {"\0\0\0\0\0"},
        {"\0\0\0\0\0"},
    };

    // break the words into up to 4 5-char substrings
    sscanf(words[0], "%5s%5s", chunk_data[0], chunk_data[1]);
    if (word_count == 2)
        sscanf(words[1], "%5s%5s", chunk_data[2], chunk_data[3]);
    free(words[0]);
    free(words[1]);

    // uppercase all the substrings
    for (int i = 0; i < 4; ++i)
        for (unsigned int j = 0; j < strlen(chunk_data[i]); ++j)
            chunk_data[i][j] = (char) toupper(chunk_data[i][j]);

    // pack the substrings
    for (int i = 0; i < 4; ++i)
        tokens[i] = token_to_packed(chunk_data[i]);
}

/* Hide the fact that wods are corrently packed longs */

bool wordeq(token_t a, token_t b)
{
    return a == b;
}

bool wordempty(token_t a)
{
    return a == 0;
}

void wordclear(token_t *v)
{
    *v = 0;
}

/*  I/O routines (speak, pspeak, rspeak, GETIN, YES) */

void vspeak(const char* msg, va_list ap)
{
    // Do nothing if we got a null pointer.
    if (msg == NULL)
        return;

    // Do nothing if we got an empty string.
    if (strlen(msg) == 0)
        return;

    // Print a newline if the global game.blklin says to.
    if (game.blklin == true)
        printf("\n");

    int msglen = strlen(msg);

    // Rendered string
    ssize_t size = 2000; /* msglen > 50 ? msglen*2 : 100; */
    char* rendered = xmalloc(size);
    char* renderp = rendered;

    // Handle format specifiers (including the custom %C, %L, %S) by adjusting the parameter accordingly, and replacing the specifier with %s.
    long previous_arg = 0;
    for (int i = 0; i < msglen; i++) {
        if (msg[i] != '%') {
            *renderp++ = msg[i];
            size--;
        } else {
            long arg = va_arg(ap, long);
            if (arg == -1)
                arg = 0;
            i++;
            // Integer specifier. In order to accommodate the fact that PARMS can have both legitimate integers *and* packed tokens, stringify everything. Future work may eliminate the need for this.
            if (msg[i] == 'd') {
                int ret = snprintf(renderp, size, "%ld", arg);
                if (ret < size) {
                    renderp += ret;
                    size -= ret;
                }
            }

            // Unmodified string specifier.
            if (msg[i] == 's') {
                packed_to_token(arg, renderp); /* unpack directly to destination */
                size_t len = strlen(renderp);
                renderp += len;
                size -= len;
            }

            // Singular/plural specifier.
            if (msg[i] == 'S') {
                if (previous_arg > 1) { // look at the *previous* parameter (which by necessity must be numeric)
                    *renderp++ = 's';
                    size--;
                }
            }

            // All-lowercase specifier.
            if (msg[i] == 'L' || msg[i] == 'C') {
                packed_to_token(arg, renderp); /* unpack directly to destination */
                int len = strlen(renderp);
                for (int j = 0; j < len; ++j) {
                    renderp[j] = tolower(renderp[j]);
                }
                if (msg[i] == 'C') // First char uppercase, rest lowercase.
                    renderp[0] = toupper(renderp[0]);
                renderp += len;
                size -= len;
            }

            previous_arg = arg;
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
    vspeak(msg, ap);
    va_end(ap);
}

void pspeak(vocab_t msg, enum speaktype mode, int skip, ...)
/* Find the skip+1st message from msg and print it.  Modes are:
 * feel = for inventory, what you can touch
 * look = the long description for the state the object is in
 * listen = the sound for the state the object is in
 * study = text on the object. */
{
    va_list ap;
    va_start(ap, skip);
    switch (mode) {
    case touch:
        vspeak(objects[msg].inventory, ap);
        break;
    case look:
        vspeak(objects[msg].descriptions[skip], ap);
        break;
    case hear:
        vspeak(objects[msg].sounds[skip], ap);
        break;
    case study:
        vspeak(objects[msg].texts[skip], ap);
        break;
    case change:
        vspeak(objects[msg].changes[skip], ap);
        break;
    }
    va_end(ap);
}

void rspeak(vocab_t i, ...)
/* Print the i-th "random" message (section 6 of database). */
{
    va_list ap;
    va_start(ap, i);
    vspeak(arbitrary_messages[i], ap);
    va_end(ap);
}

void echo_input(FILE* destination, char* input_prompt, char* input)
{
    size_t len = strlen(input_prompt) + strlen(input) + 1;
    char* prompt_and_input = (char*) xmalloc(len);
    strcpy(prompt_and_input, input_prompt);
    strcat(prompt_and_input, input);
    fprintf(destination, "%s\n", prompt_and_input);
    free(prompt_and_input);
}

int word_count(char* s)
{
    char* copy = xstrdup(s);
    char delims[] = " \t";
    int count = 0;
    char* word;

    word = strtok(copy, delims);
    while (word != NULL) {
        word = strtok(NULL, delims);
        ++count;
    }
    free(copy);
    return (count);
}

char* get_input()
{
    // Set up the prompt
    char input_prompt[] = "> ";
    if (!prompt)
        input_prompt[0] = '\0';

    // Print a blank line if game.blklin tells us to.
    if (game.blklin == true)
        printf("\n");

    char* input;
    while (true) {
        if (editline)
            input = linenoise(input_prompt);
        else {
            input = NULL;
            size_t n = 0;
            if (isatty(0))
                // LCOV_EXCL_START
                // Should be unreachable in tests, as they will use a non-interactive shell.
                printf("%s", input_prompt);
            // LCOV_EXCL_STOP
            ssize_t numread = getline(&input, &n, stdin);
            if (numread == -1) // Got EOF; return with it.
                return (NULL);
        }

        if (input == NULL) // Got EOF; return with it.
            return (input);
        else if (input[0] == '#') // Ignore comments.
            continue;
        else // We have a 'normal' line; leave the loop.
            break;
    }

    // Strip trailing newlines from the input
    input[strcspn(input, "\n")] = 0;

    linenoiseHistoryAdd(input);

    if (!isatty(0))
        echo_input(stdout, input_prompt, input);

    if (logfp)
        echo_input(logfp, input_prompt, input);

    return (input);
}

bool silent_yes()
{
    char* reply;
    bool outcome;

    for (;;) {
        reply = get_input();
        if (reply == NULL) {
            // LCOV_EXCL_START
            // Should be unreachable. Reply should never be NULL
            linenoiseFree(reply);
            exit(EXIT_SUCCESS);
            // LCOV_EXCL_STOP
        }

        char* firstword = (char*) xmalloc(strlen(reply) + 1);
        sscanf(reply, "%s", firstword);

        for (int i = 0; i < (int)strlen(firstword); ++i)
            firstword[i] = tolower(firstword[i]);

        int yes = strncmp("yes", firstword, sizeof("yes") - 1);
        int y = strncmp("y", firstword, sizeof("y") - 1);
        int no = strncmp("no", firstword, sizeof("no") - 1);
        int n = strncmp("n", firstword, sizeof("n") - 1);

        free(firstword);

        if (yes == 0 || y == 0) {
            outcome = true;
            break;
        } else if (no == 0 || n == 0) {
            outcome = false;
            break;
        } else
            rspeak(PLEASE_ANSWER);
    }
    linenoiseFree(reply);
    return (outcome);
}


bool yes(const char* question, const char* yes_response, const char* no_response)
/*  Print message X, wait for yes/no answer.  If yes, print Y and return true;
 *  if no, print Z and return false. */
{
    char* reply;
    bool outcome;

    for (;;) {
        speak(question);

        reply = get_input();
        if (reply == NULL) {
            // LCOV_EXCL_START
            // Should be unreachable. Reply should never be NULL
            linenoiseFree(reply);
            exit(EXIT_SUCCESS);
            // LCOV_EXCL_STOP
        }

        char* firstword = (char*) xmalloc(strlen(reply) + 1);
        sscanf(reply, "%s", firstword);

        for (int i = 0; i < (int)strlen(firstword); ++i)
            firstword[i] = tolower(firstword[i]);

        int yes = strncmp("yes", firstword, sizeof("yes") - 1);
        int y = strncmp("y", firstword, sizeof("y") - 1);
        int no = strncmp("no", firstword, sizeof("no") - 1);
        int n = strncmp("n", firstword, sizeof("n") - 1);

        free(firstword);

        if (yes == 0 || y == 0) {
            speak(yes_response);
            outcome = true;
            break;
        } else if (no == 0 || n == 0) {
            speak(no_response);
            outcome = false;
            break;
        } else
            rspeak(PLEASE_ANSWER);
    }
    linenoiseFree(reply);
    return (outcome);
}

/*  Data structure  routines */

int get_motion_vocab_id(const char* word)
// Return the first motion number that has 'word' as one of its words.
{
    for (int i = 0; i < NMOTIONS; ++i) {
        for (int j = 0; j < motions[i].words.n; ++j) {
            if (strcasecmp(word, motions[i].words.strs[j]) == 0)
                return (i);
        }
    }
    // If execution reaches here, we didn't find the word.
    return (WORD_NOT_FOUND);
}

int get_object_vocab_id(const char* word)
// Return the first object number that has 'word' as one of its words.
{
    for (int i = 0; i < NOBJECTS + 1; ++i) { // FIXME: the + 1 should go when 1-indexing for objects is removed
        for (int j = 0; j < objects[i].words.n; ++j) {
            if (strcasecmp(word, objects[i].words.strs[j]) == 0)
                return (i);
        }
    }
    // If execution reaches here, we didn't find the word.
    return (WORD_NOT_FOUND);
}

int get_action_vocab_id(const char* word)
// Return the first motion number that has 'word' as one of its words.
{
    for (int i = 0; i < NACTIONS; ++i) {
        for (int j = 0; j < actions[i].words.n; ++j) {
            if (strcasecmp(word, actions[i].words.strs[j]) == 0)
                return (i);
        }
    }
    // If execution reaches here, we didn't find the word.
    return (WORD_NOT_FOUND);
}

int get_special_vocab_id(const char* word)
// Return the first special number that has 'word' as one of its words.
{
    for (int i = 0; i < NSPECIALS; ++i) {
        for (int j = 0; j < specials[i].words.n; ++j) {
            if (strcasecmp(word, specials[i].words.strs[j]) == 0)
                return (i);
        }
    }
    // If execution reaches here, we didn't find the word.
    return (WORD_NOT_FOUND);
}

long get_vocab_id(const char* word)
// Search the vocab categories in order for the supplied word.
{
    long ref_num;

    ref_num = get_motion_vocab_id(word);
    if (ref_num != WORD_NOT_FOUND)
        return (ref_num + 0); // FIXME: replace with a proper hash

    ref_num = get_object_vocab_id(word);
    if (ref_num != WORD_NOT_FOUND)
        return (ref_num + 1000); // FIXME: replace with a proper hash

    ref_num = get_action_vocab_id(word);
    if (ref_num != WORD_NOT_FOUND)
        return (ref_num + 2000); // FIXME: replace with a proper hash

    ref_num = get_special_vocab_id(word);
    if (ref_num != WORD_NOT_FOUND)
        return (ref_num + 3000); // FIXME: replace with a proper hash

    // Check for the reservoir magic word.
    if (strcasecmp(word, game.zzword) == 0)
        return (PART + 2000); // FIXME: replace with a proper hash

    return (WORD_NOT_FOUND);
}

void juggle(long object)
/*  Juggle an object by picking it up and putting it down again, the purpose
 *  being to get the object to the front of the chain of things at its loc. */
{
    long i, j;

    i = game.place[object];
    j = game.fixed[object];
    move(object, i);
    move(object + NOBJECTS, j);
}

void move(long object, long where)
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
    if (from != LOC_NOWHERE && from != CARRIED && !SPECIAL(from))
        carry(object, from);
    drop(object, where);
}

long put(long object, long where, long pval)
/*  PUT is the same as MOVE, except it returns a value used to set up the
 *  negated game.prop values for the repository objects. */
{
    move(object, where);
    return (-1) - pval;;
}

void carry(long object, long where)
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

void drop(long object, long where)
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
    if (where <= 0)
        return;
    game.link[object] = game.atloc[where];
    game.atloc[where] = object;
}

long atdwrf(long where)
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

/*  Utility routines (SETBIT, TSTBIT, set_seed, get_next_lcg_value,
 *  randrange, RNDVOC) */

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

void make_zzword(char zzword[6])
{
    for (int i = 0; i < 5; ++i) {
        zzword[i] = 'A' + randrange(26);
    }
    zzword[1] = '\''; // force second char to apostrophe
    zzword[5] = '\0';
}

void datime(long* d, long* t)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    *d = (long) tv.tv_sec;
    *t = (long) tv.tv_usec;
}

// LCOV_EXCL_START
void bug(enum bugtype num, const char *error_string)
{
    fprintf(stderr, "Fatal error %d, %s.\n", num, error_string);
    exit(EXIT_FAILURE);
}
// LCOV_EXCL_STOP

/* end */
