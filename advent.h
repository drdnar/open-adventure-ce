#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#ifndef CALCULATOR
#include <inttypes.h>
#endif

#include "dungeon.h"

/* LCG PRNG parameters tested against
 * Knuth vol. 2. by the original authors */
#ifndef CALCULATOR
#define LCG_A 1093UL
#define LCG_C 221587UL
#define LCG_M 1048576UL
#define LCG_MASK 1048575UL
#define LCG_SHIFT 20
#else
#define LCG_A 1093
#define LCG_C 221587
#define LCG_M 0x100000
#define LCG_MASK 0xFFFFF
#define LCG_SHIFT 20
#endif

typedef int16_t vocab_t;  // index into a vocabulary array */
typedef int8_t verb_t;   // index into an actions array */
typedef int16_t obj_t;    // index into the object array */
typedef int16_t loc_t;    // index into the locations array */
typedef int16_t turn_t;   // turn counter or threshold */

extern bool at(obj_t obj);
extern bool here(obj_t obj);
extern int16_t liquid(void);
extern obj_t liqloc(loc_t loc);
extern bool dark(loc_t dummy);
extern bool gstone(obj_t obj);
extern bool outsid(loc_t loc);
extern bool inside(loc_t loc);
extern bool indeep(loc_t loc);

#define LINESIZE       1024
#define TOKLEN         5          // # sigificant characters in a token */
#define NDWARVES       6          // number of dwarves
#define PIRATE         NDWARVES   // must be NDWARVES-1 when zero-origin
#define DALTLC         LOC_NUGGET // alternate dwarf location
#define INVLIMIT       7          // inventory limit (# of objects)
#define INTRANSITIVE   (-1)         // illegal object number
#define GAMELIMIT      330        // base limit of turns
#define NOVICELIMIT    1000       // limit of turns for novice
#define WARNTIME       30         // late game starts at game.limit-this
#define FLASHTIME      50         // turns from first warning till blinding flash
#define PANICTIME      15         // time left after closing
#define BATTERYLIFE    2500       // turn limit increment from batteries
#define WORD_NOT_FOUND (-1)         // "Word not found" flag value for the vocab hash functions.
#define WORD_EMPTY     0          // "Word empty" flag value for the vocab hash functions
#define CARRIED        ((signed short)-1)         // Player is toting it
#define READ_MODE      "rb"       // b is not needed for POSIX but harmless
#define WRITE_MODE     "wb"       // b is not needed for POSIX but harmless

/* Special object-state values - integers > 0 are object-specific */
#define STATE_NOTFOUND  ((signed short)-1)	  // 'Not found" state of treasures */
#define STATE_FOUND	0	  // After discovered, before messed with
#define STATE_IN_CAVITY	1	  // State value common to all gemstones

/* Special fixed object-state values - integers > 0 are location */
#define IS_FIXED -1
#define IS_FREE 0

/* Map a state property value to a negative range, where the object cannot be
 * picked up but the value can be recovered later.  Avoid colliding with -1,
 * which has its own meaning. */
#define STASHED(obj)	(-1 - game.prop[obj])

/*
 *  DESTROY(N)  = Get rid of an item by putting it in LOC_NOWHERE
 *  MOD(N,M)    = Arithmetic modulus
 *  TOTING(OBJ) = true if the OBJ is being carried
 *  CNDBIT(L,N) = true if COND(L) has bit n set (bit 0 is units bit)
 *  FORCED(LOC) = true if LOC moves without asking for input (COND=2)
 *  PCT(N)      = true N% of the time (N integer from 0 to 100)
 *  FOREST(LOC) = true if LOC is part of the forest
 *  BUG(X)      = report bug and exit
 */
#define DESTROY(N)   move(N, LOC_NOWHERE)
#define MOD(N,M)     ((N) % (M))
#define TOTING(OBJ)  (game.place[OBJ] == CARRIED)
#define CNDBIT(L,N)  (tstbit(conditions[L],N))
#define FORCED(LOC)  (CNDBIT(LOC, COND_FORCED))
#define PCT(N)       (randrange(100) < (N))
#define FOREST(LOC)  (CNDBIT(LOC, COND_FOREST))
#define BUG(x)       bug(x, #x)

enum bugtype {
    SPECIAL_TRAVEL_500_GT_L_GT_300_EXCEEDS_GOTO_LIST,
    VOCABULARY_TYPE_N_OVER_1000_NOT_BETWEEN_0_AND_3,
    INTRANSITIVE_ACTION_VERB_EXCEEDS_GOTO_LIST,
    TRANSITIVE_ACTION_VERB_EXCEEDS_GOTO_LIST,
    CONDITIONAL_TRAVEL_ENTRY_WITH_NO_ALTERATION,
    LOCATION_HAS_NO_TRAVEL_ENTRIES,
    HINT_NUMBER_EXCEEDS_GOTO_LIST,
    SPEECHPART_NOT_TRANSITIVE_OR_INTRANSITIVE_OR_UNKNOWN,
    ACTION_RETURNED_PHASE_CODE_BEYOND_END_OF_SWITCH
};

#ifdef CALCULATOR
#define SAVE_FILE_HEADER "Colossal Cave Adventure save file"
extern char* save_file_header;
#endif

enum speaktype {touch, look, hear, study, change};

enum termination {endgame, quitgame, scoregame};

enum speechpart {unknown, intransitive, transitive};

typedef enum {NO_WORD_TYPE, MOTION, OBJECT, ACTION, NUMERIC} word_type_t;

typedef enum scorebonus {none, splatter, defeat, victory} score_t;

/* Phase codes for action returns.
 * These were at one time FORTRAN line numbers.
 * The values don't matter, but perturb their order at your peril.
 */
typedef enum {
    GO_TERMINATE,
    GO_MOVE,
    GO_TOP,
    GO_CLEAROBJ,
    GO_CHECKHINT,
    GO_WORD2,
    GO_UNKNOWN,
    GO_DWARFWAKE
} phase_codes_t;

struct game_t {
    unsigned int lcg_x;
    int16_t abbnum;                  // How often to print int descriptions
    score_t bonus;               // What kind of finishing bonus we are getting
    loc_t chloc;                 // pirate chest location
    loc_t chloc2;                // pirate chest alternate location
    turn_t clock1;               // # turns from finding last treasure to close
    turn_t clock2;               // # turns from warning till blinding flash
    bool clshnt;                 // has player read the clue in the endgame?
    bool closed;                 // whether we're all the way closed
    bool closng;                 // whether it's closing time yet
    bool lmwarn;                 // has player been warned about lamp going dim?
    bool novice;                 // asked for instructions at start-up?
    bool panic;                  // has player found out he's trapped?
    bool wzdark;                 // whether the loc he's leaving was dark
    bool blooded;                // has player drunk of dragon's blood?
    uint16_t conds;                   // min value for cond[loc] if loc has any hints
    int8_t detail;                  // level of detail in descriptions

    /*  dflag controls the level of activation of dwarves:
     *	0	No dwarf stuff yet (wait until reaches Hall Of Mists)
     *	1	Reached Hall Of Mists, but hasn't met first dwarf
     *	2	Met first dwarf, others start moving, no knives thrown yet
     *	3	A knife has been thrown (first set always misses)
     *	3+	Dwarves are mad (increases their accuracy) */
    uint16_t dflag;

    int8_t dkill;                   // dwarves killed
    int8_t dtotal;                  // total dwarves (including pirate) in loc
    int8_t foobar;                  // progress in saying "FEE FIE FOE FOO".
    int8_t holdng;                  // number of objects being carried
    uint16_t igo;                     // # uses of "go" instead of a direction
    uint16_t iwest;                   // # times he's said "west" instead of "w"
    int16_t knfloc;                  // knife location; 0 if none, -1 after caveat
    turn_t limit;                // lifetime of lamp
    loc_t loc;                   // where player is now
    loc_t newloc;                // where player is going
    turn_t numdie;               // number of times killed so far
    loc_t oldloc;                // where player was
    loc_t oldlc2;                // where player was two moves ago
    obj_t oldobj;                // last object player handled
    uint16_t saved;                   // point penalty for saves
    int8_t tally;                   // count of treasures gained
    uint16_t thresh;                  // current threshold for endgame scoring tier
    turn_t trndex;               // FIXME: not used, remove on next format bump
    turn_t trnluz;               // # points lost so far due to turns used
    turn_t turns;                // counts commands given (ignores yes/no)
    char zzword[TOKLEN + 1];     // randomly generated magic word from bird
    int16_t abbrev[NLOCATIONS + 1];  // has location been seen?
    int16_t atloc[NLOCATIONS + 1];   // head of object linked list per location
    int16_t dseen[NDWARVES + 1];     // true if dwarf has seen him
    loc_t dloc[NDWARVES + 1];    // location of dwarves, initially hard-wired in
    loc_t odloc[NDWARVES + 1];   // prior loc of each dwarf, initially garbage
    loc_t fixed[NOBJECTS + 1];   // fixed location of object (if  not IS_FREE)
    obj_t link[NOBJECTS * 2 + 1];// object-list links
    loc_t place[NOBJECTS + 1];   // location of object
    int16_t hinted[NHINTS];          // hinted[i] = true iff hint i has been used.
    int16_t  hintlc[NHINTS];          // hintlc[i] = how int at LOC with cond bit i
    int16_t prop[NOBJECTS + 1];      // object state array */
};

/*
 * Game application settings - settings, but not state of the game, per se.
 * This data is not saved in a saved game.
 */
struct settings_t {
    FILE *logfp;
    bool oldstyle;
    bool prompt;
};

typedef struct {
    char raw[LINESIZE];
    vocab_t id;
    word_type_t type;
} command_word_t;

typedef enum {EMPTY, RAW, TOKENIZED, GIVEN, PREPROCESSED, PROCESSING, EXECUTED} command_state_t;

typedef struct {
    enum speechpart part;
    command_word_t word[2];
    verb_t verb;
    obj_t obj;
    command_state_t state;
} command_t;

extern struct game_t game;
extern struct settings_t settings;
#ifdef CALCULATOR
extern char* save_file_name;
#endif

extern void play(void);
extern void load_dungeon(void);
extern bool get_command_input(command_t *);
extern void clear_command(command_t *);
extern void speak(const compressed_string_index_t, ...);
void sspeak(const int msg, ...);
extern void pspeak(vocab_t, enum speaktype, bool, int, ...);
extern void rspeak(vocab_t, ...);
extern void echo_input(FILE*, const char*, const char*);
extern bool silent_yes(void);
extern bool yes(const compressed_string_index_t, const compressed_string_index_t, const compressed_string_index_t);
extern void juggle(obj_t);
extern void move(obj_t, loc_t);
extern loc_t put(obj_t, loc_t, int);
extern void carry(obj_t, loc_t);
extern void drop(obj_t, loc_t);
extern int atdwrf(loc_t);
extern unsigned int setbit(unsigned int);
extern bool tstbit(unsigned int, unsigned int);
extern void set_seed(int32_t);
extern unsigned int randrange(unsigned int);
extern int score(enum termination);
#ifndef CALCULATOR
extern void terminate(enum termination) __attribute__((noreturn));
#else
extern void terminate(enum termination);
#endif
#ifdef CALCULATOR
extern void save_apd(void);
extern int set_save_file_name(void);
#endif
extern int savefile(FILE *, int32_t);
extern int suspend(void);
extern int resume(void);
extern int restore(FILE *);
extern int initialise(void);
extern phase_codes_t action(command_t*);
extern void state_change(obj_t, int);
extern bool is_valid(struct game_t);

#ifndef CALCULATOR
void bug(enum bugtype, const char *) __attribute__((__noreturn__));
#else
void bug(enum bugtype, const char *);
#endif

#ifndef CALCULATOR
/* represent an empty command word */
static const command_word_t empty_command_word = {
    .raw = "",
    .id = WORD_EMPTY,
    .type = NO_WORD_TYPE,
};
#else
extern command_word_t empty_command_word;
#endif

/* end */
