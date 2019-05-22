#ifndef CALC_H
#define CALC_H
/*#include "dehuffman.h"*/

#define DATA_FILE_ID_STRING "Colossal Cave Adventure dungeon"

typedef struct {
    /* "Colossal Cave Adventure dungeon" */
    char header[41];
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
} data_file_header_t;

uint8_t load_data_file();

const char* get_compressed_string(int n);

const char* get_uncompressed_string(int n);

const char* get_object_description(int o, int n);

const char* get_object_sound(int o, int n);

const char* get_object_text(int o, int n);

const char* get_object_change(int o, int n);

const char* get_object_word(int o, int n);

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

const long get_tkey(int n);

const travelop_t* get_travelop(int n);

#endif
