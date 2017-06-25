#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>

#include "common.h"
#include "newdb.h"

#define LINESIZE	100
#define NDWARVES	6		/* number of dwarves */
#define PIRATE		NDWARVES	/* must be NDWARVES-1 when zero-origin */
#define DALTLC		LOC_NUGGET	/* alternate dwarf location */
#define MAXPARMS	25		/* Max parameters for speak() */
#define INVLIMIT	7		/* inverntory limit (# of objects) */
#define INTRANSITIVE	-1		/* illegal object number */
#define SPECIALBASE	300		/* base number of special rooms */
#define GAMELIMIT	330		/* base limit of turns */
#define NOVICELIMIT	1000	/* limit of turns for novice */
#define WARNTIME	30		/* late game starts at game.limit-this */
#define FLASHTIME	50	/*turns from first warning till blinding flash */
#define PANICTIME	15		/* time left after closing */
#define BATTERYLIFE	2500		/* turn limit increment from batteries */

typedef long token_t;	/* word token - someday this will be char[TOKLEN+1] */
typedef long vocab_t;	/* index into a vocabulary array */

struct game_t {
    unsigned long lcg_a, lcg_c, lcg_m, lcg_x;
    long abbnum;	/* How often to print non-abbreviated descriptions */
    long blklin;
    long bonus;
    long chloc;
    long chloc2;
    long clock1;	/* # turns from finding last treasure till closing */
    long clock2;	/* # turns from first warning till blinding flash */
    bool clshnt;	/* has player read the clue in the endgame? */
    bool closed;	/* whether we're all the way closed */
    bool closng;	/* whether it's closing time yet */
    long conds;		/* min value for cond(loc) if loc has any hints */
    long detail;
    long dflag;
    long dkill;
    long dtotal;
    long foobar;	/* current progress in saying "FEE FIE FOE FOO". */
    long holdng;	/* number of objects being carried */
    long iwest;		/* How many times he's said "west" instead of "w" */
    long knfloc;	/* 0 if no knife here, loc if knife , -1 after caveat */
    long limit;		/* lifetime of lamp (not set here) */
    bool lmwarn;	/* has player been warned about lamp going dim? */
    long loc;
    long newloc;
    bool novice;	/* asked for instructions at start-up? */
    long numdie;	/* number of times killed so far */
    long oldloc;
    long oldlc2;
    long oldobj;
    bool panic;		/* has player found out he's trapped in the cave? */
    long saved;		/* point penalty for saves */
    long tally;
    long thresh;
    long trndex;
    long trnluz;	/* # points lost so far due to number of turns used */
    long turns;		/* how many commands he's given (ignores yes/no) */
    bool wzdark;	/* whether the loc he's leaving was dark */
    long zzword;
    bool blooded;	/* has player drunk of dragon's blood? */
    long abbrev[NLOCATIONS + 1];
    long atloc[NLOCATIONS + 1];
    long dseen[NDWARVES + 1];
    long dloc[NDWARVES + 1];
    long odloc[NDWARVES + 1];
    long fixed[NOBJECTS + 1];
    long link[NOBJECTS * 2 + 1];
    long place[NOBJECTS + 1];
    long hinted[NHINTS];
    long hintlc[NHINTS];
    long prop[NOBJECTS + 1];
};

extern struct game_t game;

extern long LNLENG, LNPOSN;
extern char rawbuf[LINESIZE], INLINE[LINESIZE + 1];
extern const char ascii_to_advent[];
extern const char advent_to_ascii[];
extern FILE *logfp;
extern bool oldstyle, editline, prompt;

enum speaktype {touch, look, hear, study};

/* b is not needed for POSIX but harmless */
#define READ_MODE "rb"
#define WRITE_MODE "wb"
extern void* xmalloc(size_t size);
extern void packed_to_token(long, char token[]);
extern void token_to_packed(char token[], long*);
extern void vspeak(const char*, va_list);
extern bool wordeq(token_t, token_t);
extern bool wordempty(token_t);
extern void wordclear(token_t *);
extern void speak(const char*, ...);
extern void pspeak(vocab_t, enum speaktype, int, ...);
extern void rspeak(vocab_t, ...);
extern bool GETIN(FILE *, token_t*, token_t*, token_t*, token_t*);
extern void echo_input(FILE*, char*, char*);
extern char* get_input(void);
extern bool YES(const char*, const char*, const char*);
extern long GETTXT(bool, bool, bool);
extern token_t MAKEWD(long);
extern long VOCAB(long, long);
extern void JUGGLE(long);
extern void MOVE(long, long);
extern long PUT(long, long, long);
extern void CARRY(long, long);
extern void DROP(long, long);
extern long ATDWRF(long);
extern long SETBIT(long);
extern bool TSTBIT(long, int);
extern long RNDVOC(long, long);
extern bool MAPLIN(FILE *);
extern void DATIME(long*, long*);

enum termination {endgame, quitgame, scoregame};

extern void set_seed(long);
extern unsigned long get_next_lcg_value(void);
extern long randrange(long);
extern long score(enum termination);
extern void terminate(enum termination) __attribute__((noreturn));
extern int suspend(void);
extern int resume(void);
extern int restore(FILE *);

/*
 *  MOD(N,M)	= Arithmetic modulus
 *  AT(OBJ)	= true if on either side of two-placed object
 *  CNDBIT(L,N) = true if COND(L) has bit n set (bit 0 is units bit)
 *  DARK(LOC)   = true if location "LOC" is dark
 *  FORCED(LOC) = true if LOC moves without asking for input (COND=2)
 *  FOREST(LOC) = true if LOC is part of the forest
 *  GSTONE(OBJ) = true if OBJ is a gemstone
 *  HERE(OBJ)	= true if the OBJ is at "LOC" (or is being carried)
 *  LIQUID()	= object number of liquid in bottle
 *  LIQLOC(LOC) = object number of liquid (if any) at LOC
 *  PCT(N)      = true N% of the time (N integer from 0 to 100)
 *  TOTING(OBJ) = true if the OBJ is being carried */

#define DESTROY(N)	MOVE(N, LOC_NOWHERE)
#define MOD(N,M)	((N) % (M))
#define TOTING(OBJ)	(game.place[OBJ] == CARRIED)
#define AT(OBJ) (game.place[OBJ] == game.loc || game.fixed[OBJ] == game.loc)
#define HERE(OBJ)	(AT(OBJ) || TOTING(OBJ))
#define LIQ2(PBOTL)	((1-(PBOTL))*WATER+((PBOTL)/2)*(WATER+OIL))
#define LIQUID()	(LIQ2(game.prop[BOTTLE]<0 ? -1-game.prop[BOTTLE] : game.prop[BOTTLE]))
#define LIQLOC(LOC)	(LIQ2((MOD(conditions[LOC]/2*2,8)-5)*MOD(conditions[LOC]/4,2)+1))
#define CNDBIT(L,N)	(TSTBIT(conditions[L],N))
#define FORCED(LOC)	CNDBIT(LOC, COND_FORCED)
#define DARK(DUMMY)	((!TSTBIT(conditions[game.loc],COND_LIT)) && (game.prop[LAMP] == LAMP_DARK || !HERE(LAMP)))
#define PCT(N)		(randrange(100) < (N))
#define GSTONE(OBJ)	((OBJ) == EMERALD || (OBJ) == RUBY || (OBJ) == AMBER || (OBJ) == SAPPH)
#define FOREST(LOC)	CNDBIT(LOC, COND_FOREST)
#define VOCWRD(LETTRS,SECT)	(VOCAB(MAKEWD(LETTRS),SECT))
#define SPECIAL(LOC)	((LOC) > SPECIALBASE)
#define OUTSID(LOC)	(CNDBIT(LOC, COND_ABOVE) || FOREST(LOC))

#define INDEEP(LOC)	((LOC) >= LOC_MISTHALL && !OUTSID(LOC))

/* vocabulary items */
extern long AMBER, ATTACK, AXE, BACK, BATTERY, BEAR,
       BIRD, BLOOD, BOTTLE, CAGE, CAVE, CAVITY, CHAIN, CHASM, CHEST,
       CLAM, COINS, DOOR, DPRSSN, DRAGON, DWARF, EGGS,
       EMERALD, ENTER, ENTRNC, FIND, FISSURE, FOOD, GRATE, HINT, INVENT,
       JADE, KEYS, KNIFE, LAMP, LOCK, LOOK, MAGAZINE, MESSAG, MIRROR, NUGGET, NUL,
       OGRE, OIL, OYSTER, PANIC, PEARL, PILLOW, PLANT, PLANT2, PYRAMID,
       RESER, ROD, ROD2, RUBY, RUG, SAPPH, SAY, SIGN, SNAKE,
       STEPS, STICK, STREAM, THROW, TRIDENT, TROLL, TROLL2,
       URN, VASE, VEND, VOLCANO, WATER;

enum speechpart {unknown, intransitive, transitive};

struct command_t {
    enum speechpart part;
    vocab_t verb;
    vocab_t obj;
    token_t wd1, wd1x;
    token_t wd2, wd2x;
};

void initialise(void);
int action(FILE *input, struct command_t *command);

/* Phase codes for action returns.
 * These were at one time FORTRAN line numbers.
 * The values don't matter, but perturb their order at your peril.
 */
#define GO_TERMINATE	2
#define GO_MOVE		8
#define GO_TOP		2000
#define GO_CLEAROBJ	2012
#define GO_CHECKHINT	2600
#define GO_CHECKFOO	2607
#define GO_DIRECTION	2620
#define GO_LOOKUP	2630
#define GO_WORD2	2800
#define GO_SPECIALS	1900
#define GO_UNKNOWN	8000
#define GO_ACTION	40000
#define GO_DWARFWAKE	19000

/* Special object statuses in game.place - can also be a location number (> 0) */
#define CARRIED		-1	/* Player is toting it */

/* hack to ignore GCC Unused Result */
#define IGNORE(r) do{if (r){}}while(0)

/*
 * FIXME: These constants should be replaced by strings, at their usage sites.
 * They are sixbit-packed representations of vocabulary words.  This, and code
 * left in misc.c, is the only place left in the runtime that knows about 
 * word packing.
 */
#define WORD_AXE	12405
#define WORD_BATTERY	201202005
#define WORD_BEAR	2050118
#define WORD_BIRD	2091804
#define WORD_BLOOD	212151504
#define WORD_BOTTLE	215202012
#define WORD_CAGE	3010705
#define WORD_CATCH	301200308
#define WORD_CAVITY	301220920
#define WORD_CHASM	308011913
#define WORD_CLAM	3120113
#define WORD_DOOR	4151518
#define WORD_DRAGON	418010715
#define WORD_DWARF	423011806
#define WORD_FISSURE	609191921
#define WORD_FOOD	6151504
#define WORD_GO		715
#define WORD_GRATE	718012005
#define WORD_KEYS	11052519
#define WORD_KNIFE	1114090605
#define WORD_LAMP	12011316
#define WORD_MAGAZINE	1301070126
#define WORD_MESSAG	1305191901
#define WORD_MIRROR	1309181815
#define WORD_OGRE	15071805
#define WORD_OIL	150912
#define WORD_OYSTER	1525192005
#define WORD_PILLOW	1609121215
#define WORD_PLANT	1612011420
#define WORD_POUR	16152118
#define WORD_RESER	1805190518
#define WORD_ROD	181504
#define WORD_SIGN	19090714
#define WORD_SNAKE	1914011105
#define WORD_STEPS	1920051619
#define WORD_TROLL	2018151212
#define WORD_URN	211814
#define WORD_VEND	1755140409
#define WORD_VOLCANO	1765120301
#define WORD_WATER	1851200518
#define WORD_AMBER	113020518
#define WORD_CHAIN	308010914
#define WORD_CHEST	308051920
#define WORD_COINS	315091419
#define WORD_EGGS	5070719
#define WORD_EMERALD	513051801
#define WORD_JADE	10010405
#define WORD_NUGGET	7151204
#define WORD_PEARL	1605011812
#define WORD_PYRAMID	1625180113
#define WORD_RUBY	18210225
#define WORD_RUG	182107
#define WORD_SAPPH	1901161608
#define WORD_TRIDENT	2018090405
#define WORD_VASE	22011905
#define WORD_BACK	2010311
#define WORD_CAVE	3012205
#define WORD_DPRSSN	405161805
#define WORD_ENTER	514200518
#define WORD_ENTRNC	514201801
#define WORD_LOOK	12151511
#define WORD_NUL	14211212
#define WORD_STREAM	1920180501
#define WORD_FIND	6091404
#define WORD_INVENT	914220514
#define WORD_LOCK	12150311
#define WORD_SAY	190125
#define WORD_THROW	2008181523
#define WORD_WEST	23051920
#define WORD_YES	250519
#define WORD_YINIT	25

/* end */

