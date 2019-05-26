#include "{h_file}"

uint8_t* huffman_tree;

uint8_t** compressed_strings;

char** uncompressed_strings;

compressed_string_index_t* arbitrary_messages;

class_t* classes;

turn_threshold_t* turn_thresholds;

location_t* locations;

object_t** objects;

obituary_t* obituaries;

hint_t* hints;

int32_t conditions[] = {{
{conditions}
}};

motion_t** motions;

action_t** actions;

uint16_t* tkey;

travelop_t* travel;

const char *ignore = "{ignore}";


#define HUFFMAN_BUFFERS 4
#define HUFFMAN_BUFFER_SIZE 2048
char buffers[HUFFMAN_BUFFERS][HUFFMAN_BUFFER_SIZE];
uint8_t next_buffer = 0;

#ifndef CALCULATOR
/* For the eZ80, there is an optimized decompression routine written in
 * assembly.  For other platforms, run the same algorithm written in C. */
/*void* huffman_tree;*/
char* decompress_string(void* input, char* output)
{{
    /* A neat property of this routine is that the serialized Huffman tree fits
     * in just a handful of cache lines. Too bad it doesn't really matter. */
    uint8_t* next_input_byte = input;
    uint8_t current_byte = 0;
    uint8_t current_bit = 1;
    uint8_t* node;
    uint8_t symbol;
    do
    {{
        node = &huffman_tree[0];
        while (!(*node & 0x80))
        {{
            if (--current_bit == 0)
            {{
                current_bit = 8;
                current_byte = *next_input_byte++;
            }}
            if (current_byte & 1)
                node++;
            current_byte >>= 1;
            node += *node;
        }}
        symbol = *node & 0x7F;
        *output++ = (char)symbol;
    }} while (symbol);
    return output;
}}
#else
/* For the eZ80, just reference the routines, which will be provided in
 * assembly. */
#include "dehuffman.h"
#endif


const char* dehuffman(void* data)
{{
    char* buffer_start = &buffers[next_buffer++][0];
    /* Modulus is not fast on the eZ80 */
    if (next_buffer >= HUFFMAN_BUFFERS)
        next_buffer = 0;
    decompress_string(data, buffer_start);
    return buffer_start;
}}

const char* get_compressed_string(int n)
{{
    if (n == 0)
        return NULL;
    return dehuffman((void*)compressed_strings[n]);
}}

const char* get_uncompressed_string(int n)
{{
    if (n == 0)
        return NULL;
    return uncompressed_strings[n];
}}

const char* get_object_description(int o, int n)
{{
    const object_t* obj = get_object(o);
    if (obj->descriptions_start == obj->sounds_start)
        return NULL;
    return get_compressed_string(obj->strings[obj->descriptions_start + n]);
}}

const char* get_object_sound(int o, int n)
{{
    const object_t* obj = get_object(o);
    if (obj->sounds_start == obj->texts_start)
        return NULL;
    return get_compressed_string(obj->strings[obj->sounds_start + n]);
}}

const char* get_object_text(int o, int n)
{{
    const object_t* obj = get_object(o);
    if (obj->texts_start == obj->changes_start)
        return NULL;
    return get_compressed_string(obj->strings[obj->texts_start + n]);
}}

const char* get_object_change(int o, int n)
{{
    const object_t* obj = get_object(o);
    /* No need for explicit check, because this last one will have a reference
     * to string zero, i.e. NULL, if there are no change strings. */
    return get_compressed_string(obj->strings[obj->changes_start + n]);
}}

const char* get_object_word(int o, int n)
{{
    const object_t* obj = get_object(o);
    if (obj->descriptions_start == 0)
        return NULL;
    return get_uncompressed_string(obj->strings[n]);
}}

const char* get_arbitrary_message(int n)
{{
    return get_compressed_string(arbitrary_messages[n]);
}}

const compressed_string_index_t get_arbitrary_message_index(int n)
{{
    return arbitrary_messages[n];
}}

const class_t* get_class(int n)
{{
    return &classes[n];
}}

const turn_threshold_t* get_turn_threshold(int n)
{{
    return &turn_thresholds[n];
}}

const location_t* get_location(int n)
{{
    return &locations[n];
}}

const object_t* get_object(int n)
{{
    return objects[n];
}}

const obituary_t* get_obituary(int n)
{{
    return &obituaries[n];
}}

const hint_t* get_hint(int n)
{{
    return &hints[n];
}}

/*const long get_condition(int n)
{{
    return conditions_[n];
}}*/

const motion_t* get_motion(int n)
{{
    return motions[n];
}}

const action_t* get_action(int n)
{{
    return actions[n];
}}

const long get_tkey(int n)
{{
    return tkey[n];
}}

const travelop_t* get_travelop(int n)
{{
    return &travel[n];
}}

/* end */
