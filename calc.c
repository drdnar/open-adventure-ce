#include "dungeon.h"
#include "calc.h"
#include "advent.h"

extern uint8_t huffman_tree[];

#ifndef CALCULATOR
/* For non-calculator platforms, the data will not be moved after loading and
 * aligning.  Do not attempt to reload the data. */
bool have_data_file = 0;

/* These routiness allow parsing the data file without assuming anything about
 * alignment. */
char* data_file_parse_ptr = NULL;
char data_file_get_char()
{
    return *data_file_parse_ptr++;
}

uint8_t data_file_get_byte()
{
    return (uint8_t)(*data_file_parse_ptr++);
}

uint16_t data_file_get_uint16()
{
    return (uint16_t)(data_file_get_byte() | (data_file_get_byte() << 8));
}

uint32_t data_file_get_uint24()
{
    return (uint32_t)(data_file_get_byte()
        | (data_file_get_byte() << 8)
        | (data_file_get_byte() << 16)
    );
}

int32_t data_file_get_int32()
{
    return (int32_t)(data_file_get_byte()
        | (data_file_get_byte() << 8)
        | (data_file_get_byte() << 16)
        | (data_file_get_byte() << 24)
    );
}

#endif


/* Loads (or reloads) the data file.
 * Returns zero on success, non-zero on failure.
 */
uint8_t load_data_file()
{
#ifdef CALCULATOR
    /* For the eZ80, the data file will be mapped directly into the address
     * space.  The eZ80 fully supports unaligned accesses, so the getter
     * routines can just return a pointer to the raw data.  We'll look up the
     * data file address and then cache real pointers to our data sets. */
    return 1;
#else
    if (have_data_file)
        return 0;
    /* For other platforms, we will assume nothing about alignment requirements.
     * Therefore, we will have to "unwind" the dungeon file data structures into
     * fully-aligned dynamically allocated memory. */
     return 1;
#endif
}


#define HUFFMAN_BUFFERS 8
#define HUFFMAN_BUFFER_SIZE 2048
char buffers[HUFFMAN_BUFFERS][HUFFMAN_BUFFER_SIZE];
uint8_t next_buffer = 0;

#ifndef CALCULATOR
/* For the eZ80, there is an optimized decompression routine written in
 * assembly.  For other platforms, run the same algorithm written in C. */
/*void* huffman_tree;*/

char* decompress_string(void* input, char* output)
{
    uint8_t* next_input_byte = input;
    uint8_t current_byte = 0;
    uint8_t current_bit = 1;
    uint8_t* node;
    uint8_t symbol;
    do
    {
        node = &huffman_tree[0];
        while (!(*node & 0x80))
        {
            if (--current_bit == 0)
            {
                current_bit = 8;
                current_byte = *next_input_byte++;
            }
            if (current_byte & 1)
                node++;
            current_byte >>= 1;
            node += *node;
        }
        symbol = *node & 0x7F;
        *output++ = (char)symbol;
    } while (symbol);
    return output;
}
#else
/* For the eZ80, just reference the routines, which will be provided in
 * assembly. */
#include "dehuffman.h"
#endif


const char* dehuffman(void* data)
{
    char* buffer_start = &buffers[next_buffer++][0];
    /* Modulus is not fast on the eZ80 */
    if (next_buffer >= HUFFMAN_BUFFERS)
        next_buffer = 0;
    decompress_string(data, buffer_start);
    return buffer_start;
}


const char* get_compressed_string(int n)
{
    if (n == 0)
        return NULL;
    return dehuffman((void*)compressed_strings[n]);
}


const char* get_uncompressed_string(int n)
{
    if (n == 0)
        return NULL;
    return uncompressed_strings[n];
}


const char* get_object_description(int o, int n)
{
    const object_t* obj = get_object(o);
    if (obj->descriptions_start == obj->sounds_start)
        return NULL;
    return get_compressed_string(obj->strings[obj->descriptions_start + n]);
}


const char* get_object_sound(int o, int n)
{
    const object_t* obj = get_object(o);
    if (obj->sounds_start == obj->texts_start)
        return NULL;
    return get_compressed_string(obj->strings[obj->sounds_start + n]);
}


const char* get_object_text(int o, int n)
{
    const object_t* obj = get_object(o);
    if (obj->texts_start == obj->changes_start)
        return NULL;
    return get_compressed_string(obj->strings[obj->texts_start + n]);
}


const char* get_object_change(int o, int n)
{
    const object_t* obj = get_object(o);
    /* No need for explicit check, because this last one will have a reference
     * to string zero, i.e. NULL, if there are no change strings. */
    return get_compressed_string(obj->strings[obj->changes_start + n]);
}


const char* get_object_word(int o, int n)
{
    const object_t* obj = get_object(o);
    if (obj->descriptions_start == 0)
        return NULL;
    return get_uncompressed_string(obj->strings[n]);
}


bool have_PLEASE_ANSWER = 0;
#define PLEASE_ANSWER_LEN 32
char please_answer[PLEASE_ANSWER_LEN];
const char* get_arbitrary_message(int n)
{
    /* PLEASE_ANSWER gets used in a loop.  It would be better to delay
     * decompressing until speak/rspeak, but . . . I'm lazy. */
    if (n == PLEASE_ANSWER)
    {
        if (!have_PLEASE_ANSWER)
#ifndef CALCULATOR
            if (decompress_string((void*)compressed_strings[n], &please_answer[0]) - please_answer > PLEASE_ANSWER_LEN)
                BUG(PLEASE_ANSWER_TOO_LONG);
#else
            decompress_string((void*)compressed_strings[n], &please_answer[0]);
#endif
        return please_answer;
    }
    else
        return get_compressed_string(arbitrary_messages_[n]);
}


const class_t* get_class(int n)
{
    return &classes_[n];
}


const turn_threshold_t* get_turn_threshold(int n)
{
    return &turn_thresholds_[n];
}


const location_t* get_location(int n)
{
    return &locations_[n];
}


const object_t* get_object(int n)
{
    return &objects_[n];
}


const obituary_t* get_obituary(int n)
{
    return &obituaries_[n];
}


const hint_t* get_hint(int n)
{
    return &hints_[n];
}


/*const long get_condition(int n)
{
    return conditions_[n];
}*/


const motion_t* get_motion(int n)
{
    return &motions_[n];
}


const action_t* get_action(int n)
{
    return &actions_[n];
}


const long get_tkey(int n)
{
    return tkey_[n];
}


const travelop_t* get_travelop(int n)
{
    return &travel_[n];
}
