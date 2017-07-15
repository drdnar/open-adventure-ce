#!/usr/bin/env python

# This is the open-adventure dungeon text coverage report generator. It
# consumes a YAML description of the dungeon and determines whether the
# various strings contained are present within the test check files.
#
# The default HTML output is appropriate for use with Gitlab CI.
# You can override it with a command-line argument.

import os
import sys
import yaml
import re

TEST_DIR = "."
YAML_PATH = "../adventure.yaml"
HTML_TEMPLATE_PATH = "coverage_dungeon.html.tpl"
DEFAULT_HTML_OUTPUT_PATH = "../coverage/adventure.yaml.html"

STDOUT_REPORT_CATEGORY = "  {name:.<19}: {percent}% covered ({covered} of {total}))\n"

HTML_SUMMARY_ROW = """
    <tr>
        <td class="headerItem"><a href="#{name}">{name}:</a></td>
        <td class="headerCovTableEntry">{total}</td>
        <td class="headerCovTableEntry">{covered}</td>
        <td class="headerCovTableEntry">{percent}%</td>
    </tr>
"""

HTML_CATEGORY_TABLE = """
    <tr id="{id}"></tr>
    {rows}
    <tr>
        <td>&nbsp;</td>
    </tr>
"""

HTML_CATEGORY_TABLE_HEADER_3_FIELDS = """
    <tr>
        <td class="tableHead" width="60%">{}</td>
        <td class="tableHead" width="20%">{}</td>
        <td class="tableHead" width="20%">{}</td>
    </tr>
"""

HTML_CATEGORY_TABLE_HEADER_2_FIELDS = """
    <tr>
        <td class="tableHead" colspan="2">{}</td>
        <td class="tableHead">{}</td>
    </tr>
"""

HTML_CATEGORY_ROW_3_FIELDS = """
    <tr>
        <td class="coverFile">{}</td>
        <td class="{}">&nbsp;</td>
        <td class="{}">&nbsp;</td>
    </tr>
"""

HTML_CATEGORY_ROW_2_FIELDS = """
    <tr>
        <td class="coverFile" colspan="2">{}</td>
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
    for i, hint in enumerate(hints):
        if hint["hint"]["question"] != True:
            if search(hint["hint"]["question"], text):
                hint["hint"]["question"] = True
        if hint["hint"]["hint"] != True:
            if search(hint["hint"]["hint"], text):
                hint["hint"]["hint"] = True

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
    with open(YAML_PATH, "r") as f:
        db = yaml.load(f)

    # Create report for each catagory, including HTML table, total items,
    # and number of items covered
    report = {}
    for name in db.keys():
        # initialize each catagory
        report[name] = {
            "name" : name, # convenience for string formatting
            "html" : "",
            "total" : 0,
            "covered" : 0
        }

    motions = db["motions"]
    locations = db["locations"]
    arb_msgs = db["arbitrary_messages"]
    objects = db["objects"]
    hints = db["hints"]
    classes = db["classes"]
    turn_thresholds = db["turn_thresholds"]
    obituaries = db["obituaries"]
    actions = db["actions"]
    specials = db["specials"]

    text = ""
    for filename in os.listdir(TEST_DIR):
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

    report["locations"]["total"] = len(locations) * 2
    report["locations"]["html"] = HTML_CATEGORY_TABLE_HEADER_3_FIELDS.format("Location ID", "long", "short")
    locations.sort()
    for locouter in locations:
        locname = locouter[0]
        loc = locouter[1]
        if loc["description"]["long"] != True:
            long_success = "uncovered"
        else:
            long_success = "covered"
            report["locations"]["covered"] += 1

        if loc["description"]["short"] != True:
            short_success = "uncovered"
        else:
            short_success = "covered"
            report["locations"]["covered"] += 1

        report["locations"]["html"] += HTML_CATEGORY_ROW_3_FIELDS.format(locname, long_success, short_success)

    arb_msgs.sort()
    report["arbitrary_messages"]["total"] = len(arb_msgs)
    report["arbitrary_messages"]["html"] = HTML_CATEGORY_TABLE_HEADER_2_FIELDS.format("Arbitrary Message ID", "covered")
    for name, msg in arb_msgs:
        if msg != True:
            success = "uncovered"
        else:
            success = "covered"
            report["arbitrary_messages"]["covered"] += 1
        report["arbitrary_messages"]["html"] += HTML_CATEGORY_ROW_2_FIELDS.format(name, success)

    objects.sort()
    report["objects"]["html"] = HTML_CATEGORY_TABLE_HEADER_2_FIELDS.format("Object ID", "covered")
    for (obj_name, obj) in objects:
        if obj["descriptions"]:
            for j, desc in enumerate(obj["descriptions"]):
                report["objects"]["total"] += 1
                if desc != True:
                    success = "uncovered"
                else:
                    success = "covered"
                    report["objects"]["covered"] += 1
                report["objects"]["html"] += HTML_CATEGORY_ROW_2_FIELDS.format("%s[%d]" % (obj_name, j), success)

    hints.sort()
    report["hints"]["total"] = len(hints) * 2
    report["hints"]["html"] = HTML_CATEGORY_TABLE_HEADER_3_FIELDS.format("Hint ID", "question", "hint")
    for i, hint in enumerate(hints):
        hintname = hint["hint"]["name"]
        if hint["hint"]["question"] != True:
            question_success = "uncovered"
        else:
            question_success = "covered"
            report["hints"]["covered"] += 1
        if hint["hint"]["hint"] != True:
            hint_success = "uncovered"
        else:
            hint_success = "covered"
            report["hints"]["covered"] += 1
        report["hints"]["html"] += HTML_CATEGORY_ROW_3_FIELDS.format(name, question_success, hint_success)

    report["classes"]["total"] = len(classes)
    report["classes"]["html"] = HTML_CATEGORY_TABLE_HEADER_2_FIELDS.format("Adventurer Class #", "covered")
    for name, msg in enumerate(classes):
        if msg["message"] != True:
            success = "uncovered"
        else:
            success = "covered"
            report["classes"]["covered"] += 1
        report["classes"]["html"] += HTML_CATEGORY_ROW_2_FIELDS.format(msg["threshold"], success)

    report["turn_thresholds"]["total"] = len(turn_thresholds)
    report["turn_thresholds"]["html"] = HTML_CATEGORY_TABLE_HEADER_2_FIELDS.format("Turn Threshold", "covered")
    for name, msg in enumerate(turn_thresholds):
        if msg["message"] != True:
            success = "uncovered"
        else:
            success = "covered"
            report["turn_thresholds"]["covered"] += 1
        report["turn_thresholds"]["html"] += HTML_CATEGORY_ROW_2_FIELDS.format(msg["threshold"], success)

    report["obituaries"]["total"] = len(obituaries) * 2
    report["obituaries"]["html"] = HTML_CATEGORY_TABLE_HEADER_3_FIELDS.format("Obituary #", "query", "yes_response")
    for i, obit in enumerate(obituaries):
        if obit["query"] != True:
            query_success = "uncovered"
        else:
            query_success = "covered"
            report["obituaries"]["covered"] += 1
        if obit["yes_response"] != True:
            obit_success = "uncovered"
        else:
            obit_success = "covered"
            report["obituaries"]["covered"] += 1
        report["obituaries"]["html"] += HTML_CATEGORY_ROW_3_FIELDS.format(i, query_success, obit_success)

    actions.sort()
    report["actions"]["total"] = len(actions)
    report["actions"]["html"] = HTML_CATEGORY_TABLE_HEADER_2_FIELDS.format("Action ID", "covered")
    for name, action in actions:
        if action["message"] != True:
            success = "uncovered"
        else:
            success = "covered"
            report["actions"]["covered"] += 1
        report["actions"]["html"] += HTML_CATEGORY_ROW_2_FIELDS.format(name, success)

    report["specials"]["total"] = len(specials)
    report["specials"]["html"] = HTML_CATEGORY_TABLE_HEADER_2_FIELDS.format("Special ID", "covered")
    for name, special in specials:
        if special["message"] != True:
            success = "uncovered"
        else:
            success = "covered"
            report["specials"]["covered"] += 1
        report["specials"]["html"] += HTML_CATEGORY_ROW_2_FIELDS.format(name, success)

    # calculate percentages for each catagory and HTML for category tables
    categories_html = ""
    summary_html = ""
    summary_stdout = "adventure.yaml coverage rate:\n"
    for name, category in sorted(report.items()):
        if(category["total"] > 0):
            report[name]["percent"] = round((category["covered"] / float(category["total"])) * 100, 1)
            summary_stdout += STDOUT_REPORT_CATEGORY.format(**report[name])
            categories_html += HTML_CATEGORY_TABLE.format(id=name, rows=category["html"])
            summary_html += HTML_SUMMARY_ROW.format(**report[name])
        else:
            report[name]["percent"] = 100;

    # output some quick report stats
    print(summary_stdout)

    if len(sys.argv) > 1:
        html_output_path = sys.argv[1]
    else:
        html_output_path = DEFAULT_HTML_OUTPUT_PATH

    # render HTML report
    try:
        with open(HTML_TEMPLATE_PATH, "r") as f:
            # read in HTML template
            html_template = f.read()
    except IOError as e:
        print 'ERROR: reading HTML report template failed (%s)' % e.strerror
        exit(-1)

    # parse template with report and write it out
    try:
        with open(html_output_path, "w") as f:
            f.write(html_template.format(categories=categories_html, summary=summary_html))
    except IOError as e:
        print 'ERROR: writing HTML report failed (%s)' % e.strerror
