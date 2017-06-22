#ifndef COMMON_H
#define COMMON_H
/* maximum size limits shared by dungeon compiler and runtime */

#define LOCSIZ		185
#define NOBJECTS	100

extern const char advent_to_ascii[128];
extern const char ascii_to_advent[128];

/* Symbols for cond bits - used by advent.h */
#define COND_LIT	0	/* Light */
#define COND_OILY	1	/* If bit 2 is on: on for oil, off for water */
#define COND_FLUID	2	/* Liquid asset, see bit 1 */
#define COND_NOARRR	3	/* Pirate doesn't go here unless following */
#define COND_NOBACK	4	/* Cannot use "back" to move away */
#define COND_ABOVE	5
#define COND_DEEP	6	/* Deep - e.g where dwarves are active */
#define COND_FOREST	7	/* in the forest */
/* Bits past 10 indicate areas of interest to "hint" routines */
#define COND_HBASE	10	/* Base for location hint bits */
#define COND_HCAVE	11	/* Trying to get into cave */
#define COND_HBIRD	12	/* Trying to catch bird */
#define COND_HSNAKE	13	/* Trying to deal with snake */
#define COND_HMAZE	14	/* Lost in maze */
#define COND_HDARK	15	/* Pondering dark room */
#define COND_HWITT	16	/* At Witt's End */
#define COND_HCLIFF	17	/* Cliff with urn */
#define COND_HWOODS	18	/* Lost in forest */
#define COND_HOGRE	19	/* Trying to deal with ogre */
#define COND_HJADE	20	/* Found all treasures except jade */

enum bugtype {
   MESSAGE_LINE_GT_70_CHARACTERS,                         // 0
   NULL_LINE_IN_MESSAGE,                                  // 1
   TOO_MANY_WORDS_OF_MESSAGES,                            // 2
   TOO_MANY_TRAVEL_OPTIONS,                               // 3
   TOO_MANY_VOCABULARY_WORDS,                             // 4
   REQUIRED_VOCABULARY_WORD_NOT_FOUND,                    // 5
   TOO_MANY_RTEXT_MESSAGES,                               // 6
   TOO_MANY_HINTS,                                        // 7
   LOCATION_HAS_CONDITION_BIT_BEING_SET_TWICE,            // 8
   INVALID_SECTION_NUMBER_IN_DATABASE,                    // 9
   TOO_MANY_LOCATIONS,                                    // 10
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

#endif
