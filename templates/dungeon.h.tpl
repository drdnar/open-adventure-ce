#ifndef DUNGEON_H
#define DUNGEON_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#define SILENT	255	/* no sound */

/* Symbols for cond bits */
#define COND_LIT	0	/* Light */
#define COND_OILY	1	/* If bit 2 is on: on for oil, off for water */
#define COND_FLUID	2	/* Liquid asset, see bit 1 */
#define COND_NOARRR	3	/* Pirate doesn't go here unless following */
#define COND_NOBACK	4	/* Cannot use "back" to move away */
#define COND_ABOVE	5
#define COND_DEEP	6	/* Deep - e.g where dwarves are active */
#define COND_FOREST	7	/* In the forest */
#define COND_FORCED	8	/* Only one way in or out of here */
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

/* Memory is limited and signed operations are slower and larger on the eZ80,
 * so it helps to use the smallest unsigned data type possible. */

typedef uint16_t compressed_string_index_t;
typedef uint16_t uncompressed_string_index_t;

typedef struct {{
  uint8_t n;
  uncompressed_string_index_t strs[10];
}} string_group_t;

/* There is some code that depends on these fields being in the right order.
 * In particular, the former words field is an implied part of strings.
 * If descriptions_start is 0, then there are no words present. */
typedef struct {{
  compressed_string_index_t inventory;
  int16_t plac, fixd;
  uint8_t is_treasure;
  uint8_t descriptions_start;
  uint8_t sounds_start;
  uint8_t texts_start;
  uint8_t changes_start;
  /* This actually contains both compressed and uncompressed string indices. */
  compressed_string_index_t strings[11];
}} object_t;

typedef struct {{
  compressed_string_index_t small;
  compressed_string_index_t big;
}} descriptions_t;

typedef struct {{
  descriptions_t description;
  uint8_t sound;
  bool loud;
}} location_t;

typedef struct {{
  compressed_string_index_t query;
  compressed_string_index_t yes_response;
}} obituary_t;

typedef struct {{
  uint16_t threshold;
  uint8_t point_loss;
  compressed_string_index_t message;
}} turn_threshold_t;

typedef struct {{
  uint16_t threshold;
  compressed_string_index_t message;
}} class_t;

typedef struct {{
  uint8_t number;
  uint8_t turns;
  uint8_t penalty;
  compressed_string_index_t question;
  compressed_string_index_t hint;
}} hint_t;

typedef struct {{
  string_group_t words;
}} motion_t;

typedef struct {{
  compressed_string_index_t message;
  bool noaction;
  string_group_t words;
}} action_t;

enum condtype_t {{cond_goto = 0, cond_pct = 1, cond_carry = 2, cond_with = 3, cond_not = 4}};
enum desttype_t {{dest_goto = 0, dest_special = 1, dest_speak = 2}};

typedef struct {{
  uint8_t motion;
  uint8_t condtype;
  uint8_t condarg1;
  uint8_t condarg2;
#ifndef CALCULATOR
  enum desttype_t desttype;
#else
  uint8_t desttype;
#endif
  uint8_t destval;
  bool nodwarves;
  bool stop;
}} travelop_t;

/* Abstract out the encoding of words in the travel array.  Gives us
 * some hope of getting to a less cryptic representation than we
 * inherited from FORTRAN, someday. To understand these, read the
 * encoding description for travel.
 */
#define T_TERMINATE(entry)	((entry)->motion == 1)

#define DATA_FILE_ID_STRING "Colossal Cave Adventure dungeon"

typedef struct {{
    /* "Colossal Cave Adventure dungeon" */
    char header[32];
    uint16_t huffman_table;
    uint16_t compressed_strings;
    uint16_t uncompressed_strings;
    uint16_t arbitrary_messages;
    uint16_t classes;
    uint16_t turn_thresholds;
    uint16_t locations;
    uint16_t objects;
    uint16_t obituaries;
    uint16_t hints;
    /* Possibly may be used for the conditions array in the future */
    uint16_t unused;
    uint16_t motions;
    uint16_t actions;
    uint16_t tkey;
    uint16_t travel;
}} data_file_header_t;

const char* get_compressed_string(int n);
const char* get_uncompressed_string(int n);
const char* get_object_description(int o, int n);
const char* get_object_sound(int o, int n);
const char* get_object_text(int o, int n);
const char* get_object_change(int o, int n);
const char* get_object_word(int o, int n);
const compressed_string_index_t get_arbitrary_message_index(int n);
const char* get_arbitrary_message(int n);
const class_t* get_class(int n);
const turn_threshold_t* get_turn_threshold(int n);
const location_t* get_location(int n);
const object_t* get_object(int n);
const obituary_t* get_obituary(int n);
const hint_t* get_hint(int n);
/*const long get_condition(int n);*/
const motion_t* get_motion(int n);
const action_t* get_action(int n);
const uint16_t get_tkey(int n);
const travelop_t* get_travelop(int n);

extern uint8_t* huffman_tree;
extern uint8_t** compressed_strings;
extern char** uncompressed_strings;
extern location_t* locations;
extern object_t** objects;
extern compressed_string_index_t* arbitrary_messages;
extern class_t* classes;
extern turn_threshold_t* turn_thresholds;
extern obituary_t* obituaries;
extern hint_t* hints;
extern int32_t conditions[];
extern motion_t** motions;
extern action_t** actions;
extern travelop_t* travel;
extern uint16_t* tkey;
extern const char *ignore;

#define NLOCATIONS	{num_locations}
#define NOBJECTS	{num_objects}
#define NHINTS		{num_hints}
#define NCLASSES	{num_classes}
#define NDEATHS		{num_deaths}
#define NTHRESHOLDS	{num_thresholds}
#define NMOTIONS    {num_motions}
#define NACTIONS  	{num_actions}
#define NTRAVEL		{num_travel}
#define NKEYS		{num_keys}
#define NARBMSGS	{num_arb_msgs}
#define NCOMPSTRS	{num_comp_strs}
#define NUNCOMPSTRS	{num_uncomp_strs}

#define BIRD_ENDSTATE {bird_endstate}

enum arbitrary_messages_refs {{
{arbitrary_messages}
}};

enum locations_refs {{
{locations}
}};

enum object_refs {{
{objects}
}};

enum motion_refs {{
{motions}
}};

enum action_refs {{
{actions}
}};

/* State definitions */

{state_definitions}

#endif /* end DUNGEON_H */
