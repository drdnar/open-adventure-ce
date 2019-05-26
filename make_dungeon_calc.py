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

from collections import defaultdict
 
YAML_NAME = "adventure.yaml"
H_NAME = "dungeon.h"
C_NAME = "dungeon.c"
BIN_NAME = "dungeon.bin"
H_TEMPLATE_PATH = "templates/dungeon.h.tpl"
C_TEMPLATE_PATH = "templates/dungeon.c.tpl"

DONOTEDIT_COMMENT = "/* Generated from adventure.yaml - do not hand-hack! */\n\n"

statedefines = ""

condtype_t = {0: 0, "cond_goto": 0, "cond_pct": 1, "cond_carry": 2, "cond_with": 3, "cond_not": 4 }
desttype_t = {0: 0, "dest_goto": 0, "dest_special": 1, "dest_speak": 2 }

huffman_nodes = [ ]
huffman_codes = { }
huffman_root = None

symbol_frequencies = defaultdict(int)

def update_huffman_freq_table(string):
    """Updates the symbol frequency table with the given string."""
    global symbol_frequencies
    global huffman_nodes
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

class huffman_node:
    """Class used for forming the Huffman tree."""
    frequency = 0
    symbol = '\255'
    children = 1
    leaves = 0
    left = None
    right = None
    
    def __init__(self, freq, code = '\255'):
        self.symbol = code
        self.frequency = freq
        self.leaves = 1

    def is_leaf(self):
        if self.left == None and self.right == None:
            return True
        else:
            return False

class huffman_code:
    """Used for the code that translates bytes into Huffman symbols."""
    code = 0
    length = 1
    def __init__(self, ccode, llength):
        self.code = ccode
        self.length = llength

def build_huffman_tree():
    global huffman_nodes
    global huffman_codes
    global huffman_root
    # Build list of nodes
    for k, v in symbol_frequencies.items():
        huffman_nodes.append(huffman_node(v, k))
    # Build tree
    while len(huffman_nodes) > 1:
        huffman_nodes.sort(key = lambda n: n.frequency, reverse = False)
        n = huffman_nodes.pop(0)
        m = huffman_nodes.pop(0)
        o = huffman_node(n.frequency + m.frequency)
        o.left = n
        o.right = m
        o.children += n.children + m.children
        o.leaves = n.leaves + m.leaves
        huffman_nodes.append(o)
    huffman_root = huffman_nodes.pop(0)

def serialize_tree(tree, data, code, depth):
    if tree.is_leaf():
        if ord(tree.symbol) > 0x7F:
            raise Exception('Invalid input symbol {}'.format(ord(tree.symbol)))
        data.append(0x80 | ord(tree.symbol))
        huffman_codes[tree.symbol] = huffman_code(code, depth)
        return
    data.append(2)
    data.append(1 + tree.left.leaves + (tree.left.children - tree.left.leaves) * 2)
    if data[-1] > 127:
        raise Exception('Right tree too large!')
    serialize_tree(tree.left, data, code, depth + 1)
    serialize_tree(tree.right, data, code | (1 << depth), depth + 1)

def write_bits(data, bit, bits, length, character):
    bits = bits << bit
    data[-1] = (data[-1] | bits) & 0xFF
    if bit + length > 7:
        data.append(0)
        if bit + length > 8:
            return write_bits(data, 0, bits >> 8, length - (8 - bit), character)
        else:
            return 0
    else:
        bit = length + bit
        return bit

def compress_string(string, data):
#    print(string)
    bit = 0
    if string != None:
        for ch in string:
            code = huffman_codes[ch]
            bit = write_bits(data, bit, code.code, code.length, ch)
    code = huffman_codes['\0']
    # Pad out the rest of the byte if needed.
    if write_bits(data, bit, code.code, code.length, "NULL") > 0:
        data.append(0)

def add_compressed_string(string):
    """Adds a string to the compressed string list and returns the string number."""
    global duplicate_compressed_strings
    global duplicate_compressed_strings_chars
    global total_compressed_strings_chars
    if string == None:
        return 0
    string = string.replace("\\n", "\n")
    string = string.replace("\\t", "\t")
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
    if strings == []:
        strs = "{ 0 }"
    else:
        strs = "{" + ", ".join([str(add_uncompressed_string(s)) for s in strings]) + "}"
    n = len(strings)
    sg_str = template.format(n, strs)
    return sg_str

arbitrary_messages_index = [ ]
def get_arbitrary_messages(arb):
    global arbitrary_messages_index
    template = """    {},
"""
    arb_str = ""
    for item in arb:
        arbitrary_messages_index.append(add_compressed_string(item[1]))
        arb_str += template.format(arbitrary_messages_index[-1])
    arb_str = arb_str[:-1] # trim trailing newline
    return arb_str

tree_data_string = ""
tree_data = [ ]
strings_data = [ 0 ]
strings_locations = [ ]
def get_compressed_strings(strs):
    global tree_data_string
    global tree_data
    global strings_data
    global strings_locations
    # Build Huffman tree
    build_huffman_tree()
    serialize_tree(huffman_root, tree_data, 0, 0)
#    for k, v in huffman_codes.items():
#        code_str = ("{0:0" + str(v.length) + "b}").format(v.code)
#        if k == ' ':
#            print('space   , length {:2d}, code {}'.format(v.length, code_str))
#        elif k == '\n':
#            print('newline , length {:2d}, code {}'.format(v.length, code_str))
#        elif k == '\0':
#            print('null    , length {:2d}, code {}'.format(v.length, code_str))
#        else:
#            print('Symbol {}, length {:2d}, code {}'.format(k, v.length, code_str))
    for byte in tree_data:
        tree_data_string = '{}0x{:02X}, '.format(tree_data_string, byte)
    for i, string in enumerate(compressed_string_list):
        strings_locations.append(len(strings_data) - 1)
        compress_string(string, strings_data)
    print('Compressed strings size: {}'.format(len(strings_data)))
    
    template = """    /*{}*/
    (uint8_t[]){{{}}},
"""
    comp_str = ""
    for item in strs:
        string_data = [ 0 ]
        compress_string(item, string_data)
        string = ""
        for byte in string_data:
            string = '{}0x{:02X}, '.format(string, byte)
        comp_str += template.format(item, string)
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
        uncompressed_strings=get_uncompressed_strings(uncompressed_string_list),
        huffman_tree       = tree_data_string
    )
    
    
    # Generate dungeon binary blob
    def write_offset(data_arr, location, offset):
        data_arr[location] = offset & 0xFF
        data_arr[location + 1] = offset >> 8
    
    def append_offset(data_arr, offset):
        data_arr.append(offset & 0xFF)
        data_arr.append(offset >> 8)
    
    # Generate header
    data_file = [ ]
    for ch in "Colossal Cave Adventure dungeon":
        data_file.append(ord(ch))
    data_file.append(0)
    huffman_table_location_location = len(data_file)
    data_file.append(0)
    data_file.append(0)
    compressed_strings_location_location = len(data_file)
    data_file.append(0)
    data_file.append(0)
    uncompressed_strings_location_location = len(data_file)
    data_file.append(0)
    data_file.append(0)
    arbitrary_messages_location_location = len(data_file)
    data_file.append(0)
    data_file.append(0)
    classes_location_location = len(data_file)
    data_file.append(0)
    data_file.append(0)
    turn_thresholds_location_location = len(data_file)
    data_file.append(0)
    data_file.append(0)
    locations_location_location = len(data_file)
    data_file.append(0)
    data_file.append(0)
    objects_location_location = len(data_file)
    data_file.append(0)
    data_file.append(0)
    obituaries_location_location = len(data_file)
    data_file.append(0)
    data_file.append(0)
    hints_location_location = len(data_file)
    data_file.append(0)
    data_file.append(0)
    unused_location_location = len(data_file)
    data_file.append(0)
    data_file.append(0)
    motions_location_location = len(data_file)
    data_file.append(0)
    data_file.append(0)
    actions_location_location = len(data_file)
    data_file.append(0)
    data_file.append(0)
    tkey_location_location = len(data_file)
    data_file.append(0)
    data_file.append(0)
    travel_location_location = len(data_file)
    data_file.append(0)
    data_file.append(0)
    
    # Add serialized Huffman tree
    huffman_table_location = len(data_file)
    write_offset(data_file, huffman_table_location_location, huffman_table_location)
    data_file.extend(tree_data)
    # Generate compressed strings index
    compressed_strings_location = len(data_file)
    write_offset(data_file, compressed_strings_location_location, compressed_strings_location)
    for l in strings_locations:
        append_offset(data_file, compressed_strings_location + 2 * len(strings_locations) + l)
    # And now add compressed strings data
    data_file.extend(strings_data)
    
    # Now do uncompressed strings
    uncompressed_strings_location = len(data_file)
    write_offset(data_file, uncompressed_strings_location_location, uncompressed_strings_location)
    # Generate uncompressed strings index
    data_file.extend([0] * (2 * len(uncompressed_string_list)))
    # Write uncompressed strings
    for s in uncompressed_string_list:
        write_offset(data_file, uncompressed_strings_location, len(data_file))
        uncompressed_strings_location += 2
        if s != None:
            for ch in s:
                data_file.append(ord(ch))
        data_file.append(0)
    
    # Arbitrary messages reference list
    # TODO: This is obviously redundant
    arbitrary_messages_location = len(data_file)
    write_offset(data_file, arbitrary_messages_location_location, arbitrary_messages_location)
    for i in arbitrary_messages_index:
        append_offset(data_file, i)
    
    # Classes
    classes_location = len(data_file)
    write_offset(data_file, classes_location_location, classes_location)
    for item in db["classes"]:
        append_offset(data_file, item["threshold"])
        # This is safe to do again thanks to string deduplication
        append_offset(data_file, add_compressed_string(item["message"]))
    
    # Turn thresholds
    turn_thresholds_location = len(data_file)
    write_offset(data_file, turn_thresholds_location_location, turn_thresholds_location)
    for item in db["turn_thresholds"]:
        append_offset(data_file, item["threshold"])
        data_file.append(item["point_loss"])
        append_offset(data_file, add_compressed_string(item["message"]))
    
    # Locations
    locations_location = len(data_file)
    write_offset(data_file, locations_location_location, locations_location)
    for item in db["locations"]:
        append_offset(data_file, add_compressed_string(item[1]["description"]["short"]))
        append_offset(data_file, add_compressed_string(item[1]["description"]["long"]))
        sounds = [x for x in db["arbitrary_messages"] if x[0] == item[1].get("sound", "SILENT")]
        if len(sounds) > 0:
            data_file.append(db["arbitrary_messages"].index(sounds[0]))
        else:
            data_file.append(255)
        data_file.append(1 if item[1].get("loud") else 0)
    
    # Objects
    objects_location = len(data_file)
    write_offset(data_file, objects_location_location, objects_location)
    # Generate index
    data_file.extend([0] * (2 * len(db["objects"])))
    # Now generate objects
    def get_location_index(str):
        # If -1 is passed, return -1
        if str == -1:
            return 0xFFFF
        if str == 0:
            return 0
        locs = [x for x in db["locations"] if x[0] == str]
        if len(locs) < 1:
            print('Unknown location {}'.format(str))
            sys.exit(1)
        return db["locations"].index(locs[0])
    for (i, item) in enumerate(db["objects"]):
        write_offset(data_file, objects_location, len(data_file))
        objects_location += 2
        attr = item[1]
        append_offset(data_file, add_compressed_string(attr["inventory"]))
        locs = attr.get("locations", ["LOC_NOWHERE", "LOC_NOWHERE"])
        if type(locs) == str:
            locs = [locs, -1 if attr.get("immovable", False) else 0]
        append_offset(data_file, get_location_index(locs[0]))
        append_offset(data_file, get_location_index(locs[1]))
        data_file.append(1 if attr.get("treasure") else 0)
        strings = [ ]
        if attr.get("words") != None:
            for word in attr["words"]:
                strings.append(add_uncompressed_string(word))
        descriptions_start = len(strings)
        if attr.get("descriptions") != None:
            for msg in attr["descriptions"]:
                strings.append(add_compressed_string(msg))
        sounds_start = len(strings)
        if attr.get("sounds") != None:
            for msg in attr["sounds"]:
                strings.append(add_compressed_string(msg))
        texts_start = len(strings)
        if attr.get("texts") != None:
            for msg in attr["texts"]:
                strings.append(add_compressed_string(msg))
        changes_start = len(strings)
        if attr.get("changes") == None:
            strings.append(0)
        else:
            for msg in attr["changes"]:
                strings.append(add_compressed_string(msg))
        data_file.append(descriptions_start)
        data_file.append(sounds_start)
        data_file.append(texts_start)
        data_file.append(changes_start)
        for i in strings:
            append_offset(data_file, i)
    
    # Obituaries
    obituaries_location = len(data_file)
    write_offset(data_file, obituaries_location_location, obituaries_location)
    for item in db["obituaries"]:
        append_offset(data_file, add_compressed_string(item["query"]))
        append_offset(data_file, add_compressed_string(item["yes_response"]))
    
    # Hints
    hints_location = len(data_file)
    write_offset(data_file, hints_location_location, hints_location)
    for i in db["hints"]:
        item = i["hint"]
        data_file.append(item["number"])
        data_file.append(item["penalty"])
        data_file.append(item["turns"])
        append_offset(data_file, add_compressed_string(item["question"]))
        append_offset(data_file, add_compressed_string(item["hint"]))
    
    # Possible future location of conditions array
    
    # Motions
    motions_location = len(data_file)
    write_offset(data_file, motions_location_location, motions_location)
    # Generate index
    data_file.extend([0] * (2 * len(db["motions"])))
    # Generate motions
    for i in db["motions"]:
        write_offset(data_file, motions_location, len(data_file))
        motions_location += 2
        item = i[1]
        if item["words"] == None:
            data_file.append(0)
        else:
            for string in item["words"]:
                append_offset(data_file, add_uncompressed_string(string))
    
    # Actions
    actions_location = len(data_file)
    write_offset(data_file, actions_location_location, actions_location)
    # Generate index
    data_file.extend([0] * (2 * len(db["actions"])))
    # Generate motions
    for i in db["actions"]:
        write_offset(data_file, actions_location, len(data_file))
        actions_location += 2
        item = i[1]
        append_offset(data_file, add_compressed_string(item["message"]))
        if item.get("noaction") == None:
            data_file.append(0)
        else:
            data_file.append(1)
        if item["words"] == None:
            data_file.append(0)
        else:
            for string in item["words"]:
                append_offset(data_file, add_uncompressed_string(string))
    
    # tkey, whatever that means
    tkey_location = len(data_file)
    write_offset(data_file, tkey_location_location, tkey_location)
    for (i, entry) in enumerate(tkey):
        append_offset(data_file, tkey[i])
    
    # Travel
    message_refs = [x[0] for x in db["arbitrary_messages"]]
    location_refs = [x[0] for x in db["locations"]]
    object_refs = [x[0] for x in db["objects"]]
    motion_refs = [x[0] for x in db["motions"]]
    #action_refs = [x[0] for x in db["actions"]]
    travel_location = len(data_file)
    write_offset(data_file, travel_location_location, travel_location)
    def deref_enum(enum, alt, value, q):
        if type(value) == int:
            data_file.append(value)
        else:
            try:
                data_file.append(enum.index(value))
            except ValueError:
                data_file.append(alt.index(value))
    for item in travel:
        # motion, may be a number, or a string to deref to a motion number
        deref_enum(motion_refs, None, item[2], "motion")
        # condtype, may be zero, or a string to deref to a condition enum value
        data_file.append(condtype_t[item[3]])
        # condarg1, may be a number, or a string to deref to an object enum value
        deref_enum(object_refs, None, item[4], "condarg1")
        # condarg2, always a number, but for some reason sometimes comes out as a float?
        data_file.append(int(item[5]))
        # desttype, may be zero, or a string to deref to a desttype enum value
        data_file.append(desttype_t[item[6]])
        # destval, may be a number, or a string to deref to a location enum
        deref_enum(location_refs, message_refs, item[7], "destval")
        # nodwarves
        data_file.append(1 if item[8] else 0)
        # stop
        data_file.append(1 if item[9] else 0)
    
    print('Data file size: {}'.format(len(data_file)))
    
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
        num_arb_msgs       = len(arbitrary_messages_index),
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

    with open(BIN_NAME, "wb") as bf:
        bf.write(bytearray(data_file))
        
    
    print('Total compressed strings: {}'.format(len(compressed_string_list)))
    print('Total compressed strings chars: {}'.format(total_compressed_strings_chars))
    print('Duplicate compressed strings omitted: {}'.format(duplicate_compressed_strings))
    print('Duplicate compressed strings chars omitted: {}'.format(duplicate_compressed_strings_chars))
    print('Huffman table codes: {}'.format(len(symbol_frequencies)))
    print('Total uncompressed strings: {}'.format(len(uncompressed_string_list)))
    print('Total uncompressed strings chars: {}'.format(total_uncompressed_strings_chars))
    print('Duplicate uncompressed strings omitted: {}'.format(duplicate_uncompressed_strings))
    print('Duplicate uncompressed strings chars omitted: {}'.format(duplicate_uncompressed_strings_chars))

# end
