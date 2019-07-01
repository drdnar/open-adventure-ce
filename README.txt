


                             Open Adventure CE 1.8.0

                                    a port of

                         The Colossal Cave Adventure 2.5

                                   1 July 2019
                                    Dr. D'nar



================================================================================

    The Colossal Cave Adventure is an adventure game, featuring a maze-like cave
with treasure and monsters.  The game draws elements from fantasy literature
and role-playing games like Dungeons and Dragons.  In Adventure, you will dodge
angry dwarves, fight a dragon, and find your way through a maze (or die trying).
Puzzles in the game range from easy to quite hard, and you will not get a
perfect score in just a single day of playing.  The original Colossal Cave
Adventure game was written in 1975 through 1977; this game is one of the oldest
classic computer games you can play.

    The Colossal Cave Adventure is the original interactive fiction game, the
first text adventure game, and inspiration for many games published in the 80's
through early 90's.  Will Crowther based the initial cave on the actual Colossal
Cave in Kentucky.  Don Woods significantly expanded the game.  In 2017 Eric S.
Raymond began rewriting the awkward original FORTRAN code into something more
modern and portable, a project he dubbed Open Adventure; Jason Ninneman, Peje
Nilsson, Petr Voropaev, and Aaron Traas subsequently also jointed the project.

    Open Adventure CE is a port of Open Adventure
http://www.catb.org/~esr/open-adventure to the TI-84 Plus CE.  For the French
market, of course, the TI-83 Premium CE and Edition Python are also naturally
supported.



======= Usage ==================================================================


    Adventure is played using textual commands that resemble English.  However,
the original system Adventure was programmed on has some odd limitations, some
of which are present still in this calculator port.

    In particular, only the first five characters of word are examined.  This
means that you only need to enter the first five characters of any word.  For
example, the word "bottle" has six letters in it, so you can abbreviate it to
"bottl".  Unfortunately, this also means that "northeast" is the same as "north"
so you have to type "ne" instead to go northeast.

    The input parser was the first attempt ever at natural-language parsing in a
game and has some known deficiencies.  Thus you get anomalies like "eat
building" interpreted as a command to move to the building.  These should not be
reported as bugs; instead, consider them historical curiosities.

    NOTA BENE: YOU CANNOT quit the main game just pressing MODE, DEL, or CLEAR.
You MUST type "quit" or "save" to exit.  (Alternatively, press ON, then CLEAR).


------- Installation -----------------------------------------------------------

    The standard LoadLib libraries GRAPHX, FILEIOC, and FONTLIBC are required,
available at https://github.com/CE-Programming/libraries .  FONTLIBC is a new
addition to the libraries package; please update the libraries on your device.

    You need the Times and Dr. Sans font packs, also included.  Two versions of
the Times font pack are included.  The Times.8xv font pack contains several font
sizes unused by Adventure, so I've also included a Times_mini.8xv file, which
only contains the font sizes used by Adventure; it saves about 30 K.

    Send both ADVENT.8xp and ADVENT_data.8xv to your calculator.  The program
ADVENT is the game itself, while the appvar AdvenDat is the game's dungeon
data file.  While changes to AdvenDat should be rare, different versions of
the file are not compatible.  If the game complains that the dungeon file is
missing, make sure you have the right version of AdvenDat.

    You will want to keep the AdvenDat appvar archived.  You will otherwise run
low on RAM.


------- Command Shortcuts ------------------------------------------------------

    The following standard command shortcuts are available:

  - i for inventory.
  - g for get or take.
  - l for look; or alternatively, x for examine (does the same thing).
  - u and d for up and down, respectively.
  - n, e, w, and s for north, east, west, and south, respectively.
  - ne, nw, se, sw for northeast, northwest, southeast, and southwest.
    NOTA BENE: Because the text parser only looks at the first five letters of
    each word, those are, in fact, the ONLY way to enter those directions!
  - y for yes and n for no.
  - z to do nothing for a turn (this may still cause other events to proceed).


------- Command History --------------------------------------------------------

    While playing, you can press UP to access your command history.  Press ENTER
to paste the selected command.  Press CLEAR to return to editing.  DEL is
backspace.


------- Saving and Resuming ----------------------------------------------------

    To save a game currently in progress, type "save".  You will be prompted for
a file name to give the save.  If you previously resumed the game, then it will
default to using the same name.  If the save file was archived, it will be
rearchived for you.

    You can resume a game using the Resume option on the main menu.
Simply use the cursor to select a save file, and then press ENTER to resume that
game.  You can delete a file directly from the Resume menu using the DEL key.
Archived files on the Resume menu have a dot to the left of their name.  You can
archive/unarchive a save file at the Resume screen using the STAT key.

    The standard "resume" command also still works.


------- Battery Saving ---------------------------------------------------------

    To prevent ruining your battery, Adventure will automatically quit after
several minutes of no key presses.  If you resumed a previously saved game,
Adventure will automatically overwrite that save before quitting.  If you're
playing a new game, you can manually set the save name using the "filename"
command.  However, if you do not set a save name, then the game will be
discarded.

    You can quit the game instantly at any time by pressing ON, then CLEAR.
NOTHING WILL BE SAVED



======= Change Log for Open Adventure CE =======================================

1.8.0 on 1 July 2019
 - Initial release
1.8
 - PC version from this CE version is derived.

 

======= About Open Adventure ===================================================

    The homepage for Open Adventure CE is
https://github.com/drdnar/open-adventure-ce .  You can check there for the
lastest updates, releases, and to file bug reports.

    Eric S. Raymond's home for this project is at
http://www.catb.org/~esr/open-adventure .  You can go there for tarball
downloads and other resources related to his original PC/POSIX version of this
game.

    This code is a forward-port of the Crowther/Woods Adventure 2.5 from 1995,
last version in the main line of Colossal Cave Adventure development written by
the original authors.  The authors have given permission and encouragement for
Open Adventure; it obsolesces all the 350-point versions and previous 2.x
(430-point) ports.

    Additional information about the history of the Colossal Cave Adventure and
Open Adventure is available at Eric S. Raymond's site.

    This project is called "Open Adventure" because it's not at all clear how to
number Adventure past 2.5 without misleading or causing collisions or both.  See
Raymond's history file for discussion.  The original 6-character name ADVENT on
the PDP-10 has been kept for the retro feel.



======= License ================================================================

                                   BSD LICENSE

Copyright (c) 1977, 2005 by Will Crowther and Don Woods
Copyright (c) 2017 by Eric S. Raymond
Copyright (c) 2019 by Dr. D'nar

    Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.




















    Oh, you're still here?  Well, if you're looking for help . . .

======= Non-spoiler Hints ======================================================

Say the words you see.  They can have interesting effects.

Reading is fundamental.

Yes, the fissure in the Hall of Mists can be bridged.  By magic.

"Free bird": It's more than an epic guitar solo. Do it twice!

There is a legend that if you drink the blood of a dragon, you will be able to
understand the speech of birds.

That vending machine?  It would be better off dead.

Ogres laugh at humans, but for some reason dwarves frighten them badly.

When rust is a problem, oil can be helpful.

A lucky rabbit's foot might help you keep your footing.

The troll might go away when you are no longer unbearable.
