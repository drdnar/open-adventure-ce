#!/usr/bin/python3

import os
import yaml

import pprint

test_dir = "."
yaml_name = "../adventure.yaml"

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

    for locouter in locations:
        locname = locouter[0]
        loc = locouter[1]
        if loc["description"]["long"] != True:
            print("%s long description not covered!" % locname)
        if loc["description"]["short"] != True:
            print("location: %s short description not covered!" % locname)

    for name, msg in arb_msgs:
        if msg != True:
            print("arbitrary message: %s not covered!" % name)

    for (obj_name, obj) in objects:
        if obj["descriptions"]:
            for j, desc in enumerate(obj["descriptions"]):
                if desc != True:
                    print("object: %s desctiption #%d not covered!" % (obj_name, j))
