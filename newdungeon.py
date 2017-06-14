#!/usr/bin/python3

# This is the new open-adventure dungeon generator. It'll eventually replace the existing dungeon.c It currently outputs a .h and .c pair for C code.

import json
import collections

json_name = "adventure.json"
h_name = "newdb.h"
c_name = "newdb.c"

def c_escape(string):
    """Add C escape sequences to a string."""
    string = string.replace("\n", "\\n")
    string = string.replace("\t", "\\t")
    string = string.replace('"', '\\"')
    string = string.replace("'", "\\'")
    return string

def write_regular_messages(name, h, c):

    h += "enum {}_refs {{\n".format(name)
    c += "char* {}[] = {{\n".format(name)
    
    index = 0
    for key, text in dungeon[name].items():
        h += "  {},\n".format(key)
        if text == None:
            c += "  NULL,\n"
        else:
            text = c_escape(text)
            c += "  \"{}\",\n".format(text)
        
        index += 1
        
    h += "};\n\n"
    c += "};\n\n"
    
    return (h, c)

with open(json_name, "r") as f:
    dungeon = json.load(f, object_pairs_hook = collections.OrderedDict)

h = """#include <stdio.h>

typedef struct {
  char* inventory;
  char** longs;
} object_description_t;

extern char* long_location_descriptions[];
extern char* short_location_descriptions[];
extern object_description_t object_descriptions[];
extern char* arbitrary_messages[];
extern char* class_messages[];
extern char* turn_threshold_messages[];

extern size_t CLSSES;

"""

c = """#include "{}"

""".format(h_name)

for name in [
        "arbitrary_messages",
        "long_location_descriptions",
        "short_location_descriptions",
        "class_messages",
        "turn_threshold_messages",
]:
    h, c = write_regular_messages(name, h, c)

h += "enum object_descriptions_refs {\n"
c += "object_description_t object_descriptions[] = {\n"
for key, data in dungeon["object_descriptions"].items():
    try:
        data["inventory"] = "\"{}\"".format(c_escape(data["inventory"]))
    except AttributeError:
        data["inventory"] = "NULL"
    h += "  {},\n".format(key)
    c += "  {\n"
    c += "    .inventory = {},\n".format(data["inventory"])
    try:
        data["longs"][0]
        c += "    .longs = (char* []) {\n"
        for l in data["longs"]:
            l = c_escape(l)
            c += "      \"{}\"\n".format(l)
        c += "    },\n"
    except (TypeError, IndexError):
        c += "    .longs = NULL,\n"
    c += "  },\n"
h += "};"
c += "};"

d = {
    h_name: h,
    c_name: c,
}
for filename, string in d.items():
    with open(filename, "w") as f:
        f.write(string)

