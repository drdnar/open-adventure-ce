#!/usr/bin/env python3

# This is the open-adventure dungeon generator. It consumes a YAML description of
# the dungeon and outputs a dungeon.h and dungeon.c pair of C code files.
#
# The nontrivial part of this is the compilation of the YAML for
# movement rules to the travel array that's actually used by
# playermove().
#
# Copyright (c) 2017 by Eric S. Raymond
# SPDX-License-Identifier: BSD-2-clause

import sys, yaml

from heapq import heappush, heappop, heapify
from collections import defaultdict
 
YAML_NAME = "adventure.yaml"
H_NAME = "dungeon.h"
C_NAME = "dungeon.c"
H_TEMPLATE_PATH = "templates/dungeon.h.tpl"
C_TEMPLATE_PATH = "templates/dungeon.c.tpl"

DONOTEDIT_COMMENT = "/* Generated from adventure.yaml - do not hand-hack! */\n\n"

statedefines = ""


symbol_frequencies = defaultdict(int)
def build_huffman_code_table(symbfreq):
    """Huffman encode the given dict mapping symbols to weights"""
    # Taken from https://rosettacode.org/wiki/Huffman_coding#Python
    # I wrote my own C# code to do this a while ago, but this is, like, clever-looking
    # and it was easier to use Google than to dig up my old code and convert it
    heap = [[wt, [sym, ""]] for sym, wt in symbfreq.items()]
    heapify(heap)
    while len(heap) > 1:
        lo = heappop(heap)
        hi = heappop(heap)
        for pair in lo[1:]:
            pair[1] = '0' + pair[1]
        for pair in hi[1:]:
            pair[1] = '1' + pair[1]
        heappush(heap, [lo[0] + hi[0]] + lo[1:] + hi[1:])
    return sorted(heappop(heap)[1:], key=lambda p: (len(p[-1]), p))

def update_huffman_freq_table(string):
    """Updates the symbol frequency table with the given string."""
    global symbol_frequencies
    for ch in string:
        symbol_frequencies[ch] += 1
    # Every string should also have an end.
    symbol_frequencies['\0'] += 1

# String 0 is the NULL string, i.e. the string that doesn't exist
# It is different than the empty string, a zero-length string rather than the absence of a string.
compressed_string_list = [ None ]
uncompressed_string_list = [ None ]
duplicate_compressed_strings = 0
duplicate_compressed_strings_chars = 0
total_compressed_strings_chars = 0
duplicate_uncompressed_strings = 0
duplicate_uncompressed_strings_chars = 0
total_uncompressed_strings_chars = 0

def add_compressed_string(string):
    """Adds a string to the compressed string list and returns the string number."""
    global duplicate_compressed_strings
    global duplicate_compressed_strings_chars
    global total_compressed_strings_chars
    if string == None:
        return 0
    if compressed_string_list.count(string) > 0:
        duplicate_compressed_strings += 1
        duplicate_compressed_strings_chars += len(string)
        return compressed_string_list.index(string)
    update_huffman_freq_table(string)
    compressed_string_list.append(string)
    total_compressed_strings_chars += len(string)
    return len(compressed_string_list) - 1

def add_uncompressed_string(string):
    """Adds a string to the UNcompressed string list and returns the string number."""
    global duplicate_uncompressed_strings
    global duplicate_uncompressed_strings_chars
    global total_uncompressed_strings_chars
    if string == None:
        return 0
    if uncompressed_string_list.count(string) > 0:
        duplicate_uncompressed_strings += 1
        duplicate_uncompressed_strings_chars += len(string)
        return uncompressed_string_list.index(string)
    uncompressed_string_list.append(string)
    total_uncompressed_strings_chars += len(string)
    return len(uncompressed_string_list) - 1

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
            .n = {},
            .strs = {}
        }}"""
    message_numbers = [ ]
    if strings == []:
        strs = "{ 0 }"
    else:
        strs = "{" + ", ".join([str(add_uncompressed_string(s)) for s in strings]) + "}"
    n = len(strings)
    sg_str = template.format(n, strs)
    return sg_str

def get_arbitrary_messages(arb):
    template = """    {},
"""
    arb_str = ""
    for item in arb:
        arb_str += template.format(add_compressed_string(item[1]))
    arb_str = arb_str[:-1] # trim trailing newline
    return arb_str

def get_compressed_strings(strs):
    template = """    {},
"""
    comp_str = ""
    for item in strs:
        comp_str += template.format(make_c_string(item))
    comp_str = comp_str[:-1] # trim trailing newline
    return comp_str

def get_uncompressed_strings(strs):
    template = """    {},
"""
    uncomp_str = ""
    for item in strs:
        uncomp_str += template.format(make_c_string(item))
    uncomp_str = uncomp_str[:-1] # trim trailing newline
    return uncomp_str

def get_class_messages(cls):
    template = """    {{
        .threshold = {},
        .message = {},
    }},
"""
    cls_str = ""
    for item in cls:
        threshold = item["threshold"]
        message = add_compressed_string(item["message"])
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
        message = add_compressed_string(item["message"])
        trn_str += template.format(threshold, point_loss, message)
    trn_str = trn_str[:-1] # trim trailing newline
    return trn_str

def get_locations(loc):
    template = """    {{ // {}: {}
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
        short_d = add_compressed_string(item[1]["description"]["short"])
        long_d = add_compressed_string(item[1]["description"]["long"])
        sound = item[1].get("sound", "SILENT")
        loud = "true" if item[1].get("loud") else "false"
        loc_str += template.format(i, item[0], short_d, long_d, sound, loud)
    loc_str = loc_str[:-1] # trim trailing newline
    return loc_str

def get_objects(obj):
    template = """    {{ // {index}: {name}
        .inventory = {inventory},
        .plac = {plac},
        .fixd = {fixd},
        .is_treasure = {is_treasure},
        .descriptions_start = {descriptions_start},
        .sounds_start = {sounds_start},
        .texts_start = {texts_start},
        .changes_start = {changes_start},
        .strings = {{
           {strings}
        }},
    }},
"""
    obj_str = ""
    for (i, item) in enumerate(obj):
        attr = item[1]
        strings = ""
        try:
            descriptions_start = len(attr["words"])
            for word in attr["words"]:
                strings += " " + str(add_uncompressed_string(word)) + ","
        except KeyError:
            descriptions_start = 0
        i_msg = add_compressed_string(attr["inventory"])
        if attr["descriptions"] == None:
            sounds_start = descriptions_start
        else:
            labels = []
            for l_msg in attr["descriptions"]:
                strings += " " + str(add_compressed_string(l_msg)) + ","
            sounds_start = descriptions_start + len(attr["descriptions"])
            for label in attr.get("states", []):
                labels.append(label)
            if labels:
                global statedefines
                statedefines += "/* States for %s */\n" % item[0]
                for (n, label) in enumerate(labels):
                    statedefines += "#define %s\t%d\n" % (label, n)
                statedefines += "\n"
        if attr.get("sounds") == None:
            texts_start = sounds_start
        else:
             for l_msg in attr["sounds"]:
                 strings += " " + str(add_compressed_string(l_msg)) + ","
             texts_start = sounds_start + len(attr["sounds"])
        if attr.get("texts") == None:
            changes_start = texts_start
        else:
             for l_msg in attr["texts"]:
                 strings += " " + str(add_compressed_string(l_msg)) + ","
             changes_start = texts_start + len(attr["texts"])
        if attr.get("changes") == None:
            strings += " 0," # null string
        else:
             for l_msg in attr["changes"]:
                 strings += " " + str(add_compressed_string(l_msg)) + ","
        locs = attr.get("locations", ["LOC_NOWHERE", "LOC_NOWHERE"])
        immovable = attr.get("immovable", False)
        try:
            if type(locs) == str:
                locs = [locs, -1 if immovable else 0]
        except IndexError:
            sys.stderr.write("dungeon: unknown object location in %s\n" % locs)
            sys.exit(1)
        treasure = "true" if attr.get("treasure") else "false"
        obj_str += template.format(index = i, name = item[0], inventory = i_msg, plac = locs[0], fixd = locs[1], is_treasure = treasure, descriptions_start = descriptions_start, sounds_start = sounds_start, texts_start = texts_start, changes_start = changes_start, strings = strings)
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
        query = add_compressed_string(o["query"])
        yes = add_compressed_string(o["yes_response"])
        obit_str += template.format(query, yes)
    obit_str = obit_str[:-1] # trim trailing newline
    return obit_str

def get_hints(hnt):
    template = """    {{
        .number = {},
        .penalty = {},
        .turns = {},
        .question = {},
        .hint = {},
    }},
"""
    hnt_str = ""
    for member in hnt:
        item = member["hint"]
        number = item["number"]
        penalty = item["penalty"]
        turns = item["turns"]
        question = add_compressed_string(item["question"])
        hint = add_compressed_string(item["hint"])
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
        .noaction = {},
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
            message = "0"
        else:
            message = add_compressed_string(contents["message"])

        if contents.get("noaction") == None:
            noaction = "false"
        else:
            noaction = "true"

        act_str += template.format(words_str, message, noaction)
        global ignore
        if contents.get("oldstyle", True) == False:
            for word in contents["words"]:
                if len(word) == 1:
                    ignore += word.upper()
    act_str = act_str[:-1] # trim trailing newline
    return act_str

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
    assert len(locs) <= 300
    assert len(objs) <= 100
    # THIS CODE IS WAAAY MORE COMPLEX THAN IT NEEDS TO BE.  It's the
    # result of a massive refactoring exercise that concentrated all
    # the old nastiness in one spot. It hasn't been finally simplified
    # because there's no need to do it until one of the assertions
    # fails. Hint: if you try cleaning this up, the acceptance test is
    # simple - the output dungeon.c must not change.
    #
    # This function first compiles the YAML to a form identical to the
    # data in section 3 of the old adventure.text file, then a second
    # stage unpacks that data into the travel array.  Here are the
    # rules of that intermediate form:
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
    # This says that, from 11, 49 takes him to 8 unless game.prop[3]=0, in which
    # case he goes to 9.  Verb 50 takes him to 9 regardless of game.prop[3].
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
                sys.stderr.write("dungeon: unknown location %s in goto clause of %s\n" % (action[1], name))
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
                tt += [motionnames[verbmap[e]].upper() for e in rule["verbs"]]
                if not rule["verbs"]:
                    tt.append(1)	# Magic dummy entry for null rules
                ltravel.append(tuple(tt))

    # At this point the ltravel data is in the Section 3
    # representation from the FORTRAN version.  Next we perform the
    # same mapping into what used to be the runtime format.

    travel = [[0, "LOC_NOWHERE", 0, 0, 0, 0, 0, 0, "false", "false"]]
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
            travel[-1][-1] = "false" if travel[-1][-1] == "true" else "true" 
        while rule:
            cond = newloc // 1000
            nodwarves = (cond == 100)
            if cond == 0:
                condtype = "cond_goto"
                condarg1 = condarg2 = 0
            elif cond < 100:
                condtype = "cond_pct"
                condarg1 = cond
                condarg2 = 0
            elif cond == 100:
                condtype = "cond_goto"
                condarg1 = 100
                condarg2 = 0
            elif cond <= 200:
                condtype = "cond_carry"
                condarg1 = objnames[cond - 100]
                condarg2 = 0
            elif cond <= 300:
                condtype = "cond_with"
                condarg1 = objnames[cond - 200]
                condarg2 = 0
            else:
                condtype = "cond_not"
                condarg1 = cond % 100
                condarg2 = (cond - 300) // 100.
            dest = newloc % 1000
            if dest <= 300:
                desttype = "dest_goto";
                destval = locnames[dest]
            elif dest > 500:
                desttype = "dest_speak";
                destval = msgnames[dest - 500]
            else:
                desttype = "dest_special";
                destval = locnames[dest - 300]
            travel.append([len(tkey)-1,
                           locnames[len(tkey)-1],
                           rule.pop(0),
                           condtype,
                           condarg1,
                           condarg2,
                           desttype,
                           destval,
                           "true" if nodwarves else "false",
                           "false"])
        travel[-1][-1] = "true"
    return (travel, tkey)

def get_travel(travel):
    template = """    {{ // from {}: {}
        .motion = {},
        .condtype = {},
        .condarg1 = {},
        .condarg2 = {},
        .desttype = {},
        .destval = {},
        .nodwarves = {},
        .stop = {},
    }},
"""
    out = ""
    for entry in travel:
        out += template.format(*entry)
    out = out[:-1] # trim trailing newline
    return out

if __name__ == "__main__":
    with open(YAML_NAME, "r") as f:
        db = yaml.load(f)

    locnames = [x[0] for x in db["locations"]]
    msgnames = [el[0] for el in db["arbitrary_messages"]]
    objnames = [el[0] for el in db["objects"]]
    motionnames = [el[0] for el in db["motions"]]

    (travel, tkey) = buildtravel(db["locations"],
                                 db["objects"])
    ignore = ""
    try:
        with open(H_TEMPLATE_PATH, "r") as htf:
            # read in dungeon.h template
            h_template = DONOTEDIT_COMMENT + htf.read()
        with open(C_TEMPLATE_PATH, "r") as ctf:
            # read in dungeon.c template
            c_template = DONOTEDIT_COMMENT + ctf.read()
    except IOError as e:
        print('ERROR: reading template failed ({})'.format(e.strerror))
        exit(-1)

    c = c_template.format(
        h_file             = H_NAME,
        arbitrary_messages = get_arbitrary_messages(db["arbitrary_messages"]),
        classes            = get_class_messages(db["classes"]),
        turn_thresholds    = get_turn_thresholds(db["turn_thresholds"]),
        locations          = get_locations(db["locations"]),
        objects            = get_objects(db["objects"]),
        obituaries         = get_obituaries(db["obituaries"]),
        hints              = get_hints(db["hints"]),
        conditions         = get_condbits(db["locations"]),
        motions            = get_motions(db["motions"]),
        actions            = get_actions(db["actions"]),
        tkeys              = bigdump(tkey),
        travel             = get_travel(travel), 
        ignore             = ignore,
        compressed_strings = get_compressed_strings(compressed_string_list),
        uncompressed_strings=get_uncompressed_strings(uncompressed_string_list)
    )

    # 0-origin index of birds's last song.  Bird should
    # die after player hears this.
    deathbird = len(dict(db["objects"])["BIRD"]["sounds"]) - 1

    h = h_template.format(
        num_locations      = len(db["locations"])-1,
        num_objects        = len(db["objects"])-1,
        num_hints          = len(db["hints"]),
        num_classes        = len(db["classes"])-1,
        num_deaths         = len(db["obituaries"]),
        num_thresholds     = len(db["turn_thresholds"]),
        num_motions        = len(db["motions"]),
        num_actions        = len(db["actions"]),
        num_travel         = len(travel),
        num_keys           = len(tkey),
        num_comp_strs      = len(compressed_string_list),
        num_uncomp_strs    = len(uncompressed_string_list),
        bird_endstate      = deathbird,
        arbitrary_messages = get_refs(db["arbitrary_messages"]),
        locations          = get_refs(db["locations"]),
        objects            = get_refs(db["objects"]),
        motions            = get_refs(db["motions"]),
        actions            = get_refs(db["actions"]),
        state_definitions  = statedefines
    )

    with open(H_NAME, "w") as hf:
        hf.write(h)

    with open(C_NAME, "w") as cf:
        cf.write(c)
    
    print('Total compressed strings: {}'.format(len(compressed_string_list)))
    print('Total compressed strings chars: {}'.format(total_compressed_strings_chars))
    print('Duplicate compressed strings omitted: {}'.format(duplicate_compressed_strings))
    print('Duplicate compressed strings chars omitted: {}'.format(duplicate_compressed_strings_chars))
    print('Huffman table codes: {}'.format(len(symbol_frequencies)))
    huffman_table = build_huffman_code_table(symbol_frequencies)
#    print('Symbol\tWeight\tHuffman Code')
#    for p in huffman_table:
#        if p[0] == '\0':
#            print('null\t{}\t{}'.format(symbol_frequencies[p[0]], p[1]))
#        elif p[0] == ' ':
#            print('space\t{}\t{}'.format(symbol_frequencies[p[0]], p[1]))
#        elif p[0] == '\n':
#            print('newline\t{}\t{}'.format(symbol_frequencies[p[0]], p[1]))
#        else:
#            print('{}\t{}\t{}'.format(p[0], symbol_frequencies[p[0]], p[1]))
    print('Total uncompressed strings: {}'.format(len(uncompressed_string_list)))
    print('Total uncompressed strings chars: {}'.format(total_uncompressed_strings_chars))
    print('Duplicate uncompressed strings omitted: {}'.format(duplicate_uncompressed_strings))
    print('Duplicate uncompressed strings chars omitted: {}'.format(duplicate_uncompressed_strings_chars))

# end
