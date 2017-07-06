#!/usr/bin/python3

# This is the new open-adventure dungeon generator. It'll eventually
# replace the existing dungeon.c It currently outputs a .h and .c pair
# for C code.
#
# The nontrivial part of this is the compilation of the YAML for
# movement rules to the travel array that's actually used by
# playermove().  This program first compiles the YAML to a form
# identical to the data in section 3 of the old adventure.text file,
# then a second stage unpacks that data into the travel array.
#
# Here are the rules of the intermediate form:
#
# Each row of data contains a location number (X), a second
# location number (Y), and a list of motion numbers (see section 4).
# each motion represents a verb which will go to Y if currently at X.
# Y, in turn, is interpreted as follows.  Let M=Y/1000, N=Y mod 1000.
#		If N<=300	it is the location to go to.
#		If 300<N<=500	N-300 is used in a computed goto to
#					a section of special code.
#		If N>500	message N-500 from section 6 is printed,
#					and he stays wherever he is.
# Meanwhile, M specifies the conditions on the motion.
#		If M=0		it's unconditional.
#		If 0<M<100	it is done with M% probability.
#		If M=100	unconditional, but forbidden to dwarves.
#		If 100<M<=200	he must be carrying object M-100.
#		If 200<M<=300	must be carrying or in same room as M-200.
#		If 300<M<=400	game.prop(M % 100) must *not* be 0.
#		If 400<M<=500	game.prop(M % 100) must *not* be 1.
#		If 500<M<=600	game.prop(M % 100) must *not* be 2, etc.
# If the condition (if any) is not met, then the next *different*
# "destination" value is used (unless it fails to meet *its* conditions,
# in which case the next is found, etc.).  Typically, the next dest will
# be for one of the same verbs, so that its only use is as the alternate
# destination for those verbs.  For instance:
#		15	110022	29	31	34	35	23	43
#		15	14	29
# This says that, from loc 15, any of the verbs 29, 31, etc., will take
# him to 22 if he's carrying object 10, and otherwise will go to 14.
#		11	303008	49
#		11	9	50
# This says that, from 11, 49 takes him to 8 unless game.prop(3)=0, in which
# case he goes to 9.  Verb 50 takes him to 9 regardless of game.prop(3).

import sys, yaml

yaml_name = "adventure.yaml"
h_name = "dungeon.h"
c_name = "dungeon.c"

statedefines = ""

h_template = """/* Generated from adventure.yaml - do not hand-hack! */
#ifndef DUNGEON_H
#define DUNGEON_H

#include <stdio.h>
#include <stdbool.h>

#define SILENT	-1	/* no sound */

/* Symbols for cond bits */
#define COND_LIT	0	/* Light */
#define COND_OILY	1	/* If bit 2 is on: on for oil, off for water */
#define COND_FLUID	2	/* Liquid asset, see bit 1 */
#define COND_NOARRR	3	/* Pirate doesn't go here unless following */
#define COND_NOBACK	4	/* Cannot use "back" to move away */
#define COND_ABOVE	5
#define COND_DEEP	6	/* Deep - e.g where dwarves are active */
#define COND_FOREST	7	/* In the forest */
#define COND_FORCED	8	/* Only one way in or out of here */
/* Bits past 10 indicate areas of interest to "hint" routines */
#define COND_HBASE	10	/* Base for location hint bits */
#define COND_HCAVE	11	/* Trying to get into cave */
#define COND_HBIRD	12	/* Trying to catch bird */
#define COND_HSNAKE	13	/* Trying to deal with snake */
#define COND_HMAZE	14	/* Lost in maze */
#define COND_HDARK	15	/* Pondering dark room */
#define COND_HWITT	16	/* At Witt's End */
#define COND_HCLIFF	17	/* Cliff with urn */
#define COND_HWOODS	18	/* Lost in forest */
#define COND_HOGRE	19	/* Trying to deal with ogre */
#define COND_HJADE	20	/* Found all treasures except jade */

typedef struct {{
  const char** strs;
  const int n;
}} string_group_t;

typedef struct {{
  const string_group_t words;
  const char* inventory;
  int plac, fixd;
  bool is_treasure;
  const char** descriptions;
  const char** sounds;
  const char** texts;
  const char** changes;
}} object_t;

typedef struct {{
  const char* small;
  const char* big;
}} descriptions_t;

typedef struct {{
  descriptions_t description;
  const long sound;
  const bool loud;
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

typedef struct {{
  const int number;
  const int turns;
  const int penalty;
  const char* question;
  const char* hint;
}} hint_t;

typedef struct {{
  const string_group_t words;
}} motion_t;

typedef struct {{
  const string_group_t words;
  const char* message;
}} action_t;

typedef struct {{
  const string_group_t words;
  const char* message;
}} special_t;

typedef struct {{
  const long motion;
  const long cond;
  const long dest;
  const bool nodwarves;
  const bool stop;
}} travelop_t;

/* Abstract out the encoding of words in the travel array.  Gives us
 * some hope of getting to a less cryptic representation than we
 * inherited from FORTRAN, someday. To understand these, read the
 * encoding description for travel.
 */
#define T_TERMINATE(entry)	((entry).motion == 1)
#define L_SPEAK(loc)		((loc) - 500)

extern const location_t locations[];
extern const object_t objects[];
extern const char* arbitrary_messages[];
extern const class_t classes[];
extern const turn_threshold_t turn_thresholds[];
extern const obituary_t obituaries[];
extern const hint_t hints[];
extern long conditions[];
extern const motion_t motions[];
extern const action_t actions[];
extern const special_t specials[];
extern const travelop_t travel[];
extern const long tkey[];
extern const char *ignore;

#define NLOCATIONS	{}
#define NOBJECTS	{}
#define NHINTS		{}
#define NCLASSES	{}
#define NDEATHS		{}
#define NTHRESHOLDS	{}
#define NMOTIONS        {}
#define NACTIONS  	{}
#define NSPECIALS       {}
#define NTRAVEL		{}
#define NKEYS		{}

#define BIRD_ENDSTATE	{}

enum arbitrary_messages_refs {{
{}
}};

enum locations_refs {{
{}
}};

enum object_refs {{
{}
}};

enum motion_refs {{
{}
}};

enum action_refs {{
{}
}};

enum special_refs {{
{}
}};

/* State definitions */

{}
#endif /* end DUNGEON_H */
"""

c_template = """/* Generated from adventure.yaml - do not hand-hack! */

#include "{}"

const char* arbitrary_messages[] = {{
{}
}};

const class_t classes[] = {{
{}
}};

const turn_threshold_t turn_thresholds[] = {{
{}
}};

const location_t locations[] = {{
{}
}};

const object_t objects[] = {{
{}
}};

const obituary_t obituaries[] = {{
{}
}};

const hint_t hints[] = {{
{}
}};

long conditions[] = {{
{}
}};

const motion_t motions[] = {{
{}
}};

const action_t actions[] = {{
{}
}};

const special_t specials[] = {{
{}
}};

const long tkey[] = {{{}}};

const travelop_t travel[] = {{
{}
}};

const char *ignore = \"{}\";

/* end */
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

def get_string_group(strings):
    template = """{{
            .strs = {},
            .n = {},
        }}"""
    if strings == []:
        strs = "NULL"
    else:
        strs = "(const char* []) {" + ", ".join([make_c_string(s) for s in strings]) + "}"
    n = len(strings)
    sg_str = template.format(strs, n)
    return sg_str

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
    template = """    {{ // {}
        .description = {{
            .small = {},
            .big = {},
        }},
        .sound = {},
        .loud = {},
    }},
"""
    loc_str = ""
    for (i, item) in enumerate(loc):
        short_d = make_c_string(item[1]["description"]["short"])
        long_d = make_c_string(item[1]["description"]["long"])
        sound = item[1].get("sound", "SILENT")
        loud = "true" if item[1].get("loud") else "false"
        loc_str += template.format(i, short_d, long_d, sound, loud)
    loc_str = loc_str[:-1] # trim trailing newline
    return loc_str

def get_objects(obj):
    template = """    {{ // {}
        .words = {},
        .inventory = {},
        .plac = {},
        .fixd = {},
        .is_treasure = {},
        .descriptions = (const char* []) {{
{}
        }},
        .sounds = (const char* []) {{
{}
        }},
        .texts = (const char* []) {{
{}
        }},
        .changes = (const char* []) {{
{}
        }},
    }},
"""
    obj_str = ""
    for (i, item) in enumerate(obj):
        attr = item[1]
        try:
            words_str = get_string_group(attr["words"])
        except KeyError:
            words_str = get_string_group([])
        i_msg = make_c_string(attr["inventory"])
        descriptions_str = ""
        if attr["descriptions"] == None:
            descriptions_str = " " * 12 + "NULL,"
        else:
            labels = []
            for l_msg in attr["descriptions"]:
                descriptions_str += " " * 12 + make_c_string(l_msg) + ",\n"
            for label in attr.get("states", []):
                labels.append(label)
            descriptions_str = descriptions_str[:-1] # trim trailing newline
            if labels:
                global statedefines
                statedefines += "/* States for %s */\n" % item[0]
                for (i, label) in enumerate(labels):
                    statedefines += "#define %s\t%d\n" % (label, i)
                statedefines += "\n"
        sounds_str = ""
        if attr.get("sounds") == None:
            sounds_str = " " * 12 + "NULL,"
        else:
             for l_msg in attr["sounds"]:
                 sounds_str += " " * 12 + make_c_string(l_msg) + ",\n"
             sounds_str = sounds_str[:-1] # trim trailing newline
        texts_str = ""
        if attr.get("texts") == None:
            texts_str = " " * 12 + "NULL,"
        else:
             for l_msg in attr["texts"]:
                 texts_str += " " * 12 + make_c_string(l_msg) + ",\n"
             texts_str = texts_str[:-1] # trim trailing newline
        changes_str = ""
        if attr.get("changes") == None:
            changes_str = " " * 12 + "NULL,"
        else:
             for l_msg in attr["changes"]:
                 changes_str += " " * 12 + make_c_string(l_msg) + ",\n"
             changes_str = changes_str[:-1] # trim trailing newline
        locs = attr.get("locations", ["LOC_NOWHERE", "LOC_NOWHERE"])
        immovable = attr.get("immovable", False)
        try:
            if type(locs) == str:
                locs = [locs, -1 if immovable else 0]
        except IndexError:
            sys.stderr.write("dungeon: unknown object location in %s\n" % locs)
            sys.exit(1)
        treasure = "true" if attr.get("treasure") else "false"
        obj_str += template.format(i, words_str, i_msg, locs[0], locs[1], treasure, descriptions_str, sounds_str, texts_str, changes_str)
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

def get_hints(hnt, arb):
    template = """    {{
        .number = {},
        .penalty = {},
        .turns = {},
        .question = {},
        .hint = {},
    }},
"""
    hnt_str = ""
    md = dict(arb)
    for member in hnt:
        item = member["hint"]
        number = item["number"]
        penalty = item["penalty"]
        turns = item["turns"]
        question = make_c_string(item["question"])
        hint = make_c_string(item["hint"])
        hnt_str += template.format(number, penalty, turns, question, hint)
    hnt_str = hnt_str[:-1] # trim trailing newline
    return hnt_str

def get_condbits(locations):
    cnd_str = ""
    for (name, loc) in locations:
        conditions = loc["conditions"]
        hints = loc.get("hints") or []
        flaglist = []
        for flag in conditions:
            if conditions[flag]:
                flaglist.append(flag)
        line = "|".join([("(1<<COND_%s)" % f) for f in flaglist])
        trail = "|".join([("(1<<COND_H%s)" % f['name']) for f in hints])
        if trail:
            line += "|" + trail
        if line.startswith("|"):
            line = line[1:]
        if not line:
            line = "0"
        cnd_str += "    " + line + ",\t// " + name + "\n"
    return cnd_str

def get_motions(motions):
    template = """    {{
        .words = {},
    }},
"""
    mot_str = ""
    for motion in motions:
        contents = motion[1]
        if contents["words"] == None:
            words_str = get_string_group([])
        else:
            words_str = get_string_group(contents["words"])
        mot_str += template.format(words_str)
        global ignore
        if contents.get("oldstyle", True) == False:
            for word in contents["words"]:
                if len(word) == 1:
                    ignore += word.upper()
    return mot_str

def get_actions(actions):
    template = """    {{
        .words = {},
        .message = {},
    }},
"""
    act_str = ""
    for action in actions:
        contents = action[1]
        
        if contents["words"] == None:
            words_str = get_string_group([])
        else:
            words_str = get_string_group(contents["words"])

        if contents["message"] == None:
            message = "NO_MESSAGE"
        else:
            message = contents["message"]
            
        act_str += template.format(words_str, message)
        global ignore
        if contents.get("oldstyle", True) == False:
            for word in contents["words"]:
                if len(word) == 1:
                    ignore += word.upper()
    act_str = act_str[:-1] # trim trailing newline
    return act_str

def get_specials(specials):
    template = """    {{
        .words = {},
        .message = {},
    }},
"""
    spc_str = ""
    for special in specials:
        contents = special[1]

        if contents["words"] == None:
            words_str = get_string_group([])
        else:
            words_str = get_string_group(contents["words"])

        if contents["message"] == None:
            message = "NULL"
        else:
            message = make_c_string(contents["message"])

        spc_str += template.format(words_str, message)
        global ignore
        if contents.get("oldstyle", True) == False:
            for word in contents["words"]:
                if len(word) == 1:
                    ignore += word.upper()
    spc_str = spc_str[:-1] # trim trailing newline
    return spc_str

def bigdump(arr):
    out = ""
    for (i, entry) in enumerate(arr):
        if i % 10 == 0:
            if out and out[-1] == ' ':
                out = out[:-1]
            out += "\n    "
        out += str(arr[i]).lower() + ", "
    out = out[:-2] + "\n"
    return out

def buildtravel(locs, objs):
    ltravel = []
    verbmap = {}
    for i, motion in enumerate(db["motions"]):
        try:
            for word in motion[1]["words"]:
                verbmap[word.upper()] = i
        except TypeError:
            pass
    def dencode(action, name):
        "Decode a destination number"
        if action[0] == "goto":
            try:
                return locnames.index(action[1])
            except ValueError:
                sys.stderr.write("dungeon: unknown location %s in goto clause of %s\n" % (cond[1], name))
        elif action[0] == "special":
            return 300 + action[1]
        elif action[0] == "speak":
            try:
                return 500 + msgnames.index(action[1])
            except ValueError:
                sys.stderr.write("dungeon: unknown location %s in carry clause of %s\n" % (cond[1], name))
        else:
            print(cond)
            raise ValueError
    def cencode(cond, name):
        if cond is None:
            return 0
        elif cond == ["nodwarves"]:
            return 100
        elif cond[0] == "pct":
            return cond[1]
        elif cond[0] == "carry":
            try:
                return 100 + objnames.index(cond[1])
            except ValueError:
                sys.stderr.write("dungeon: unknown object name %s in carry clause of %s\n" % (cond[1], name))
                sys.exit(1)
        elif cond[0] == "with":
            try:
                return 200 + objnames.index(cond[1])
            except IndexError:
                sys.stderr.write("dungeon: unknown object name %s in with clause of \n" % (cond[1], name))
                sys.exit(1)
        elif cond[0] == "not":
            try:
                obj = objnames.index(cond[1])
                if type(cond[2]) == int:
                    state = cond[2]
                elif cond[2] in objs[obj][1].get("states", []):
                    state = objs[obj][1].get("states").index(cond[2])
                else:
                    for (i, stateclause) in enumerate(objs[obj][1]["descriptions"]):
                        if type(stateclause) == list:
                            if stateclause[0] == cond[2]:
                                state = i
                                break
                    else:
                        sys.stderr.write("dungeon: unmatched state symbol %s in not clause of %s\n" % (cond[2], name))
                        sys.exit(0);
                return 300 + obj + 100 * state
            except ValueError:
                sys.stderr.write("dungeon: unknown object name %s in not clause of %s\n" % (cond[1], name))
                sys.exit(1)
        else:
            print(cond)
            raise ValueError

    for (i, (name, loc)) in enumerate(locs):
        if "travel" in loc:
            for rule in loc["travel"]:
                tt = [i]
                dest = dencode(rule["action"], name) + 1000 * cencode(rule.get("cond"), name)
                tt.append(dest)
                tt += [verbmap[e] for e in rule["verbs"]]
                if not rule["verbs"]:
                    tt.append(1)
                ltravel.append(tuple(tt))

    # At this point the ltravel data is in the Section 3
    # representation from the FORTRAN version.  Next we perform the
    # same mapping into the runtime format.  This was the C translation
    # of the FORTRAN code:
    # long loc;
    # while ((loc = GETNUM(database)) != -1) {
    #     long newloc = GETNUM(NULL);
    #     long L;
    #     if (TKEY[loc] == 0) {
    #         TKEY[loc] = TRVS;
    #     } else {
    #         TRAVEL[TRVS - 1] = -TRAVEL[TRVS - 1];
    #     }
    #     while ((L = GETNUM(NULL)) != 0) {
    #         TRAVEL[TRVS] = newloc * 1000 + L;
    #         TRVS = TRVS + 1;
    #         if (TRVS == TRVSIZ)
    #             BUG(TOO_MANY_TRAVEL_OPTIONS);
    #     }
    #     TRAVEL[TRVS - 1] = -TRAVEL[TRVS - 1];
    # }
    #
    # In order to de-crypticize the runtime code, we're going to break these
    # magic numbers up into a struct.
    travel = [[0, 0, 0, False, False]]
    tkey = [0]
    oldloc = 0
    while ltravel:
        rule = list(ltravel.pop(0))
        loc = rule.pop(0)
        newloc = rule.pop(0)
        if loc != oldloc:
            tkey.append(len(travel))
            oldloc = loc 
        elif travel:
            travel[-1][-1] = not travel[-1][-1]
        while rule:
            cond = newloc // 1000
            travel.append([rule.pop(0),
                           cond,
                           newloc % 1000,
                           cond==100,
                           False])
        travel[-1][-1] = True
    return (travel, tkey)

def get_travel(travel):
    template = """    {{
        .motion = {},
        .cond = {},
        .dest = {},
        .nodwarves = {},
        .stop = {},
    }},
"""
    out = ""
    for entry in travel:
        out += template.format(*entry).lower()
    out = out[:-1] # trim trailing newline
    return out

if __name__ == "__main__":
    with open(yaml_name, "r") as f:
        db = yaml.load(f)

    locnames = [x[0] for x in db["locations"]]
    msgnames = [el[0] for el in db["arbitrary_messages"]]
    objnames = [el[0] for el in db["objects"]]

    (travel, tkey) = buildtravel(db["locations"],
                                 db["objects"])
    ignore = ""
    c = c_template.format(
        h_name,
        get_arbitrary_messages(db["arbitrary_messages"]),
        get_class_messages(db["classes"]),
        get_turn_thresholds(db["turn_thresholds"]),
        get_locations(db["locations"]),
        get_objects(db["objects"]),
        get_obituaries(db["obituaries"]),
        get_hints(db["hints"], db["arbitrary_messages"]),
        get_condbits(db["locations"]),
        get_motions(db["motions"]),
        get_specials(db["actions"]),
        get_specials(db["specials"]),
        bigdump(tkey),
        get_travel(travel), 
        ignore,
    )

    # 0-origin index of birds's last song.  Bird should
    # die after player hears this.
    deathbird = len(dict(db["objects"])["BIRD"]["sounds"]) - 1

    h = h_template.format(
        len(db["locations"])-1,
        len(db["objects"])-1,
        len(db["hints"]),
        len(db["classes"])-1,
        len(db["obituaries"]),
        len(db["turn_thresholds"]),
        len(db["motions"]),
        len(db["actions"]),
        len(db["specials"]),
        len(travel),
        len(tkey),
        deathbird,
        get_refs(db["arbitrary_messages"]),
        get_refs(db["locations"]),
        get_refs(db["objects"]),
        get_refs(db["motions"]),
        get_refs(db["actions"]),
        get_refs(db["specials"]),
        statedefines,
    )

    with open(h_name, "w") as hf:
        hf.write(h)

    with open(c_name, "w") as cf:
        cf.write(c)

# end
