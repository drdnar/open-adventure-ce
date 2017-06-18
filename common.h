#ifndef COMMON_H
#define COMMON_H
/* maximum size limits shared by dungeon compiler and runtime */

#define LOCSIZ		185
#define NOBJECTS	100
#define HNTSIZ		 20

extern const char advent_to_ascii[128];
extern const char ascii_to_advent[128];

enum bug_e {
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
   TOO_MANY_CLASS_OR_TURN_MESSAGES,                       // 11
   SPECIAL_TRAVEL_500_GT_L_GT_300_EXCEEDS_GOTO_LIST = 20, // 20
   RAN_OFF_END_OF_VOCABULARY_TABLE,                       // 21
   VOCABULARY_TYPE_N_OVER_1000_NOT_BETWEEN_0_AND_3,       // 22
   INTRANSITIVE_ACTION_VERB_EXCEEDS_GOTO_LIST,            // 23
   TRANSITIVE_ACTION_VERB_EXCEEDS_GOTO_LIST,              // 24
   CONDITIONAL_TRAVEL_ENTRY_WITH_NO_ALTERATION,           // 25
   LOCATION_HAS_NO_TRAVEL_ENTRIES,                        // 26
   HINT_NUMBER_EXCEEDS_GOTO_LIST,                         // 27
   INVALID_MOTH_RETURNED_BY_DATA_FUNCTION,                // 28
   TOO_MANY_PARAMETERS_GIVEN_TO_SETPRM,                   // 29
   SPEECHPART_NOT_TRANSITIVE_OR_INTRANSITIVE_OR_UNKNOWN=99, // 99
   ACTION_RETURNED_PHASE_CODE_BEYOND_END_OF_SWITCH,       // 100
};

static inline void bug(enum bug_e num, const char *error_string) __attribute__((__noreturn__));
static inline void bug(enum bug_e num, const char *error_string)
{
   fprintf(stderr, "Fatal error %d, %s.\n", num, error_string);
   exit(EXIT_FAILURE);
}

#define BUG(x) bug(x, #x)

#endif
