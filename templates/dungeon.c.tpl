#include "{h_file}"

uint8_t* dungeon;

/* For the calculator platform, the order of these variable declarations must
 * not be changed or you will break the optimized dungeon loading code. */

uint8_t* huffman_tree;

#ifndef CALCULATOR
uint8_t** compressed_strings;
#else
uint16_t* compressed_strings;
#endif

#ifndef CALCULATOR
char** uncompressed_strings;
#else
uint16_t* uncompressed_strings;
#endif

compressed_string_index_t* arbitrary_messages;

class_t* classes;

turn_threshold_t* turn_thresholds;

location_t* locations;

#ifndef CALCULATOR
object_t** objects;
#else
uint16_t* objects;
#endif

obituary_t* obituaries;

hint_t* hints;

#ifdef CALCULATOR
uint8_t* unused;
#endif

#ifndef CALCULATOR
motion_t** motions;
#else
uint16_t* motions;
#endif

#ifndef CALCULATOR
action_t** actions;
#else
uint16_t* actions;
#endif

uint16_t* tkey;

travelop_t* travel;

const char *ignore = "{ignore}";

int32_t conditions[] = {{
{conditions}
}};


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
#include "ez80.h"
#endif


const char* dehuffman(uint8_t* data)
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
#ifndef CALCULATOR
    return dehuffman(compressed_strings[n]);
#else
    return dehuffman(dungeon + compressed_strings[n]);
#endif
}}

const char* get_uncompressed_string(int n)
{{
    if (n == 0)
        return NULL;
#ifndef CALCULATOR
    return uncompressed_strings[n];
#else
    return (char*)(dungeon + uncompressed_strings[n]);
#endif
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
#ifndef CALCULATOR
    return objects[n];
#else
    return (object_t*)(dungeon + objects[n]);
#endif
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
#ifndef CALCULATOR
    return motions[n];
#else
    return (motion_t*)(dungeon + motions[n]);
#endif
}}

const action_t* get_action(int n)
{{
#ifndef CALCULATOR
    return actions[n];
#else
    return (action_t*)(dungeon + actions[n]);
#endif
}}

const uint16_t get_tkey(int n)
{{
    return tkey[n];
}}

const travelop_t* get_travelop(int n)
{{
    return &travel[n];
}}

/* end */
