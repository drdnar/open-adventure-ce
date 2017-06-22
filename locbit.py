#!/usr/bin/env python3
#
# Enhance adventure.yaml entries with explicit properties based on Section 9
# of adventure.text and the kludgy macro definitions in advent.h.
#
# This script is meant to be gotten right, used once, and then discarded.
# We'll leave a copy in the repository history for reference 
#
# When in doubt, make the code dumber and the data smarter.
#
# It bothers me that I don't know why FORCED is checking the fluid bit.
#
import sys, yaml

# This is the original location-attribute data from section 9 of adventure.text
# Bit indices are the first element of each tuple; the remaining numbers are
# indices of locations with that bit set.
section12 = (
    (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10),
    (0, 100, 115, 116, 126, 145, 146, 147, 148, 149, 150),
    (0, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160),
    (0, 161, 162, 163, 164, 165, 166, 167),
    (1, 24),
    (2, 1, 3, 4, 7, 38, 95, 113, 24, 168, 169),
    (3, 46, 47, 48, 54, 56, 58, 82, 85, 86),
    (3, 122, 123, 124, 125, 126, 127, 128, 129, 130),
    (4, 6, 145, 146, 147, 148, 149, 150, 151, 152),
    (4, 153, 154, 155, 156, 157, 158, 159, 160, 161),
    (4, 162, 163, 164, 165, 166, 42, 43, 44, 45),
    (4, 49, 50, 51, 52, 53, 55, 57, 80, 83),
    (4, 84, 87, 107, 112, 131, 132, 133, 134, 135),
    (4, 136, 137, 138, 139, 108),
    (11, 8),
    (12, 13),
    (13, 19),
    (14, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51),
    (14, 52, 53, 54, 55, 56, 80, 81, 82, 86, 87),
    (15, 99, 100, 101),
    (16, 108),
    (17, 6),
    (18, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154),
    (18, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164),
    (18, 165, 166),
    (19, 143),
    (20, 8, 15, 64, 109, 126),
)

# Names for attribute bits
attrnames = (
    "LIT",	# 0
    "OILY",	# 1
    "FLUID",	# 2
    "NOARRR",	# 3
    "NOBACK",	# 4
    "FORCED",	# 5	# New
    "FOREST",	# 6	# New
    "ABOVE",	# 7	# New
    "DEEP",	# 8	# New
    "",		# 9
    "HBASE",	# 10
    "HCAVE",	# 11
    "HBIRD",	# 12
    "HSNAKE",	# 13
    "HMAZE",	# 14
    "HDARK",	# 15
    "HWITT",	# 16
    "HCLIFF",	# 17
    "HWOODS",	# 18
    "HOGRE",	# 19
    "HJADE",	# 20
)

nlocs = 184
grate = 8
misthall = 15
sapphireloc = 167

# For reference from advent.h:
#
# define FORCED(LOC)	(COND[LOC] == 2)
# define FOREST(LOC)	((LOC) >= LOC_FOREST1 && (LOC) <= LOC_FOREST22)
#
#/*  The following two functions were added to fix a bug (game.clock1 decremented
# *  while in forest).  They should probably be replaced by using another
# *  "cond" bit.  For now, however, a quick fix...  OUTSID(LOC) is true if
# *  LOC is outside, INDEEP(LOC) is true if LOC is "deep" in the cave (hall
# *  of mists or deeper).  Note special kludges for "Foof!" locs. */
# define OUTSID(LOC)	((LOC) <= LOC_GRATE || FOREST(LOC) || (LOC) == PLAC[SAPPH] || (LOC) == LOC_FOOF2 || (LOC) == LOC_FOOF4)
# define INDEEP(LOC)	((LOC) >= LOC_MISTHALL && !OUTSID(LOC) && (LOC) != LOC_FOOF1)

def genline(loc):
    attrs = []
    name = locnames[loc]
    for props in section12:
        if loc in props[1:]:
            if props[0] not in attrs:
                attrs.append(props[0])
    # Adod new attributes.  These are computed the same way as the
    # INDEEP(), OUTSID(), and FORCED macros in advent.h.
    # FORCED is on only if COND == 2
    if attrs == [2]:
        attrs.append(5)	# FORCED
    if "FOREST" in name:
        attrs.append(6)	# FOREST
    # 167 is the sapphire's start location
    if loc in range(1, grate+1) or name in ("FOOF2", "FOOF4") or name == sapphireloc:
        attrs.append(7)	# ABOVE
    if not loc in range(0, misthall+1) and name != "FOOF1" and 6 not in attrs:
        attrs.append(8)	# DEEP
    names = str([attrnames[n] for n in attrs]).replace("'", "")
    return "    conditions: %s\n" % (names,)

if __name__ == "__main__":
    with open("adventure.yaml", "r") as fp:
        db = yaml.load(fp)
        fp.seek(0)
        locnames = [el[0] for el in db["locations"]]
        ln = -1
        while True:
            line = fp.readline()
            if not line:
                break
            if line.startswith("- LOC"):
                if ln > -1:
                    sys.stdout.write(genline(ln))
                ln += 1
            sys.stdout.write(line)

# end
