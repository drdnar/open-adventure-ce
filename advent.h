#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>

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
#define WORD_NOT_FOUND  -1              /* "Word not found" flag value for the vocab hash functions. */

typedef long token_t;	/* word token - someday this will be char[TOKLEN+1] */
typedef long vocab_t;	/* index into a vocabulary array */

extern const char advent_to_ascii[128];
extern const char ascii_to_advent[128];
extern const char new_advent_to_ascii[64];
extern const char new_ascii_to_advent[128];

enum bugtype {
   TOO_MANY_VOCABULARY_WORDS,                             // 4
   REQUIRED_VOCABULARY_WORD_NOT_FOUND,                    // 5
   INVALID_SECTION_NUMBER_IN_DATABASE,                    // 9
   SPECIAL_TRAVEL_500_GT_L_GT_300_EXCEEDS_GOTO_LIST = 20, // 20
   RAN_OFF_END_OF_VOCABULARY_TABLE,                       // 21
   VOCABULARY_TYPE_N_OVER_1000_NOT_BETWEEN_0_AND_3,       // 22
   INTRANSITIVE_ACTION_VERB_EXCEEDS_GOTO_LIST,            // 23
   TRANSITIVE_ACTION_VERB_EXCEEDS_GOTO_LIST,              // 24
   CONDITIONAL_TRAVEL_ENTRY_WITH_NO_ALTERATION,           // 25
   LOCATION_HAS_NO_TRAVEL_ENTRIES,                        // 26
   HINT_NUMBER_EXCEEDS_GOTO_LIST,                         // 27
   TOO_MANY_PARAMETERS_GIVEN_TO_SETPRM,                   // 28
   SPEECHPART_NOT_TRANSITIVE_OR_INTRANSITIVE_OR_UNKNOWN=99, // 99
   ACTION_RETURNED_PHASE_CODE_BEYOND_END_OF_SWITCH,       // 100
};

/* Alas, declaring this static confuses the coverage analyzer */
void bug(enum bugtype, const char *) __attribute__((__noreturn__));
#define BUG(x) bug(x, #x)

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
    char zzword[6];
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

enum speaktype {touch, look, hear, study, change};

/* b is not needed for POSIX but harmless */
#define READ_MODE "rb"
#define WRITE_MODE "wb"
extern char* xstrdup(const char* s);
extern void* xmalloc(size_t size);
extern void packed_to_token(long, char token[]);
extern long token_to_packed(const char token[6]);
extern void tokenize(char*, long tokens[4]);
extern void vspeak(const char*, va_list);
extern bool wordeq(token_t, token_t);
extern bool wordempty(token_t);
extern void wordclear(token_t *);
extern void speak(const char*, ...);
extern void pspeak(vocab_t, enum speaktype, int, ...);
extern void rspeak(vocab_t, ...);
extern bool GETIN(FILE *, token_t*, token_t*, token_t*, token_t*);
extern void echo_input(FILE*, char*, char*);
extern int word_count(char*);
extern char* get_input(void);
extern bool silent_yes(void);
extern bool yes(const char*, const char*, const char*);
extern long GETTXT(bool, bool, bool);
extern token_t MAKEWD(long);
extern long vocab(long, long);
extern int get_motion_vocab_id(const char*);
extern int get_object_vocab_id(const char*);
extern int get_action_vocab_id(const char*);
extern int get_special_vocab_id(const char*);
extern long get_vocab_id(const char*);
extern void juggle(long);
extern void move(long, long);
extern long put(long, long, long);
extern void carry(long, long);
extern void drop(long, long);
extern long atdwrf(long);
extern long setbit(long);
extern bool tstbit(long, int);
extern long rndvoc(long, long);
extern void make_zzword(char*);
extern bool MAPLIN(FILE *);
extern void datime(long*, long*);

enum termination {endgame, quitgame, scoregame};

extern void set_seed(long);
extern unsigned long get_next_lcg_value(void);
extern long randrange(long);
extern long score(enum termination);
extern void terminate(enum termination) __attribute__((noreturn));
extern int savefile(FILE *, long);
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

#define DESTROY(N)	move(N, LOC_NOWHERE)
#define MOD(N,M)	((N) % (M))
#define TOTING(OBJ)	(game.place[OBJ] == CARRIED)
#define AT(OBJ) (game.place[OBJ] == game.loc || game.fixed[OBJ] == game.loc)
#define HERE(OBJ)	(AT(OBJ) || TOTING(OBJ))
#define LIQ2(PBOTL)	((1-(PBOTL))*WATER+((PBOTL)/2)*(WATER+OIL))
#define LIQUID()	(LIQ2(game.prop[BOTTLE]<0 ? -1-game.prop[BOTTLE] : game.prop[BOTTLE]))
#define LIQLOC(LOC)	(LIQ2((MOD(conditions[LOC]/2*2,8)-5)*MOD(conditions[LOC]/4,2)+1))
#define CNDBIT(L,N)	(tstbit(conditions[L],N))
#define FORCED(LOC)	CNDBIT(LOC, COND_FORCED)
#define DARK(DUMMY)	((!tstbit(conditions[game.loc],COND_LIT)) && (game.prop[LAMP] == LAMP_DARK || !HERE(LAMP)))
#define PCT(N)		(randrange(100) < (N))
#define GSTONE(OBJ)	((OBJ) == EMERALD || (OBJ) == RUBY || (OBJ) == AMBER || (OBJ) == SAPPH)
#define FOREST(LOC)	CNDBIT(LOC, COND_FOREST)
#define SPECIAL(LOC)	((LOC) > SPECIALBASE)
#define OUTSID(LOC)	(CNDBIT(LOC, COND_ABOVE) || FOREST(LOC))

#define INDEEP(LOC)	((LOC) >= LOC_MISTHALL && !OUTSID(LOC))

enum speechpart {unknown, intransitive, transitive};

struct command_t {
    enum speechpart part;
    vocab_t verb;
    vocab_t obj;
    token_t wd1, wd1x;
    token_t wd2, wd2x;
};

void initialise(void);
int action(struct command_t *command);

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
#define WORD_CATCH	301200308
#define WORD_GO		715
#define WORD_POUR	16152118
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

