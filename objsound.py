#!/usr/bin/env python3
#
# Enhance adventure.yaml entries with explicit object-sound properties
# based on Section 13 of adventure.text.
#
# When in doubt, make the code dumber and the data smarter.
#
import sys, yaml

# This is the original sound-attribute data from section 13 of adventure.text
section13 = (
    (8, 	3,	-1),
    (11,	2,	-1),
    (13,	-1,	1),
    (14,	1,	-1),
    (15,	2,	-1),
    (16,	-1,	1),
    (24,	6,	-1),
    (31,	4,	-1),
    (33,	3,	-1),
    (36,	-1,	1),
    (38,	-1,	1),
    (41,	1,	-1),
    (47,	-1,	1),
    (48,	-1,	1),
    (49,	-1,	1),
)


def genline(ml):
    attrs = {}
    appendme = ""
    return out

if __name__ == "__main__":
    with open("adventure.yaml", "r") as fp:
        db = yaml.load(fp)
        fp.seek(0)
        objnames = [el[0] for el in db["object_descriptions"]]
        objnum = 0
        counter = -99
        soundtrap = texttrap = None
        while True:
            line = fp.readline()
            if not line:
                break
            if line.startswith("- OBJ"):
                counter = -99;
                soundtrap = texttrap = None
                for (obj, sound, text) in section13:
                    if obj == objnum:
                        counter = -2	# Skip inventory and longs markup line
                        soundtrap = None if (sound == -1) else sound 
                        texttrap = None if (text == -1) else text 
                        break
                objnum += 1
            sys.stdout.write(line)
            if soundtrap is not None and counter == soundtrap:
                sys.stdout.write("    sounds:\n")
            if texttrap is not None and counter == texttrap:
                sys.stdout.write("    texts:\n")
            counter += 1

# end
