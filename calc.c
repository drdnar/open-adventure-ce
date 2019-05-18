
#include "dungeon.h"
#include "calc.h"

const char* get_compressed_string(int n)
{
    if (n == 0)
	return NULL;
    return compressed_strings[n];
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

const char* get_arbitrary_message(int n)
{
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
