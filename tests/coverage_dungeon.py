#!/usr/bin/env python

# This is the open-adventure dungeon text coverage report generator. It
# consumes a YAML description of the dungeon and determines whether the
# various strings contained are present within the test check files.

import os
import yaml
import re

test_dir = "."
yaml_name = "../adventure.yaml"
html_template_path = "coverage_dungeon.html.tpl"
html_output_path = "../coverage/adventure.yaml.html"

row_3_fields = """
    <tr>
        <td class="coverFile">{}</td>
        <td class="{}">&nbsp;</td>
        <td class="{}">&nbsp;</td>
    </tr>
"""

row_2_fields = """
    <tr>
        <td class="coverFile">{}</td>
        <td class="{}">&nbsp;</td>
    </tr>
"""

def search(needle, haystack):
    # Search for needle in haystack, first escaping needle for regex, then
    # replacing %s, %d, etc. with regex wildcards, so the variable messages
    # within the dungeon definition will actually match
    needle = re.escape(needle) \
             .replace("\\n", "\n") \
             .replace("\\t", "\t") \
             .replace("\%S", ".*") \
             .replace("\%s", ".*") \
             .replace("\%d", ".*") \
             .replace("\%V", ".*")

    return re.search(needle, haystack)

def loc_coverage(locations, text):
    # locations have a long and a short description, that each have to 
    # be checked seperately
    for locname, loc in locations:
        if loc["description"]["long"] == None or loc["description"]["long"] == '':
            loc["description"]["long"] = True
        if loc["description"]["long"] != True:
            if search(loc["description"]["long"], text):
                loc["description"]["long"] = True
        if loc["description"]["short"] == None or loc["description"]["short"] == '':
            loc["description"]["short"] = True
        if loc["description"]["short"] != True:
            if search(loc["description"]["short"], text):
                loc["description"]["short"] = True

def arb_coverage(arb_msgs, text):
    # arbitrary messages are a map to tuples
    for i, msg in enumerate(arb_msgs):
        (msg_name, msg_text) = msg
        if msg_text == None or msg_text == '':
            arb_msgs[i] = (msg_name, True)
        elif msg_text != True:
            if search(msg_text, text):
                arb_msgs[i] = (msg_name, True)

def obj_coverage(objects, text):
    # objects have multiple descriptions based on state
    for i, objouter in enumerate(objects):
        (obj_name, obj) = objouter
        if obj["descriptions"]:
            for j, desc in enumerate(obj["descriptions"]):
                if desc == None or desc == '':
                    obj["descriptions"][j] = True
                    objects[i] = (obj_name, obj)
                elif desc != True:
                    if search(desc, text):
                        obj["descriptions"][j] = True
                        objects[i] = (obj_name, obj)

def hint_coverage(hints, text):
    # hints have a "question" where the hint is offered, followed
    # by the actual hint if the player requests it
    for name, hint in hints:
        if hint["question"] != True:
            if search(hint["question"], text):
                hint["question"] = True
        if hint["hint"] != True:
            if search(hint["hint"], text):
                hint["hint"] = True

def obit_coverage(obituaries, text):
    # obituaries have a "query" where it asks the player for a resurrection, 
    # followed by a snarky comment if the player says yes
    for i, obit in enumerate(obituaries):
        if obit["query"] != True:
            if search(obit["query"], text):
                obit["query"] = True
        if obit["yes_response"] != True:
            if search(obit["yes_response"], text):
                obit["yes_response"] = True

def threshold_coverage(classes, text):
    # works for class thresholds and turn threshold, which have a "message" 
    # property
    for i, msg in enumerate(classes):
        if msg["message"] == None:
            msg["message"] = True
        elif msg["message"] != True:
            if search(msg["message"], text):
                msg["message"] = True

def specials_actions_coverage(items, text):
    # works for actions or specials
    for name, item in items:
        if item["message"] == None or item["message"] == "NO_MESSAGE":
            item["message"] = True
        if item["message"] != True:
            if search(item["message"], text):
                item["message"] = True

if __name__ == "__main__":
    with open(yaml_name, "r") as f:
        db = yaml.load(f)

    with open(html_template_path, "r") as f:
        html_template = f.read()

    motions = db["motions"]
    locations = db["locations"]
    arb_msgs = db["arbitrary_messages"]
    objects = db["objects"]
    hintsraw = db["hints"]
    classes = db["classes"]
    turn_thresholds = db["turn_thresholds"]
    obituaries = db["obituaries"]
    actions = db["actions"]
    specials = db["specials"]

    hints = []
    for hint in hintsraw:
        hints.append((hint["hint"]["name"], {"question" : hint["hint"]["question"],"hint" : hint["hint"]["hint"]}))

    text = ""
    for filename in os.listdir(test_dir):
        if filename.endswith(".chk"):
            with open(filename, "r") as chk:
                text = chk.read()
                loc_coverage(locations, text)
                arb_coverage(arb_msgs, text)
                obj_coverage(objects, text)
                hint_coverage(hints, text)
                threshold_coverage(classes, text)
                threshold_coverage(turn_thresholds, text)
                obit_coverage(obituaries, text)
                specials_actions_coverage(actions, text)
                specials_actions_coverage(specials, text)

    location_html = ""
    location_total = len(locations) * 2
    location_covered = 0
    locations.sort()
    for locouter in locations:
        locname = locouter[0]
        loc = locouter[1]
        if loc["description"]["long"] != True:
            long_success = "uncovered"
        else:
            long_success = "covered"
            location_covered += 1

        if loc["description"]["short"] != True:
            short_success = "uncovered"
        else:
            short_success = "covered"
            location_covered += 1

        location_html += row_3_fields.format(locname, long_success, short_success)
    location_percent = round((location_covered / float(location_total)) * 100, 1)

    arb_msgs.sort()
    arb_msg_html = ""
    arb_total = len(arb_msgs)
    arb_covered = 0
    for name, msg in arb_msgs:
        if msg != True:
            success = "uncovered"
        else:
            success = "covered"
            arb_covered += 1
        arb_msg_html += row_2_fields.format(name, success)
    arb_percent = round((arb_covered / float(arb_total)) * 100, 1)

    object_html = ""
    objects_total = 0
    objects_covered = 0
    objects.sort()
    for (obj_name, obj) in objects:
        if obj["descriptions"]:
            for j, desc in enumerate(obj["descriptions"]):
                objects_total += 1
                if desc != True:
                    success = "uncovered"
                else:
                    success = "covered"
                    objects_covered += 1
                object_html += row_2_fields.format("%s[%d]" % (obj_name, j), success)
    objects_percent = round((objects_covered / float(objects_total)) * 100, 1)

    hints.sort()
    hints_html = "";
    hints_total = len(hints) * 2
    hints_covered = 0
    for name, hint in hints:
        if hint["question"] != True:
            question_success = "uncovered"
        else:
            question_success = "covered"
            hints_covered += 1
        if hint["hint"] != True:
            hint_success = "uncovered"
        else:
            hint_success = "covered"
            hints_covered += 1
        hints_html += row_3_fields.format(name, question_success, hint_success)
    hints_percent = round((hints_covered / float(hints_total)) * 100, 1)

    class_html = ""
    class_total = len(classes)
    class_covered = 0
    for name, msg in enumerate(classes):
        if msg["message"] != True:
            success = "uncovered"
        else:
            success = "covered"
            class_covered += 1
        class_html += row_2_fields.format(msg["threshold"], success)
    class_percent = round((class_covered / float(class_total)) * 100, 1)

    turn_html = ""
    turn_total = len(turn_thresholds)
    turn_covered = 0
    for name, msg in enumerate(turn_thresholds):
        if msg["message"] != True:
            success = "uncovered"
        else:
            success = "covered"
            turn_covered += 1
        turn_html += row_2_fields.format(msg["threshold"], success)
    turn_percent = round((turn_covered / float(turn_total)) * 100, 1)

    obituaries_html = "";
    obituaries_total = len(obituaries) * 2
    obituaries_covered = 0
    for i, obit in enumerate(obituaries):
        if obit["query"] != True:
            query_success = "uncovered"
        else:
            query_success = "covered"
            obituaries_covered += 1
        if obit["yes_response"] != True:
            obit_success = "uncovered"
        else:
            obit_success = "covered"
            obituaries_covered += 1
        obituaries_html += row_3_fields.format(i, query_success, obit_success)
    obituaries_percent = round((obituaries_covered / float(obituaries_total)) * 100, 1)

    actions.sort()
    actions_html = "";
    actions_total = len(actions)
    actions_covered = 0
    for name, action in actions:
        if action["message"] != True:
            success = "uncovered"
        else:
            success = "covered"
            actions_covered += 1
        actions_html += row_2_fields.format(name, success)
    actions_percent = round((actions_covered / float(actions_total)) * 100, 1)

    special_html = ""
    special_total = len(specials)
    special_covered = 0
    for name, special in specials:
        if special["message"] != True:
            success = "uncovered"
        else:
            success = "covered"
            special_covered += 1
        special_html += row_2_fields.format(name, success)
    special_percent = round((special_covered / float(special_total)) * 100, 1)

    # output some quick report stats
    print("\nadventure.yaml coverage rate:")
    print("  locations..........: {}% covered ({} of {})".format(location_percent, location_covered, location_total))
    print("  arbitrary_messages.: {}% covered ({} of {})".format(arb_percent, arb_covered, arb_total))
    print("  objects............: {}% covered ({} of {})".format(objects_percent, objects_covered, objects_total))
    print("  hints..............: {}% covered ({} of {})".format(hints_percent, hints_covered, hints_total))
    print("  classes............: {}% covered ({} of {})".format(class_percent, class_covered, class_total))
    print("  turn_thresholds....: {}% covered ({} of {})".format(turn_percent, turn_covered, turn_total))
    print("  obituaries.........: {}% covered ({} of {})".format(obituaries_percent, obituaries_covered, obituaries_total))
    print("  actions............: {}% covered ({} of {})".format(actions_percent, actions_covered, actions_total))
    print("  specials...........: {}% covered ({} of {})".format(special_percent, special_covered, special_total))

    # render HTML report
    with open(html_output_path, "w") as f:
        f.write(html_template.format(
                location_total, location_covered, location_percent,
                arb_total, arb_covered, arb_percent,
                objects_total, objects_covered, objects_percent,
                hints_total, hints_covered, hints_percent,
                class_total, class_covered, class_percent,
                turn_total, turn_covered, turn_percent,
                obituaries_total, obituaries_covered, obituaries_percent,
                actions_total, actions_covered, actions_percent,
                special_total, special_covered, special_percent,
                location_html, arb_msg_html, object_html, hints_html, 
                class_html, turn_html, obituaries_html, actions_html, special_html
        ))
