#!/usr/bin/python3

import os
import yaml

import pprint

test_dir = "."
yaml_name = "../adventure.yaml"
html_template_path = "coverage_dungeon.html.tpl"
html_output_path = "../coverage/adventure.yaml.html"

location_row = """
    <tr>
        <td class="coverFile">{}</td>
        <td class="{}">&nbsp;</td>
        <td class="{}">&nbsp;</td>
    </tr>
"""

arb_msg_row = """
    <tr>
        <td class="coverFile">{}</td>
        <td class="{}">&nbsp;</td>
    </tr>
"""

object_row = """
    <tr>
        <td class="coverFile">{}</td>
        <td class="{}">&nbsp;</td>
    </tr>
"""

def loc_coverage(locations, text):
    for locname, loc in locations:
        if loc["description"]["long"] == None or loc["description"]["long"] == '':
            loc["description"]["long"] = True
        if loc["description"]["long"] != True:
            if text.find(loc["description"]["long"]) != -1:
                loc["description"]["long"] = True
        if loc["description"]["short"] == None or loc["description"]["short"] == '':
            loc["description"]["short"] = True
        if loc["description"]["short"] != True:
            if text.find(loc["description"]["short"]) != -1:
                loc["description"]["short"] = True

def arb_coverage(arb_msgs, text):
    for i, msg in enumerate(arb_msgs):
        (msg_name, msg_text) = msg
        if msg_text == None or msg_text == '':
            arb_msgs[i] = (msg_name, True)
        elif msg_text != True:
            if text.find(msg_text) != -1:
                arb_msgs[i] = (msg_name, True)

def obj_coverage(objects, text):
    for i, objouter in enumerate(objects):
        (obj_name, obj) = objouter
        if obj["descriptions"]:
            for j, desc in enumerate(obj["descriptions"]):
                if desc == None or desc == '':
                    obj["descriptions"][j] = True
                    objects[i] = (obj_name, obj)
                elif desc != True:
                    if text.find(desc) != -1:
                        obj["descriptions"][j] = True
                        objects[i] = (obj_name, obj)

if __name__ == "__main__":
    with open(yaml_name, "r") as f:
        db = yaml.load(f)

    with open(html_template_path, "r") as f:
        html_template = f.read()

    locations = db["locations"]
    arb_msgs = db["arbitrary_messages"]
    objects = db["objects"]

    text = ""
    for filename in os.listdir(test_dir):
        if filename.endswith(".chk"):
            with open(filename, "r") as chk:
                text = chk.read()
                loc_coverage(locations, text)
                arb_coverage(arb_msgs, text)
                obj_coverage(objects, text)

    print("\nadventure.yaml coverage rate:")

    location_html = ""
    location_total = len(locations) * 2
    location_covered = 0
    for locouter in locations:
        locname = locouter[0]
        loc = locouter[1]
        long_success = "covered"
        short_success = "covered"
        if loc["description"]["long"] != True:
            long_success = "uncovered"
        else:
            location_covered += 1

        if loc["description"]["short"] != True:
            short_success = "uncovered"
        else:
            location_covered += 1

        location_html += location_row.format(locname, long_success, short_success)
    location_percent = round((location_covered / location_total) * 100, 1)
    print("  locations..........: {}% covered ({} of {})".format(location_percent, location_covered, location_total))

    arb_msg_html = ""
    arb_total = len(arb_msgs)
    arb_covered = 0
    for name, msg in arb_msgs:
        success = "covered"
        if msg != True:
            success = "uncovered"
        else:
            arb_covered += 1
        arb_msg_html += arb_msg_row.format(name, success)
    arb_percent = round((arb_covered / arb_total) * 100, 1)
    print("  arbitrary_messages.: {}% covered ({} of {})".format(arb_percent, arb_covered, arb_total))

    object_html = ""
    objects_total = 0
    objects_covered = 0
    for (obj_name, obj) in objects:
        if obj["descriptions"]:
            for j, desc in enumerate(obj["descriptions"]):
                objects_total += 1
                success = "covered"
                if desc != True:
                    success = "uncovered"
                else:
                    objects_covered += 1
                object_html += object_row.format("%s[%d]" % (obj_name, j), success)
    objects_percent = round((objects_covered / objects_total) * 100, 1)
    print("  objects............: {}% covered ({} of {})".format(objects_percent, objects_covered, objects_total))

    with open(html_output_path, "w") as f:
        f.write(html_template.format(
                location_total, location_covered, location_percent,
                arb_total, arb_covered, arb_percent,
                objects_total, objects_covered, objects_percent,
                location_html, arb_msg_html, object_html
        ))
