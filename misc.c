#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "advent.h"
#include "database.h"
#include "linenoise/linenoise.h"

/* hack to ignore GCC Unused Result */
#define IGNORE(r) do{if (r){}}while(0)

#define PERCENT	63	/* partly hide the packed encoding */

const char advent_to_ascii[] = {0, 32, 33, 34, 39, 40, 41, 42, 43, 44, 45, 46, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 37, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 1, 2, 3, 4, 5, 6, 7, 8, 0, 0, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 35, 36, 38, 47, 58, 59, 60, 61, 62, 63, 64, 91, 92, 93, 94, 95, 96, 123, 124, 125, 126, 0};
/* Rendered from the now-gone MPINIT() function */
const char ascii_to_advent[] = {0, 74, 75, 76, 77, 78, 79, 80, 81, 82, 0, 0, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 0, 1, 2, 106, 107, 63, 108, 3, 4, 5, 6, 7, 8, 9, 10, 109, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 110, 111, 112, 113, 114, 115, 116, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 117, 118, 119, 120, 121, 122, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 123, 124, 125, 126, 83};

/*  I/O routines (SPEAK, PSPEAK, RSPEAK, SETPRM, GETIN, YES) */

void SPEAK(vocab_t msg)
/*  Print the message which starts at LINES[N].  Precede it with a blank line
 *  unless game.blklin is false. */
{
    long blank, casemake, i, nxt, neg, nparms, param, prmtyp, state;

    if (msg == 0)
	return;
    blank=game.blklin;
    nparms=1;
    do {
	nxt=labs(LINES[msg])-1;
	msg=msg+1;
	LNLENG=0;
	LNPOSN=1;
	state=0;
	for (i = msg; i <= nxt; i++) {
	    PUTTXT(LINES[i],&state,2);
	}
	LNPOSN=0;
	++LNPOSN;

	while (LNPOSN <= LNLENG) { 
	    if (INLINE[LNPOSN] != PERCENT) {
		++LNPOSN;
		continue;
	    }
	    prmtyp = INLINE[LNPOSN+1];
	    /*  A "%"; the next character determine the type of
	     *  parameter: 1 (!) = suppress message completely, 29 (S) = NULL
	     *  If PARAM=1, else 'S' (optional plural ending), 33 (W) = word
	     *  (two 30-bit values) with trailing spaces suppressed, 22 (L) or
	     *  31 (U) = word but map to lower/upper case, 13 (C) = word in
	     *  lower case with first letter capitalised, 30 (T) = text ending
	     *  with a word of -1, 65-73 (1-9) = number using that many
	     *  characters, 12 (B) = variable number of blanks. */
	    if (prmtyp == 1)
		return;
	    if (prmtyp == 29) {
		SHFTXT(LNPOSN+2,-1);
		INLINE[LNPOSN] = 55;
		if (PARMS[nparms] == 1)
		    SHFTXT(LNPOSN+1,-1);
		++nparms;
		continue;
	    }
	    if (prmtyp == 30) {
		SHFTXT(LNPOSN+2,-2);
		state=0;
		casemake=2;

		while (PARMS[nparms] > 0) {
		    if (PARMS[nparms+1] < 0)
			casemake=0;
		    PUTTXT(PARMS[nparms],&state,casemake);
		    nparms=nparms+1;
		}
		++nparms;
		continue;
	    }
	    if (prmtyp == 12) {
		prmtyp=PARMS[nparms];
		SHFTXT(LNPOSN+2,prmtyp-2);
		if (prmtyp != 0) {
		    for (i=1; i<=prmtyp; i++) {
			INLINE[LNPOSN]=0;
			++LNPOSN;
		    }
		}
		++nparms;
		continue;
	    }
	    if (prmtyp == 33 || prmtyp == 22 || prmtyp == 31 || prmtyp == 13) {
		SHFTXT(LNPOSN+2,-2);
		state = 0;
		casemake = -1;
		if (prmtyp == 31)
		    casemake=1;
		if (prmtyp == 33)
		    casemake=0;
		i = LNPOSN;
		PUTTXT(PARMS[nparms],&state,casemake);
		PUTTXT(PARMS[nparms+1],&state,casemake);
		if (prmtyp == 13 && INLINE[i] >= 37 && INLINE[i] <= 62)
		    INLINE[i] -= 26;
		nparms += 2;
		continue;
	    }

	    prmtyp=prmtyp-64;
	    if (prmtyp < 1 || prmtyp > 9) {
		++LNPOSN;
		continue;
	    }
	    SHFTXT(LNPOSN+2,prmtyp-2);
	    LNPOSN += prmtyp;
	    param=labs(PARMS[nparms]);
	    neg=0;
	    if (PARMS[nparms] < 0)
		neg=9;
	    for (i=1; i <= prmtyp; i++) {
		--LNPOSN;
		INLINE[LNPOSN]=MOD(param,10)+64;
		if (i != 1 && param == 0) {
		    INLINE[LNPOSN]=neg;
		    neg=0;
		}
		param=param/10;
	    }
	    LNPOSN += prmtyp;
	    ++nparms;
	    continue;
	}

	if (blank)
	    TYPE0();
	blank=false;
	TYPE();
	msg = nxt + 1;
    } while
	(LINES[msg] >= 0);
}

void PSPEAK(vocab_t msg,int skip)
/*  Find the skip+1st message from msg and print it.  msg should be
 *  the index of the inventory message for object.  (INVEN+N+1 message
 *  is game.prop=N message). */
{
    long i, m;

    m=PTEXT[msg];
    if (skip >= 0) {
	for (i=0; i <=skip; i++) {
	    do {
		m=labs(LINES[m]);
	    } while
		(LINES[m] >= 0);
	}
    }
    SPEAK(m);
}

void RSPEAK(vocab_t i)
/* Print the i-th "random" message (section 6 of database). */
{
    if (i != 0)
	SPEAK(RTEXT[i]);
}

void SETPRM(long first, long p1, long p2)
/*  Stores parameters into the PRMCOM parms array for use by speak.  P1 and P2
 *  are stored into PARMS(first) and PARMS(first+1). */
{
    if (first >= MAXPARMS)
	BUG(29);
    else {
	PARMS[first] = p1;
	PARMS[first+1] = p2;
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
	    TYPE0();
	if (!MAPLIN(input))
	    return false;
	*pword1=GETTXT(true,true,true);
	if (game.blklin && *pword1 < 0)
	    continue;
	*pword1x=GETTXT(false,true,true);
	do {	
	    junk=GETTXT(false,true,true);
	} while 
	    (junk > 0);
	*pword2=GETTXT(true,true,true);
	*pword2x=GETTXT(false,true,true);
	do {
	    junk=GETTXT(false,true,true);
	} while 
	    (junk > 0);
	if (GETTXT(true,true,true) <= 0)
	    return true;
	RSPEAK(53);
    }
}

long YES(FILE *input, vocab_t x, vocab_t y, vocab_t z)
/*  Print message X, wait for yes/no answer.  If yes, print Y and return true;
 *  if no, print Z and return false. */
{
    token_t reply, junk1, junk2, junk3;

    for (;;) {
	RSPEAK(x);
	GETIN(input, &reply, &junk1, &junk2, &junk3);
	if (reply == MAKEWD(250519) || reply == MAKEWD(25)) {
	    RSPEAK(y);
	    return true;
	}
	if (reply == MAKEWD(1415) || reply == MAKEWD(14)) {
	    RSPEAK(z);
	    return false;
	}
	RSPEAK(185);
    }
}

/*  Line-parsing routines (GETTXT, MAKEWD, PUTTXT, SHFTXT, TYPE0) */

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
    text= -1;
    while (true) {
	if (LNPOSN > LNLENG)
	    return(text);
	if ((!skip) || INLINE[LNPOSN] != 0)
	    break;
	LNPOSN=LNPOSN+1;
    }

    text=0;
    for (int I=1; I<=TOKLEN; I++) {
	text=text*64;
	if (LNPOSN > LNLENG || (onewrd && INLINE[LNPOSN] == 0))
	    continue;
	char current=INLINE[LNPOSN];
	if (current < PERCENT) {
	    splitting = -1;
	    if (upper && current >= 37)
		current=current-26;
	    text=text+current;
	    LNPOSN=LNPOSN+1;
	    continue;
	}
	if (splitting != LNPOSN) {
	    text=text+PERCENT;
	    splitting = LNPOSN;
	    continue;
	}

	text=text+current-PERCENT;
	splitting = -1;
	LNPOSN=LNPOSN+1;
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

    for (long k=letters; k != 0; k=k/100) {
	word=word+i*(MOD(k,50)+10);
	i=i*64;
	if (MOD(k,100) > 50)word=word+i*5;
    }
    i=64L*64L*64L*64L*64L/i;
    word=word*i;
    return word;
}

void PUTTXT(token_t word, long *state, long casemake)
/*  Unpack the 30-bit value in word to obtain up to TOKLEN (currently
 *  5) integer-encoded chars, and store them in inline starting at
 *  LNPOSN.  If LNLENG>=LNPOSN, shift existing characters to the right
 *  to make room.  STATE will be zero when puttxt is called with the
 *  first of a sequence of words, but is thereafter unchanged by the
 *  caller, so PUTTXT can use it to maintain state across calls.
 *  LNPOSN and LNLENG are incremented by the number of chars stored.
 *  If CASEMAKE=1, all letters are made uppercase; if -1, lowercase; if 0,
 *  as is.  any other value for case is the same as 0 but also causes
 *  trailing blanks to be included (in anticipation of subsequent
 *  additional text). */
{
    long alph1, alph2, byte, div, i, w;

    alph1=13*casemake+24;
    alph2=26*labs(casemake)+alph1;
    if (labs(casemake) > 1)
	alph1=alph2;
    /*  alph1&2 define range of wrong-case chars, 11-36 or 37-62 or empty. */
    div=64L*64L*64L*64L;
    w=word;
    for (i=1; i<=TOKLEN; i++) 
    {
	if (w <= 0 && *state == 0 && labs(casemake) <= 1)
	    return;
	byte=w/div;
	w=(w-byte*div)*64;
	if (!(*state != 0 || byte != PERCENT)) {
	    *state=PERCENT;
	    continue;
	}
	SHFTXT(LNPOSN,1);
	*state=*state+byte;
	if (*state < alph2 && *state >= alph1)*state=*state-26*casemake;
	INLINE[LNPOSN]=*state;
	LNPOSN=LNPOSN+1;
	*state=0;
    }
}
#define PUTTXT(WORD,STATE,CASE) fPUTTXT(WORD,&STATE,CASE)

void SHFTXT(long from, long delta) 
/*  Move INLINE(N) to INLINE(N+DELTA) for N=FROM,LNLENG.  Delta can be
 *  negative.  LNLENG is updated; LNPOSN is not changed. */
{
    long I, k, j;

    if (!(LNLENG < from || delta == 0)) {
	for (I=from; I<=LNLENG; I++) {
	    k=I;
	    if (delta > 0)
		k=from+LNLENG-I;
	    j=k+delta;
	    INLINE[j]=INLINE[k];
	}
    }
    LNLENG=LNLENG+delta;
}

void TYPE0(void)
/*  Type a blank line.  This procedure is provided as a convenience for callers
 *  who otherwise have no use for MAPCOM. */
{
    long temp;

    temp=LNLENG;
    LNLENG=0;
    TYPE();
    LNLENG=temp;
    return;
}

/*  Suspend/resume I/O routines (SAVWDS, SAVARR, SAVWRD) */

void fSAVWDS(long *W1, long *W2, long *W3, long *W4,
	     long *W5, long *W6, long *W7)
/* Write or read 7 variables.  See SAVWRD. */
{
    SAVWRD(0,(*W1));
    SAVWRD(0,(*W2));
    SAVWRD(0,(*W3));
    SAVWRD(0,(*W4));
    SAVWRD(0,(*W5));
    SAVWRD(0,(*W6));
    SAVWRD(0,(*W7));
}

void fSAVARR(long arr[], long n)
/* Write or read an array of n words.  See SAVWRD. */
{
    long i;

    for (i=1; i<=n; i++) {
	SAVWRD(0,arr[i]);
    }
    return;
}

void fSAVWRD(long op, long *pword) 
/*  If OP<0, start writing a file, using word to initialise encryption; save
 *  word in the file.  If OP>0, start reading a file; read the file to find
 *  the value with which to decrypt the rest.  In either case, if a file is
 *  already open, finish writing/reading it and don't start a new one.  If OP=0,
 *  read/write a single word.  Words are buffered in case that makes for more
 *  efficient disk use.  We also compute a simple checksum to catch elementary
 *  poking within the saved file.  When we finish reading/writing the file,
 *  we store zero into *PWORD if there's no checksum error, else nonzero. */
{
    static long buf[250], cksum = 0, h1, hash = 0, n = 0, state = 0;

    if (op != 0)
    {
	long ifvar = state; 
	switch (ifvar<0 ? -1 : (ifvar>0 ? 1 : 0)) 
	{ 
	case -1:
	case 1:
	    if (n == 250)SAVEIO(1,state > 0,buf);
	    n=MOD(n,250)+1;
	    if (state <= 0) {
		n--; buf[n]=cksum; n++;
		SAVEIO(1,false,buf);
	    }
	    n--; *pword=buf[n]-cksum; n++;
	    SAVEIO(-1,state > 0,buf);
	    state=0;
	    break;
	case 0:	/* FIXME: Huh? should be impossible */
	    state=op;
	    SAVEIO(0,state > 0,buf);
	    n=1;
	    if (state <= 0) {
		hash=MOD(*pword,1048576L);
		buf[0]=1234L*5678L-hash;
	    }
	    SAVEIO(1,true,buf);
	    hash=MOD(1234L*5678L-buf[0],1048576L);
	    cksum=buf[0];
	    return;
	}
    }
    if (state == 0)
	return;
    if (n == 250)
	SAVEIO(1,state > 0,buf);
    n=MOD(n,250)+1;
    h1=MOD(hash*1093L+221573L,1048576L);
    hash=MOD(h1*1093L+221573L,1048576L);
    h1=MOD(h1,1234)*765432+MOD(hash,123);
    n--;
    if (state > 0)
	*pword=buf[n]+h1;
    buf[n]=*pword-h1;
    n++;
    cksum=MOD(cksum*13+*pword,1000000000L);
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
    long i, lexeme;

    for (i=1; i<=TABSIZ; i++) {
	if (KTAB[i] == -1) {
	    lexeme= -1;
	    if (init < 0)
		return(lexeme);
	    BUG(5);
	}
	if (init >= 0 && KTAB[i]/1000 != init) 
	    continue;
	if (ATAB[i] == id) {
	    lexeme=KTAB[i];
	    if (init >= 0)
		lexeme=MOD(lexeme,1000);
	    return(lexeme);
	}
    }
    BUG(21);
}

void DSTROY(long object)
/*  Permanently eliminate "object" by moving to a non-existent location. */
{
    MOVE(object,0);
}

void JUGGLE(long object)
/*  Juggle an object by picking it up and putting it down again, the purpose
 *  being to get the object to the front of the chain of things at its loc. */
{
    long i, j;

    i=game.place[object];
    j=game.fixed[object];
    MOVE(object,i);
    MOVE(object+NOBJECTS,j);
}

void MOVE(long object, long where)
/*  Place any object anywhere by picking it up and dropping it.  May
 *  already be toting, in which case the carry is a no-op.  Mustn't
 *  pick up objects which are not at any loc, since carry wants to
 *  remove objects from game.atloc chains. */
{
    long from;

    if (object > NOBJECTS) 
	from=game.fixed[object-NOBJECTS];
    else
	from=game.place[object];
    if (from > 0 && from <= 300)
	CARRY(object,from);
    DROP(object,where);
}

long PUT(long object, long where, long pval)
/*  PUT is the same as MOVE, except it returns a value used to set up the
 *  negated game.prop values for the repository objects. */
{
    MOVE(object,where);
    return (-1)-pval;;
}

void CARRY(long object, long where) 
/*  Start toting an object, removing it from the list of things at its former
 *  location.  Incr holdng unless it was already being toted.  If object>NOBJECTS
 *  (moving "fixed" second loc), don't change game.place or game.holdng. */
{
    long temp;

    if (object <= NOBJECTS) {
	if (game.place[object] == -1)
	    return;
	game.place[object]= -1;
	game.holdng=game.holdng+1;
    }
    if (game.atloc[where] == object) {
	game.atloc[where]=game.link[object];
	return;
    }
    temp=game.atloc[where];
    while (game.link[temp] != object) {
	temp=game.link[temp];
    }
    game.link[temp]=game.link[object];
}

void DROP(long object, long where)
/*  Place an object at a given loc, prefixing it onto the game.atloc list.  Decr
 *  game.holdng if the object was being toted. */
{
    if (object > NOBJECTS)
	game.fixed[object-NOBJECTS] = where;
    else
    {
	if (game.place[object] == -1)
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
    long at, i;

    at =0;
    if (game.dflag < 2)
	return(at);
    at = -1;
    for (i=1; i<=NDWARVES-1; i++) {
	if (game.dloc[i] == where)
	    return i;
	if (game.dloc[i] != 0)
	    at=0;
    }
    return(at);
}

/*  Utility routines (SETBIT, TSTBIT, set_seed, get_next_lcg_value,
 *  randrange, RNDVOC, BUG) */

long SETBIT(long bit)
/*  Returns 2**bit for use in constructing bit-masks. */
{
    return(1 << bit);
}

bool TSTBIT(long mask, int bit)
/*  Returns true if the specified bit is set in the mask. */
{
    return (mask & (1 << bit)) != 0;
}

void set_seed(long seedval)
/* Set the LCG seed */
{
    lcgstate.x = (unsigned long) seedval % lcgstate.m;
}

unsigned long get_next_lcg_value(void)
/* Return the LCG's current value, and then iterate it. */
{
    unsigned long old_x = lcgstate.x;
    lcgstate.x = (lcgstate.a * lcgstate.x + lcgstate.c) % lcgstate.m;
    return old_x;
}

long randrange(long range)
/* Return a random integer from [0, range). */
{
    return range * get_next_lcg_value() / lcgstate.m;
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
	if (MOD(ATAB[i]/div, 64L) == second)
	{
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

/*  Machine dependent routines (MAPLIN, TYPE, SAVEIO) */

bool MAPLIN(FILE *fp)
{
    long i, val;

    /*  Read a line of input, from the specified input source,
     *  translate the chars to integers in the range 0-126 and store
     *  them in the common array "INLINE".  Integer values are as follows:
     *     0   = space [ASCII CODE 40 octal, 32 decimal]
     *    1-2  = !" [ASCII 41-42 octal, 33-34 decimal]
     *    3-10 = '()*+,-. [ASCII 47-56 octal, 39-46 decimal]
     *   11-36 = upper-case letters
     *   37-62 = lower-case letters
     *    63   = percent (%) [ASCII 45 octal, 37 decimal]
     *   64-73 = digits, 0 through 9
     *  Remaining characters can be translated any way that is convenient;
     *  The "TYPE" routine below is used to map them back to characters when
     *  necessary.  The above mappings are required so that certain special
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

    if (!oldstyle && fp == stdin)
	fputs("> ", stdout);
    do {
	IGNORE(fgets(rawbuf,sizeof(rawbuf)-1,fp));
    } while
	    (!feof(fp) && rawbuf[0] == '#');
    if (feof(fp)) {
	if (logfp && fp == stdin)
	    fclose(logfp);
	return false;
    } else {
	if (logfp && fp == stdin)
	    IGNORE(fputs(rawbuf, logfp));
	else if (!isatty(0))
	    IGNORE(fputs(rawbuf, stdout));
	strcpy(INLINE+1, rawbuf);
	LNLENG=0;
	for (i=1; i<=(long)sizeof(INLINE) && INLINE[i]!=0; i++) {
	    val=INLINE[i]+1;
	    INLINE[i]=ascii_to_advent[val];
	    if (INLINE[i] != 0)
		LNLENG=i;
	}
	LNPOSN=1;
	return true;
    }
}

void TYPE(void)
/*  Type the first "LNLENG" characters stored in inline, mapping them
 *  from integers to text per the rules described above.  INLINE
 *  may be changed by this routine. */
{
    long i;

    if (LNLENG == 0) {
	printf("\n");
	return;
    }

    for (i=1; i<=LNLENG; i++) {
	INLINE[i]=advent_to_ascii[INLINE[i]+1];
    }
    INLINE[LNLENG+1]=0;
    printf("%s\n", INLINE+1);
    return;
}

void fSAVEIO(long op, long in, long arr[]) 
/*  If OP=0, ask for a file name and open a file.  (If IN=true, the file is for
 *  input, else output.)  If OP>0, read/write ARR from/into the previously-opened
 *  file.  (ARR is a 250-integer array.)  If OP<0, finish reading/writing the
 *  file.  (Finishing writing can be a no-op if a "stop" statement does it
 *  automatically.  Finishing reading can be a no-op as long as a subsequent
 *  SAVEIO(0,false,X) will still work.) */
{
    static FILE *fp = NULL;
    char* name;

    switch (op < 0 ? -1 : (op > 0 ? 1 : 0)) 
    { 
    case -1:
	fclose(fp);
	break;
    case 0:
	while (fp == NULL) {
	    name = linenoise("File name: ");
	    fp = fopen(name,(in ? READ_MODE : WRITE_MODE));
	    if (fp == NULL)
		printf("Can't open file %s, try again.\n", name); 
	}
	linenoiseFree(name);
	break;
    case 1: 
	if (in)
	    IGNORE(fread(arr,sizeof(long),250,fp));
	else
	    IGNORE(fwrite(arr,sizeof(long),250,fp));
	break;
    }
}

void DATIME(long* d, long* t)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    *d = (long) tv.tv_sec;
    *t = (long) tv.tv_usec;
}

long MOD(long n, long m) 
{
    return(n%m);
}
