/*
 * 'cheat' is a tool for generating save game files to test states that ought
 * not happen. It leverages chunks of advent, mostly initialize() and
 * savefile(), so we know we're always outputing save files that advent
 * can import.
 *
 * Copyright (c) 1977, 2005 by Will Crowther and Don Woods
 * Copyright (c) 2017 by Eric S. Raymond
 * SPDX-License-Identifier: BSD-2-clause
 */
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "advent.h"

int main(int argc, char *argv[])
{
    int ch;
    char *savefilename = NULL;
    int version = 0;
    FILE *fp = NULL;

    // Initialize game variables
    initialise();

    /* we're generating a saved game, so saved once by default,
     * unless overridden with command-line options below.
     */
    game.saved = 1;

    /*  Options. */
    const char* opts = "d:l:s:t:v:o:";
    const char* usage = "Usage: %s [-d numdie] [-s numsaves] [-v version] -o savefilename \n"
                        "        -d number of deaths. Signed integer.\n"
                        "        -l lifetime of lamp in turns. Signed integer.\n"
                        "        -s number of saves. Signed integer.\n"
                        "        -t number of turns. Signed integer.\n"
                        "        -v version number of save format.\n"
                        "        -o required. File name of save game to write.\n";

    while ((ch = getopt(argc, argv, opts)) != EOF) {
        switch (ch) {
        case 'd':
            game.numdie = (turn_t)atoi(optarg);
            printf("cheat: game.numdie = %d\n", game.numdie);
            break;
        case 'l':
            game.limit = (turn_t)atoi(optarg);
            printf("cheat: game.limit = %d\n", game.limit);
            break;
        case 's':
            game.saved = (int)atoi(optarg);
            printf("cheat: game.saved = %d\n", game.saved);
            break;
        case 't':
            game.turns = (turn_t)atoi(optarg);
            printf("cheat: game.turns = %d\n", game.turns);
            break;
        case 'v':
            version = atoi(optarg);
            printf("cheat: version = %d\n", version);
            break;
        case 'o':
            savefilename = optarg;
            break;
        default:
            fprintf(stderr,
                    usage, argv[0]);
            exit(EXIT_FAILURE);
            break;
        }
    }

    // Save filename required; the point of cheat is to generate save file
    if (savefilename == NULL) {
        fprintf(stderr,
                usage, argv[0]);
        fprintf(stderr,
                "ERROR: filename required\n");
        exit(EXIT_FAILURE);
    }

    fp = fopen(savefilename, WRITE_MODE);
    if (fp == NULL) {
        fprintf(stderr,
                "Can't open file %s. Exiting.\n", savefilename);
        exit(EXIT_FAILURE);
    }

    savefile(fp, version);

    fclose(fp);

    printf("cheat: %s created.\n", savefilename);

    return EXIT_SUCCESS;
}
