#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <ctype.h>

#include "advent.h"
#include "database.h"
#include "linenoise/linenoise.h"
#include "newdb.h"

void* xmalloc(size_t size)
{
  void* ptr = malloc(size);
  if (ptr == NULL)
    {
      fprintf(stderr, "Out of memory!\n");
      exit(EXIT_FAILURE);
    }
  return(ptr);
}

char* xstrdup(const char* s)
{
    char* ptr = strdup(s);
    if (ptr == NULL) {
        fprintf(stderr, "Out of memory!\n");
        exit(EXIT_FAILURE);
    }
    return (ptr);
}

void packed_to_token(long packed, char token[6])
{
    // Unpack and map back to ASCII.
    for (int i = 0; i < 5; ++i) {
        char advent = (packed >> i * 6) & 63;
        token[4 - i] = advent_to_ascii[(int) advent];
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

/*  I/O routines (SPEAK, PSPEAK, RSPEAK, SETPRM, GETIN, YES) */

void speak(const char* msg)
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

    // Create a copy of our string, so we can edit it.
    char* copy = xstrdup(msg);

    // Staging area for stringified parameters.
    char parameters[5][100]; // FIXME: to be replaced with dynamic allocation

    // Handle format specifiers (including the custom %C, %L, %S) by adjusting the parameter accordingly, and replacing the specifier with %s.
    int pi = 0; // parameter index
    for (int i = 0; i < (int)strlen(msg); ++i) {
        if (msg[i] == '%') {
            ++pi;

            // Integer specifier. In order to accommodate the fact that PARMS can have both legitimate integers *and* packed tokens, stringify everything. Future work may eliminate the need for this.
            if (msg[i + 1] == 'd') {
                copy[i + 1] = 's';
                sprintf(parameters[pi], "%ld", PARMS[pi]);
            }

            // Unmodified string specifier.
            if (msg[i + 1] == 's') {
                packed_to_token(PARMS[pi], parameters[pi]);
            }

            // Singular/plural specifier.
            if (msg[i + 1] == 'S') {
                copy[i + 1] = 's';
                if (PARMS[pi - 1] > 1) { // look at the *previous* parameter (which by necessity must be numeric)
                    sprintf(parameters[pi], "%s", "s");
                } else {
                    sprintf(parameters[pi], "%s", "");
                }
            }

            // All-lowercase specifier.
            if (msg[i + 1] == 'L') {
                copy[i + 1] = 's';
                packed_to_token(PARMS[pi], parameters[pi]);
                for (int j = 0; j < (int)strlen(parameters[pi]); ++j) {
                    parameters[pi][j] = tolower(parameters[pi][j]);
                }
            }

            // First char uppercase, rest lowercase.
            if (msg[i + 1] == 'C') {
                copy[i + 1] = 's';
                packed_to_token(PARMS[pi], parameters[pi]);
                for (int j = 0; j < (int)strlen(parameters[pi]); ++j) {
                    parameters[pi][j] = tolower(parameters[pi][j]);
                }
                parameters[pi][0] = toupper(parameters[pi][0]);
            }
        }
    }

    // Render the final string.
    char rendered[2000]; // FIXME: to be replaced with dynamic allocation
    sprintf(rendered, copy, parameters[1], parameters[2], parameters[3], parameters[4]); // FIXME: to be replaced with vsprintf()

    // Print the message.
    printf("%s\n", rendered);

    free(copy);
}

void PSPEAK(vocab_t msg, int skip)
/*  Find the skip+1st message from msg and print it.  msg should be
 *  the index of the inventory message for object.  (INVEN+N+1 message
 *  is game.prop=N message). */
{
    if (skip >= 0)
        speak(object_descriptions[msg].longs[skip]);
    else
        speak(object_descriptions[msg].inventory);
}

void RSPEAK(vocab_t i)
/* Print the i-th "random" message (section 6 of database). */
{
    speak(arbitrary_messages[i]);
}

void SETPRM(long first, long p1, long p2)
/*  Stores parameters into the PRMCOM parms array for use by speak.  P1 and P2
 *  are stored into PARMS(first) and PARMS(first+1). */
{
    if (first >= MAXPARMS)
        BUG(29);
    else {
        PARMS[first] = p1;
        PARMS[first + 1] = p2;
    }
}

bool GETIN(FILE *input,
           long *pword1, long *pword1x,
           long *pword2, long *pword2x)
/*  Get a command from the adventurer.  Snarf out the first word, pad it with
 *  blanks, and return it in WORD1.  Chars 6 thru 10 are returned in WORD1X, in
 *  case we need to print out the whole word in an error message.  Any number of
 *  blanks may follow the word.  If a second word appears, it is returned in
 *  WORD2 (chars 6 thru 10 in WORD2X), else WORD2 is -1. */
{
    long junk;

    for (;;) {
        if (game.blklin)
            fputc('\n', stdout);;
        if (!MAPLIN(input))
            return false;
        *pword1 = GETTXT(true, true, true);
        if (game.blklin && *pword1 < 0)
            continue;
        *pword1x = GETTXT(false, true, true);
        do {
            junk = GETTXT(false, true, true);
        } while
        (junk > 0);
        *pword2 = GETTXT(true, true, true);
        *pword2x = GETTXT(false, true, true);
        do {
            junk = GETTXT(false, true, true);
        } while
        (junk > 0);
        if (GETTXT(true, true, true) <= 0)
            return true;
        RSPEAK(TWO_WORDS);
    }
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
  while (true)
    {
      if (editline)
	input = linenoise(input_prompt);
      else
	{
	  input = NULL;
	  size_t n = 0;
	  if (isatty(0))
	    printf("%s", input_prompt);
	  getline(&input, &n, stdin);
	}
      
      if (input == NULL) // Got EOF; quit.
	exit(EXIT_SUCCESS);
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
    
  return(input);
}

bool YES(vocab_t question, vocab_t yes_response, vocab_t no_response)
/*  Print message X, wait for yes/no answer.  If yes, print Y and return true;
 *  if no, print Z and return false. */
{
    char* reply;
    bool outcome;
   
    for (;;) {
	RSPEAK(question);

	reply = get_input();

	char* firstword = (char*) xmalloc(strlen(reply));
	sscanf(reply, "%s", firstword);

	for (int i = 0; i < strlen(firstword); ++i)
	    firstword[i] = tolower(firstword[i]);
	
	int yes = strncmp("yes", reply, sizeof("yes") - 1);
	int y = strncmp("y", reply, sizeof("y") - 1);
	int no = strncmp("no", reply, sizeof("no") - 1);
	int n = strncmp("n", reply, sizeof("n") - 1);
	
	if (yes == 0 || y == 0) {
	    RSPEAK(yes_response);
	    outcome = true;
	    break;
	}
	else if (no == 0 || n == 0) {
   	    RSPEAK(no_response);
	    outcome = false;
	    break;
	}
	else
	    RSPEAK(PLEASE_ANSWER);
    }
    linenoiseFree(reply);
    return(outcome);
}

/*  Line-parsing routines (GETTXT, MAKEWD, PUTTXT, SHFTXT) */

long GETTXT(bool skip, bool onewrd, bool upper)
/*  Take characters from an input line and pack them into 30-bit words.
 *  Skip says to skip leading blanks.  ONEWRD says stop if we come to a
 *  blank.  UPPER says to map all letters to uppercase.  If we reach the
 *  end of the line, the word is filled up with blanks (which encode as 0's).
 *  If we're already at end of line when TEXT is called, we return -1. */
{
    long text;
    static long splitting = -1;

    if (LNPOSN != splitting)
        splitting = -1;
    text = -1;
    while (true) {
        if (LNPOSN > LNLENG)
            return (text);
        if ((!skip) || INLINE[LNPOSN] != 0)
            break;
        ++LNPOSN;
    }

    text = 0;
    for (int I = 1; I <= TOKLEN; I++) {
        text = text * 64;
        if (LNPOSN > LNLENG || (onewrd && INLINE[LNPOSN] == 0))
            continue;
        char current = INLINE[LNPOSN];
        if (current < ascii_to_advent['%']) {
            splitting = -1;
            if (upper && current >= ascii_to_advent['a'])
                current = current - 26;
            text = text + current;
            ++LNPOSN;
            continue;
        }
        if (splitting != LNPOSN) {
            text = text + ascii_to_advent['%'];
            splitting = LNPOSN;
            continue;
        }

        text = text + current - ascii_to_advent['%'];
        splitting = -1;
        ++LNPOSN;
    }

    return text;
}

token_t MAKEWD(long letters)
/*  Combine TOKLEN (currently 5) uppercase letters (represented by
 *  pairs of decimal digits in lettrs) to form a 30-bit value matching
 *  the one that GETTXT would return given those characters plus
 *  trailing blanks.  Caution: lettrs will overflow 31 bits if
 *  5-letter word starts with V-Z.  As a kludgey workaround, you can
 *  increment a letter by 5 by adding 50 to the next pair of
 *  digits. */
{
    long i = 1, word = 0;

    for (long k = letters; k != 0; k = k / 100) {
        word = word + i * (MOD(k, 50) + 10);
        i = i * 64;
        if (MOD(k, 100) > 50)word = word + i * 5;
    }
    i = 64L * 64L * 64L * 64L * 64L / i;
    word = word * i;
    return word;
}

/*  Data structure  routines */

long VOCAB(long id, long init)
/*  Look up ID in the vocabulary (ATAB) and return its "definition" (KTAB), or
 *  -1 if not found.  If INIT is positive, this is an initialisation call setting
 *  up a keyword variable, and not finding it constitutes a bug.  It also means
 *  that only KTAB values which taken over 1000 equal INIT may be considered.
 *  (Thus "STEPS", which is a motion verb as well as an object, may be located
 *  as an object.)  And it also means the KTAB value is taken modulo 1000. */
{
    long lexeme;

    for (long i = 1; i <= TABSIZ; i++) {
        if (KTAB[i] == -1) {
            lexeme = -1;
            if (init < 0)
                return (lexeme);
            BUG(5);
        }
        if (init >= 0 && KTAB[i] / 1000 != init)
            continue;
        if (ATAB[i] == id) {
            lexeme = KTAB[i];
            if (init >= 0)
                lexeme = MOD(lexeme, 1000);
            return (lexeme);
        }
    }
    BUG(21);
}

void JUGGLE(long object)
/*  Juggle an object by picking it up and putting it down again, the purpose
 *  being to get the object to the front of the chain of things at its loc. */
{
    long i, j;

    i = game.place[object];
    j = game.fixed[object];
    MOVE(object, i);
    MOVE(object + NOBJECTS, j);
}

void MOVE(long object, long where)
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
    if (from != NOWHERE && from != CARRIED && !SPECIAL(from))
        CARRY(object, from);
    DROP(object, where);
}

long PUT(long object, long where, long pval)
/*  PUT is the same as MOVE, except it returns a value used to set up the
 *  negated game.prop values for the repository objects. */
{
    MOVE(object, where);
    return (-1) - pval;;
}

void CARRY(long object, long where)
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

void DROP(long object, long where)
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

long ATDWRF(long where)
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
 *  randrange, RNDVOC, BUG) */

long SETBIT(long bit)
/*  Returns 2**bit for use in constructing bit-masks. */
{
    return (1 << bit);
}

bool TSTBIT(long mask, int bit)
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

long RNDVOC(long second, long force)
/*  Searches the vocabulary ATAB for a word whose second character is
 *  char, and changes that word such that each of the other four
 *  characters is a random letter.  If force is non-zero, it is used
 *  as the new word.  Returns the new word. */
{
    long rnd = force;

    if (rnd == 0) {
        for (int i = 1; i <= 5; i++) {
            long j = 11 + randrange(26);
            if (i == 2)
                j = second;
            rnd = rnd * 64 + j;
        }
    }

    long div = 64L * 64L * 64L;
    for (int i = 1; i <= TABSIZ; i++) {
        if (MOD(ATAB[i] / div, 64L) == second) {
            ATAB[i] = rnd;
            break;
        }
    }

    return rnd;
}

void BUG(long num)
/*  The following conditions are currently considered fatal bugs.  Numbers < 20
 *  are detected while reading the database; the others occur at "run time".
 *	0	Message line > 70 characters
 *	1	Null line in message
 *	2	Too many words of messages
 *	3	Too many travel options
 *	4	Too many vocabulary words
 *	5	Required vocabulary word not found
 *	6	Too many RTEXT messages
 *	7	Too many hints
 *	8	Location has cond bit being set twice
 *	9	Invalid section number in database
 *	10	Too many locations
 *	11	Too many class or turn messages
 *	20	Special travel (500>L>300) exceeds goto list
 *	21	Ran off end of vocabulary table
 *	22	Vocabulary type (N/1000) not between 0 and 3
 *	23	Intransitive action verb exceeds goto list
 *	24	Transitive action verb exceeds goto list
 *	25	Conditional travel entry with no alternative
 *	26	Location has no travel entries
 *	27	Hint number exceeds goto list
 *	28	Invalid month returned by date function
 *	29	Too many parameters given to SETPRM */
{

    printf("Fatal error %ld.  See source code for interpretation.\n", num);
    exit(0);
}

/*  Machine dependent routines (MAPLIN, SAVEIO) */

bool MAPLIN(FILE *fp)
{
    bool eof;

    /* Read a line of input, from the specified input source.
     * This logic is complicated partly because it has to serve
     * several cases with different requirements and partly because
     * of a quirk in linenoise().
     *
     * The quirk shows up when you paste a test log from the clipboard
     * to the program's command prompt.  While fgets (as expected)
     * consumes it a line at a time, linenoise() returns the first
     * line and discards the rest.  Thus, there needs to be an
     * editline (-s) option to fall back to fgets while still
     * prompting.  Note that linenoise does behave properly when
     * fed redirected stdin.
     *
     * The logging is a bit of a mess because there are two distinct cases
     * in which you want to echo commands.  One is when shipping them to
     * a log under the -l option, in which case you want to suppress
     * prompt generation (so test logs are unadorned command sequences).
     * On the other hand, if you redirected stdin and are feeding the program
     * a logfile, you *do* want prompt generation - it makes checkfiles
     * easier to read when the commands are marked by a preceding prompt.
     */
    do {
        if (!editline) {
            if (prompt)
                fputs("> ", stdout);
            IGNORE(fgets(rawbuf, sizeof(rawbuf) - 1, fp));
            eof = (feof(fp));
        } else {
            char *cp = linenoise("> ");
            eof = (cp == NULL);
            if (!eof) {
                strncpy(rawbuf, cp, sizeof(rawbuf) - 1);
                linenoiseHistoryAdd(rawbuf);
                strncat(rawbuf, "\n", sizeof(rawbuf) - strlen(rawbuf) - 1);
                linenoiseFree(cp);
            }
        }
    } while
    (!eof && rawbuf[0] == '#');
    if (eof) {
        if (logfp && fp == stdin)
            fclose(logfp);
        return false;
    } else {
        FILE *efp = NULL;
        if (logfp && fp == stdin)
            efp = logfp;
        else if (!isatty(0))
            efp = stdout;
        if (efp != NULL) {
            if (prompt && efp == stdout)
                fputs("> ", efp);
            IGNORE(fputs(rawbuf, efp));
        }
        strcpy(INLINE + 1, rawbuf);
        /*  translate the chars to integers in the range 0-126 and store
         *  them in the common array "INLINE".  Integer values are as follows:
         *     0   = space [ASCII CODE 40 octal, 32 decimal]
         *    1-2  = !" [ASCII 41-42 octal, 33-34 decimal]
         *    3-10 = '()*+,-. [ASCII 47-56 octal, 39-46 decimal]
         *   11-36 = upper-case letters
         *   37-62 = lower-case letters
         *    63   = percent (%) [ASCII 45 octal, 37 decimal]
         *   64-73 = digits, 0 through 9
         *  Remaining characters can be translated any way that is convenient;
         *  The above mappings are required so that certain special
         *  characters are known to fit in 6 bits and/or can be easily spotted.
         *  Array elements beyond the end of the line should be filled with 0,
         *  and LNLENG should be set to the index of the last character.
         *
         *  If the data file uses a character other than space (e.g., tab) to
         *  separate numbers, that character should also translate to 0.
         *
         *  This procedure may use the map1,map2 arrays to maintain static data for
         *  the mapping.  MAP2(1) is set to 0 when the program starts
         *  and is not changed thereafter unless the routines on this page choose
         *  to do so. */
        LNLENG = 0;
        for (long i = 1; i <= (long)sizeof(INLINE) && INLINE[i] != 0; i++) {
            long val = INLINE[i];
            INLINE[i] = ascii_to_advent[val];
            if (INLINE[i] != 0)
                LNLENG = i;
        }
        LNPOSN = 1;
        return true;
    }
}

void DATIME(long* d, long* t)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    *d = (long) tv.tv_sec;
    *t = (long) tv.tv_usec;
}

/* end */
