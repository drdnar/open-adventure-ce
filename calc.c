
#include "dungeon.h"
#include "calc.h"

const char* get_compressed_string(int n)
{
    return compressed_strings[n];
}


const char* get_uncompressed_string(int n)
{
    return uncompressed_strings[n];
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
