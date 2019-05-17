#ifndef CALC_H
#define CALC_H

const char* get_compressed_string(int n);

const char* get_uncompressed_string(int n);

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
