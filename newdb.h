typedef struct {
  char* inventory;
  char** longs;
} object_description_s;

extern char* long_location_descriptions[];
extern char* short_location_descriptions[];
extern object_description_s object_descriptions[];
extern char* arbitrary_messages[];
extern char* class_messages[];
extern char* turn_threshold_messages[];

extern size_t CLSSES;
