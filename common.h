#ifndef COMMON_H
#define COMMON_H
/* maximum size limits shared by dungeon compiler and runtime */

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

#endif
