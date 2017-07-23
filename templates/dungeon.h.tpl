#ifndef DUNGEON_H
#define DUNGEON_H

#include <stdio.h>
#include <stdbool.h>

#define SILENT	-1	/* no sound */

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

typedef struct {{
  const char** strs;
  const int n;
}} string_group_t;

typedef struct {{
  const string_group_t words;
  const char* inventory;
  int plac, fixd;
  bool is_treasure;
  const char** descriptions;
  const char** sounds;
  const char** texts;
  const char** changes;
}} object_t;

typedef struct {{
  const char* small;
  const char* big;
}} descriptions_t;

typedef struct {{
  descriptions_t description;
  const long sound;
  const bool loud;
}} location_t;

typedef struct {{
  const char* query;
  const char* yes_response;
}} obituary_t;

typedef struct {{
  const int threshold;
  const int point_loss;
  const char* message;
}} turn_threshold_t;

typedef struct {{
  const int threshold;
  const char* message;
}} class_t;

typedef struct {{
  const int number;
  const int turns;
  const int penalty;
  const char* question;
  const char* hint;
}} hint_t;

typedef struct {{
  const string_group_t words;
}} motion_t;

typedef struct {{
  const string_group_t words;
  const char* message;
  const bool noaction;
}} action_t;

enum condtype_t {{cond_goto, cond_pct, cond_carry, cond_with, cond_not}};
enum desttype_t {{dest_goto, dest_special, dest_speak}};

typedef struct {{
  const long motion;
  const long condtype;
  const long condarg1;
  const long condarg2;
  const enum desttype_t desttype;
  const long destval;
  const bool nodwarves;
  const bool stop;
}} travelop_t;

/* Abstract out the encoding of words in the travel array.  Gives us
 * some hope of getting to a less cryptic representation than we
 * inherited from FORTRAN, someday. To understand these, read the
 * encoding description for travel.
 */
#define T_TERMINATE(entry)	((entry).motion == 1)

extern const location_t locations[];
extern const object_t objects[];
extern const char* arbitrary_messages[];
extern const class_t classes[];
extern const turn_threshold_t turn_thresholds[];
extern const obituary_t obituaries[];
extern const hint_t hints[];
extern long conditions[];
extern const motion_t motions[];
extern const action_t actions[];
extern const travelop_t travel[];
extern const long tkey[];
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