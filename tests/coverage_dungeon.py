#!/usr/bin/env python

# This is the open-adventure dungeon text coverage report generator. It
# consumes a YAML description of the dungeon and determines whether the
# various strings contained are present within the test check files.
#
# The default HTML output is appropriate for use with Gitlab CI.
# You can override it with a command-line argument.
#
# The DANGLING list is for actions that should be considered always found
# even if the checkfile search doesn't find them. Typically this will because
# they emit a templated message that can't be regression-tested by equality.

import os
import sys
import yaml
import re

TEST_DIR = "."
YAML_PATH = "../adventure.yaml"
HTML_TEMPLATE_PATH = "../templates/coverage_dungeon.html.tpl"
DEFAULT_HTML_OUTPUT_PATH = "../coverage/adventure.yaml.html"
DANGLING = ["ACT_VERSION"]

STDOUT_REPORT_CATEGORY = "  {name:.<19}: {percent:5.1f}% covered ({covered} of {total})\n"

HTML_SUMMARY_ROW = '''
    <tr>
        <td class="headerItem"><a href="#{name}">{name}:</a></td>
        <td class="headerCovTableEntry">{total}</td>
        <td class="headerCovTableEntry">{covered}</td>
        <td class="headerCovTableEntry">{percent:.1f}%</td>
    </tr>
'''

HTML_CATEGORY_SECTION = '''
    <tr id="{id}"></tr>
    {rows}
    <tr>
        <td>&nbsp;</td>
    </tr>
'''

HTML_CATEGORY_HEADER = '''
    <tr>
        <td class="tableHead" width="60%" colspan="{colspan}">{label}</td>
        {cells}
    </tr>
'''

HTML_CATEGORY_HEADER_CELL = '<td class="tableHead" width="15%">{}</td>\n'

HTML_CATEGORY_COVERAGE_CELL = '<td class="{}">&nbsp;</td>\n'

HTML_CATEGORY_ROW = '''
    <tr>
        <td class="coverFile" colspan="{colspan}">{id}</td>
        {cells}
    </tr>
'''

def search(needle, haystack):
    # Search for needle in haystack, first escaping needle for regex, then
    # replacing %s, %d, etc. with regex wildcards, so the variable messages
    # within the dungeon definition will actually match
        
    if needle == None or needle == "" or needle == "NO_MESSAGE":
        # if needle is empty, assume we're going to find an empty string
        return True
    
    needle_san = re.escape(needle) \
             .replace("\\n", "\n") \
             .replace("\\t", "\t") \
             .replace("\%S", ".*") \
             .replace("\%s", ".*") \
             .replace("\%d", ".*") \
             .replace("\%V", ".*")

    return re.search(needle_san, haystack)

def obj_coverage(objects, text, report):
    # objects have multiple descriptions based on state
    for i, objouter in enumerate(objects):
        (obj_name, obj) = objouter
        if obj["descriptions"]:
            for j, desc in enumerate(obj["descriptions"]):
                name = "{}[{}]".format(obj_name, j)
                if name not in report["messages"]:
                    report["messages"][name] = {"covered" : False}
                    report["total"] += 1
                if report["messages"][name]["covered"] != True and search(desc, text):
                    report["messages"][name]["covered"] = True
                    report["covered"] += 1

def loc_coverage(locations, text, report):
    # locations have a long and a short description, that each have to
    # be checked seperately
    for name, loc in locations:
        desc = loc["description"]
        if name not in report["messages"]:
            report["messages"][name] = {"long" : False, "short": False}
            report["total"] += 2
        if report["messages"][name]["long"] != True and search(desc["long"], text):
            report["messages"][name]["long"] = True
            report["covered"] += 1
        if report["messages"][name]["short"] != True and search(desc["short"], text):
            report["messages"][name]["short"] = True
            report["covered"] += 1

def hint_coverage(obituaries, text, report):
    # hints have a "question" where the hint is offered, followed
    # by the actual hint if the player requests it
    for i, hintouter in enumerate(obituaries):
        hint = hintouter["hint"]
        name = hint["name"]
        if name not in report["messages"]:
            report["messages"][name] = {"question" : False, "hint": False}
            report["total"] += 2
        if report["messages"][name]["question"] != True and search(hint["question"], text):
            report["messages"][name]["question"] = True
            report["covered"] += 1
        if report["messages"][name]["hint"] != True and search(hint["hint"], text):
            report["messages"][name]["hint"] = True
            report["covered"] += 1

def obit_coverage(obituaries, text, report):
    # obituaries have a "query" where it asks the player for a resurrection,
    # followed by a snarky comment if the player says yes
    for name, obit in enumerate(obituaries):
        if name not in report["messages"]:
            report["messages"][name] = {"query" : False, "yes_response": False}
            report["total"] += 2
        if report["messages"][name]["query"] != True and search(obit["query"], text):
            report["messages"][name]["query"] = True
            report["covered"] += 1
        if report["messages"][name]["yes_response"] != True and search(obit["yes_response"], text):
            report["messages"][name]["yes_response"] = True
            report["covered"] += 1

def threshold_coverage(classes, text, report):
    # works for class thresholds and turn threshold, which have a "message"
    # property
    for name, item in enumerate(classes):
        if name not in report["messages"]:
            report["messages"][name] = {"covered" : "False"}
            report["total"] += 1
        if report["messages"][name]["covered"] != True and search(item["message"], text):
            report["messages"][name]["covered"] = True
            report["covered"] += 1

def arb_coverage(arb_msgs, text, report):
    for name, message in arb_msgs:
        if name not in report["messages"]:
            report["messages"][name] = {"covered" : False}
            report["total"] += 1
        if report["messages"][name]["covered"] != True and search(message, text):
            report["messages"][name]["covered"] = True
            report["covered"] += 1

def actions_coverage(items, text, report):
    # works for actions
    for name, item in items:
        if name not in report["messages"]:
            report["messages"][name] = {"covered" : False}
            report["total"] += 1
        if report["messages"][name]["covered"] != True and (search(item["message"], text) or name in DANGLING):
            report["messages"][name]["covered"] = True
            report["covered"] += 1

def coverage_report(db, check_file_contents):
    # Create report for each catagory, including total items,  number of items
    # covered, and a list of the covered messages
    report = {}
    for name in db.keys():
        # initialize each catagory
        report[name] = {
            "name" : name, # convenience for string formatting
            "total" : 0,
            "covered" : 0,
            "messages" : {}
        }

    # search for each message in every test check file
    for chk in check_file_contents:
        arb_coverage(db["arbitrary_messages"], chk, report["arbitrary_messages"])
        hint_coverage(db["hints"], chk, report["hints"])
        loc_coverage(db["locations"], chk, report["locations"])
        obit_coverage(db["obituaries"], chk, report["obituaries"])
        obj_coverage(db["objects"], chk, report["objects"])
        actions_coverage(db["actions"], chk, report["actions"])
        threshold_coverage(db["classes"], chk, report["classes"])
        threshold_coverage(db["turn_thresholds"], chk, report["turn_thresholds"])

    return report

if __name__ == "__main__":
    # load DB
    try:
        with open(YAML_PATH, "r") as f:
            db = yaml.load(f)
    except IOError as e:
        print('ERROR: could not load {} ({}})'.format(YAML_PATH, e.strerror))
        exit(-1)

    # get contents of all the check files
    check_file_contents = []
    for filename in os.listdir(TEST_DIR):
        if filename.endswith(".chk"):
            with open(filename, "r") as f:
                check_file_contents.append(f.read())

    # run coverage analysis report on dungeon database
    report = coverage_report(db, check_file_contents)

    # render report output
    categories_html = ""
    summary_html = ""
    summary_stdout = "adventure.yaml coverage rate:\n"
    for name, category in sorted(report.items()):
        # ignore categories with zero entries
        if category["total"] > 0:
            # Calculate percent coverage
            category["percent"] = (category["covered"] / float(category["total"])) * 100

            # render section header
            cat_messages = list(category["messages"].items())
            cat_keys = cat_messages[0][1].keys()
            headers_html = ""
            colspan = 10 - len(cat_keys)
            for key in cat_keys:
                headers_html += HTML_CATEGORY_HEADER_CELL.format(key)
            category_html = HTML_CATEGORY_HEADER.format(colspan=colspan, label=category["name"], cells=headers_html)

            # render message coverage row
            for message_id, covered in cat_messages:
                category_html_row = ""
                for key, value in covered.items():
                    category_html_row += HTML_CATEGORY_COVERAGE_CELL.format("uncovered" if value != True else "covered")
                category_html += HTML_CATEGORY_ROW.format(id=message_id,colspan=colspan, cells=category_html_row)
            categories_html += HTML_CATEGORY_SECTION.format(id=name, rows=category_html)

            # render category summaries
            summary_stdout += STDOUT_REPORT_CATEGORY.format(**category)
            summary_html += HTML_SUMMARY_ROW.format(**category)

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
        print('ERROR: reading HTML report template failed ({})'.format(e.strerror))
        exit(-1)

    # parse template with report and write it out
    try:
        with open(html_output_path, "w") as f:
            f.write(html_template.format(categories=categories_html, summary=summary_html))
    except IOError as e:
        print('ERROR: writing HTML report failed ({})'.format(e.strerror))
