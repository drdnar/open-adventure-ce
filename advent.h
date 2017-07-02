#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>

#include "dungeon.h"

#define LINESIZE       1024
#define NDWARVES       6          // number of dwarves
#define PIRATE         NDWARVES   // must be NDWARVES-1 when zero-origin
#define DALTLC         LOC_NUGGET // alternate dwarf location
#define INVLIMIT       7          // inverntory limit (# of objects)
#define INTRANSITIVE   -1         // illegal object number
#define SPECIALBASE    300        // base number of special rooms
#define GAMELIMIT      330        // base limit of turns
#define NOVICELIMIT    1000       // limit of turns for novice
#define WARNTIME       30         // late game starts at game.limit-this
#define FLASHTIME      50         // turns from first warning till blinding flash
#define PANICTIME      15         // time left after closing
#define BATTERYLIFE    2500       // turn limit increment from batteries
#define WORD_NOT_FOUND -1         // "Word not found" flag value for the vocab hash functions.
#define CARRIED        -1         // Player is toting it
#define READ_MODE      "rb"       // b is not needed for POSIX but harmless
#define WRITE_MODE     "wb"       // b is not needed for POSIX but harmless

/*
 *  MOD(N,M)    = Arithmetic modulus
 *  AT(OBJ)     = true if on either side of two-placed object
 *  CNDBIT(L,N) = true if COND(L) has bit n set (bit 0 is units bit)
 *  DARK(LOC)   = true if location "LOC" is dark
 *  FORCED(LOC) = true if LOC moves without asking for input (COND=2)
 *  FOREST(LOC) = true if LOC is part of the forest
 *  GSTONE(OBJ) = true if OBJ is a gemstone
 *  HERE(OBJ)   = true if the OBJ is at "LOC" (or is being carried)
 *  LIQUID()    = object number of liquid in bottle
 *  LIQLOC(LOC) = object number of liquid (if any) at LOC
 *  PCT(N)      = true N% of the time (N integer from 0 to 100)
 *  TOTING(OBJ) = true if the OBJ is being carried */
#define DESTROY(N)   move(N, LOC_NOWHERE)
#define MOD(N,M)     ((N) % (M))
#define TOTING(OBJ)  (game.place[OBJ] == CARRIED)
#define AT(OBJ)      (game.place[OBJ] == game.loc || game.fixed[OBJ] == game.loc)
#define HERE(OBJ)    (AT(OBJ) || TOTING(OBJ))
#define LIQ2(PBOTL)  ((1-(PBOTL))*WATER+((PBOTL)/2)*(WATER+OIL))
#define LIQUID()     (LIQ2(game.prop[BOTTLE]<0 ? -1-game.prop[BOTTLE] : game.prop[BOTTLE]))
#define LIQLOC(LOC)  (LIQ2((MOD(conditions[LOC]/2*2,8)-5)*MOD(conditions[LOC]/4,2)+1))
#define CNDBIT(L,N)  (tstbit(conditions[L],N))
#define FORCED(LOC)  CNDBIT(LOC, COND_FORCED)
#define DARK(DUMMY)  ((!tstbit(conditions[game.loc],COND_LIT)) && (game.prop[LAMP] == LAMP_DARK || !HERE(LAMP)))
#define PCT(N)       (randrange(100) < (N))
#define GSTONE(OBJ)  ((OBJ) == EMERALD || (OBJ) == RUBY || (OBJ) == AMBER || (OBJ) == SAPPH)
#define FOREST(LOC)  CNDBIT(LOC, COND_FOREST)
#define SPECIAL(LOC) ((LOC) > SPECIALBASE)
#define OUTSID(LOC)  (CNDBIT(LOC, COND_ABOVE) || FOREST(LOC))
#define INDEEP(LOC)  ((LOC) >= LOC_MISTHALL && !OUTSID(LOC))
#define BUG(x)       bug(x, #x)
#define MOTION_WORD(n)  ((n) + 0)
#define OBJECT_WORD(n)  ((n) + 1000)
#define ACTION_WORD(n)  ((n) + 2000)
#define SPECIAL_WORD(n) ((n) + 3000)
#define PROMOTE_WORD(n) ((n) + 1000)
#define DEMOTE_WORD(n)  ((n) - 1000)

enum bugtype {
    SPECIAL_TRAVEL_500_GT_L_GT_300_EXCEEDS_GOTO_LIST,
    VOCABULARY_TYPE_N_OVER_1000_NOT_BETWEEN_0_AND_3,
    INTRANSITIVE_ACTION_VERB_EXCEEDS_GOTO_LIST,
    TRANSITIVE_ACTION_VERB_EXCEEDS_GOTO_LIST,
    CONDITIONAL_TRAVEL_ENTRY_WITH_NO_ALTERATION,
    LOCATION_HAS_NO_TRAVEL_ENTRIES,
    HINT_NUMBER_EXCEEDS_GOTO_LIST,
    SPEECHPART_NOT_TRANSITIVE_OR_INTRANSITIVE_OR_UNKNOWN,
    ACTION_RETURNED_PHASE_CODE_BEYOND_END_OF_SWITCH,
};

enum speaktype {touch, look, hear, study, change};

enum termination {endgame, quitgame, scoregame};

enum speechpart {unknown, intransitive, transitive};

/* Phase codes for action returns.
 * These were at one time FORTRAN line numbers.
 * The values don't matter, but perturb their order at your peril.
 */
enum phase_codes {
    GO_TERMINATE,
    GO_MOVE,
    GO_TOP,
    GO_CLEAROBJ,
    GO_CHECKHINT,
    GO_CHECKFOO,
    GO_DIRECTION,
    GO_LOOKUP,
    GO_WORD2,
    GO_SPECIALS,
    GO_UNKNOWN,
    GO_ACTION,
    GO_DWARFWAKE,
};

typedef long token_t;  // word token - someday this will be char[TOKLEN+1]
typedef long vocab_t;  // index into a vocabulary array */

struct game_t {
    unsigned long lcg_a, lcg_c, lcg_m, lcg_x;
    long abbnum;                 // How often to print non-abbreviated descriptions
    long blklin;
    long bonus;
    long chloc;
    long chloc2;
    long clock1;                 // # turns from finding last treasure till closing
    long clock2;                 // # turns from first warning till blinding flash
    bool clshnt;                 // has player read the clue in the endgame?
    bool closed;                 // whether we're all the way closed
    bool closng;                 // whether it's closing time yet
    long conds;                  // min value for cond(loc) if loc has any hints
    long detail;

    /*  dflag controls the level of activation of dwarves:
     *	0	No dwarf stuff yet (wait until reaches Hall Of Mists)
     *	1	Reached Hall Of Mists, but hasn't met first dwarf
     *	2	Met first dwarf, others start moving, no knives thrown yet
     *	3	A knife has been thrown (first set always misses)
     *	3+	Dwarves are mad (increases their accuracy) */
    long dflag;

    long dkill;
    long dtotal;
    long foobar;                 // current progress in saying "FEE FIE FOE FOO".
    long holdng;                 // number of objects being carried
    long iwest;                  // How many times he's said "west" instead of "w"
    long knfloc;                 // 0 if no knife here, loc if knife , -1 after caveat
    long limit;                  // lifetime of lamp (not set here)
    bool lmwarn;                 // has player been warned about lamp going dim?
    long loc;
    long newloc;
    bool novice;                 // asked for instructions at start-up?
    long numdie;                 // number of times killed so far
    long oldloc;
    long oldlc2;
    long oldobj;
    bool panic;                  // has player found out he's trapped in the cave?
    long saved;                  // point penalty for saves
    long tally;
    long thresh;
    long trndex;
    long trnluz;                 // # points lost so far due to number of turns used
    long turns;                  // how many commands he's given (ignores yes/no)
    bool wzdark;                 // whether the loc he's leaving was dark
    char zzword[6];              // randomly generated magic word from bird
    bool blooded;                // has player drunk of dragon's blood?
    long abbrev[NLOCATIONS + 1];
    long atloc[NLOCATIONS + 1];
    long dseen[NDWARVES + 1];    // true if dwarf has seen him
    long dloc[NDWARVES + 1];     // location of dwarves, initially hard-wired in
    long odloc[NDWARVES + 1];    // prior loc of each dwarf, initially garbage
    long fixed[NOBJECTS + 1];
    long link[NOBJECTS * 2 + 1];
    long place[NOBJECTS + 1];
    long hinted[NHINTS];         // hintlc[i] is how long he's been at LOC with cond bit i
    long hintlc[NHINTS];         // hinted[i] is true iff hint i has been used.
    long prop[NOBJECTS + 1];
};

struct command_t {
    enum speechpart part;
    vocab_t verb;
    vocab_t obj;
    token_t wd1, wd1x;
    token_t wd2, wd2x;
};

extern struct game_t game;
extern FILE *logfp;
extern bool oldstyle, prompt;

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
extern void echo_input(FILE*, const char*, const char*);
extern int word_count(char*);
extern char* get_input(void);
extern bool silent_yes(void);
extern bool yes(const char*, const char*, const char*);
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
extern void make_zzword(char*);
extern void set_seed(long);
extern unsigned long get_next_lcg_value(void);
extern long randrange(long);
extern long score(enum termination);
extern void terminate(enum termination) __attribute__((noreturn));
extern int savefile(FILE *, long);
extern int suspend(void);
extern int resume(void);
extern int restore(FILE *);
extern long initialise(void);
extern int action(struct command_t *command);

/* Alas, declaring this static confuses the coverage analyzer */
void bug(enum bugtype, const char *) __attribute__((__noreturn__));

/* end */
