#!/usr/bin/python3

# This is the new open-adventure dungeon generator. It'll eventually replace the existing dungeon.c It currently outputs a .h and .c pair for C code.

import yaml
import sys

yaml_name = "adventure.yaml"
h_name = "newdb.h"
c_name = "newdb.c"

def c_escape(string):
    """Add C escape sequences to a string."""
    string = string.replace("\n", "\\n")
    string = string.replace("\t", "\\t")
    string = string.replace('"', '\\"')
    string = string.replace("'", "\\'")
    return string

def quotewrap(string):
    """Wrap a string in double quotes."""
    return '"' + string + '"'

def write_regular_messages(name, h, c):

    h += "enum {}_refs {{\n".format(name)
    for key, text in dungeon[name]:
        h += "  {},\n".format(key)
    h += "};\n\n"
    
    c += "char* {}[] = {{\n".format(name)   
    index = 0
    for key, text in dungeon[name]:
        if text == None:
            c += "  NULL,\n"
        else:
            text = c_escape(text)
            c += "  \"{}\",\n".format(text)
        index += 1
    c += "};\n\n"
    
    return (h, c)

with open(yaml_name, "r") as f:
    dungeon = yaml.load(f)

h = """#include <stdio.h>

typedef struct {
  char* inventory;
  char** longs;
} object_description_t;

typedef struct {
  char* small;
  char* big;
} descriptions_t;

typedef struct {
  descriptions_t description;
} location_t;

extern location_t locations[];
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
        "class_messages",
        "turn_threshold_messages",
]:
    h, c = write_regular_messages(name, h, c)

h += "enum locations_refs {\n"
c += "location_t locations[] = {\n"
for key, data in dungeon["locations"]:
    h += "  {},\n".format(key)

    try:
        short = quotewrap(c_escape(data["description"]["short"]))
    except AttributeError:
        short = "NULL"
    try:
        long = quotewrap(c_escape(data["description"]["long"]))
    except AttributeError:
        long = "NULL"

    c += """  {{
    .description = {{
      .small = {},
      .big = {},
    }},
  }},
""".format(short, long)

c += "};\n\n"
h += "};\n\n"

h += "enum object_descriptions_refs {\n"
c += "object_description_t object_descriptions[] = {\n"
for key, data in dungeon["object_descriptions"]:
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
            c += "      \"{}\",\n".format(l)
        c += "    },\n"
    except (TypeError, IndexError):
        c += "    .longs = NULL,\n"
    c += "  },\n"
h += "};"
c += "};"

c += """
size_t CLSSES = {};
""".format(len(dungeon["class_messages"]))

# finally, write out the files
d = {
    h_name: h,
    c_name: c,
}
for filename, string in d.items():
    with open(filename, "w") as f:
        f.write(string)

