#!/usr/bin/python3

# This is the new open-adventure dungeon generator. It'll eventually replace the existing dungeon.c It currently outputs a .h and .c pair for C code.

import yaml

yaml_name = "adventure.yaml"
h_name = "newdb.h"
c_name = "newdb.c"

h_template = """#include <stdio.h>

typedef struct {{
  const char* inventory;
  const char** longs;
}} object_description_t;

typedef struct {{
  const char* small;
  const char* big;
}} descriptions_t;

typedef struct {{
  descriptions_t description;
}} location_t;

typedef struct {{
  const char* query;
  const char* yes_response;
}} obituary_t;

typedef struct {{
  const int threshold;
  const int point_loss;
  const char* message;
}} turn_threshold_t;

typedef struct {{
  const int threshold;
  const char* message;
}} class_t;

extern location_t locations[];
extern object_description_t object_descriptions[];
extern const char* arbitrary_messages[];
extern const class_t classes[];
extern turn_threshold_t turn_thresholds[];
extern obituary_t obituaries[];

extern size_t CLSSES;
extern int maximum_deaths;
extern int turn_threshold_count;

enum arbitrary_messages_refs {{
{}
}};

enum locations_refs {{
{}
}};

enum object_descriptions_refs {{
{}
}};
"""

c_template = """#include "{}"

const char* arbitrary_messages[] = {{
{}
}};

const class_t classes[] = {{
{}
}};

turn_threshold_t turn_thresholds[] = {{
{}
}};

location_t locations[] = {{
{}
}};

object_description_t object_descriptions[] = {{
{}
}};

obituary_t obituaries[] = {{
{}
}};

size_t CLSSES = {};
int maximum_deaths = {};
int turn_threshold_count = {};
"""

def make_c_string(string):
    """Render a Python string into C string literal format."""
    if string == None:
        return "NULL"
    string = string.replace("\n", "\\n")
    string = string.replace("\t", "\\t")
    string = string.replace('"', '\\"')
    string = string.replace("'", "\\'")
    string = '"' + string + '"'
    return string

def get_refs(l):
    reflist = [x[0] for x in l]
    ref_str = ""
    for ref in reflist:
        ref_str += "    {},\n".format(ref)
    ref_str = ref_str[:-1] # trim trailing newline
    return ref_str

def get_arbitrary_messages(arb):
    template = """    {},
"""
    arb_str = ""
    for item in arb:
        arb_str += template.format(make_c_string(item[1]))
    arb_str = arb_str[:-1] # trim trailing newline
    return arb_str

def get_class_messages(cls):
    template = """    {{
        .threshold = {},
        .message = {},
    }},
"""
    cls_str = ""
    for item in cls:
        threshold = item["threshold"]
        message = make_c_string(item["message"])
        cls_str += template.format(threshold, message)
    cls_str = cls_str[:-1] # trim trailing newline
    return cls_str

def get_turn_thresholds(trn):
    template = """    {{
        .threshold = {},
        .point_loss = {},
        .message = {},
    }},
"""
    trn_str = ""
    for item in trn:
        threshold = item["threshold"]
        point_loss = item["point_loss"]
        message = make_c_string(item["message"])
        trn_str += template.format(threshold, point_loss, message)
    trn_str = trn_str[:-1] # trim trailing newline
    return trn_str

def get_locations(loc):
    template = """    {{
        .description = {{
            .small = {},
            .big = {},
        }},
    }},
"""
    loc_str = ""
    for item in loc:
        short_d = make_c_string(item[1]["description"]["short"])
        long_d = make_c_string(item[1]["description"]["long"])
        loc_str += template.format(short_d, long_d)
    loc_str = loc_str[:-1] # trim trailing newline
    return loc_str

def get_object_descriptions(obj):
    template = """    {{
        .inventory = {},
        .longs = (const char* []) {{
{}
        }},
    }},
"""
    obj_str = ""
    for item in obj:
        i_msg = make_c_string(item[1]["inventory"])
        longs_str = ""
        if item[1]["longs"] == None:
            longs_str = " " * 12 + "NULL,"
        else:
            for l_msg in item[1]["longs"]:
                longs_str += " " * 12 + make_c_string(l_msg) + ",\n"
            longs_str = longs_str[:-1] # trim trailing newline
        obj_str += template.format(i_msg, longs_str)
    obj_str = obj_str[:-1] # trim trailing newline
    return obj_str

def get_obituaries(obit):
    template = """    {{
        .query = {},
        .yes_response = {},
    }},
"""
    obit_str = ""
    for o in obit:
        query = make_c_string(o["query"])
        yes = make_c_string(o["yes_response"])
        obit_str += template.format(query, yes)
    obit_str = obit_str[:-1] # trim trailing newline
    return obit_str

with open(yaml_name, "r") as f:
    db = yaml.load(f)

h = h_template.format(
    get_refs(db["arbitrary_messages"]),
    get_refs(db["locations"]),
    get_refs(db["object_descriptions"]),
)

c = c_template.format(
    h_name,
    get_arbitrary_messages(db["arbitrary_messages"]),
    get_class_messages(db["classes"]),
    get_turn_thresholds(db["turn_thresholds"]),
    get_locations(db["locations"]),
    get_object_descriptions(db["object_descriptions"]),
    get_obituaries(db["obituaries"]),
    len(db["classes"]),
    len(db["obituaries"]),
    len(db["turn_thresholds"]),
)

with open(h_name, "w") as hf:
    hf.write(h)

with open(c_name, "w") as cf:
    cf.write(c)
