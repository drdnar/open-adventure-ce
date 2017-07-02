/*
 * 'cheat' is a tool for generating save game files to test states that ought
 * not happen. It leverages chunks of advent, mostly initialize() and
 * savefile(), so we know we're always outputing save files that advent
 * can import.
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
    long numdie = 0;
    long saved = 1;
    long version = 0;
    FILE *fp = NULL;

    /*  Options. */
    const char* opts = "d:s:v:o:";
    const char* usage = "Usage: %s [-d numdie] [-s numsaves] [-v version] -o savefilename \n"
                        "        -d number of deaths. Signed integer value.\n"
                        "        -s number of saves. Signed integer value.\n"
                        "        -v version number of save format.\n"
                        "        -o required. File name of save game to write.\n";

    while ((ch = getopt(argc, argv, opts)) != EOF) {
        switch (ch) {
        case 'd':
            numdie = (long)atoi(optarg);
            break;
        case 's':
            saved = (long)atoi(optarg);
            break;
        case 'v':
            version = (long)atoi(optarg);;
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

    // Initialize game variables
    initialise();

    // apply cheats
    game.numdie = numdie;
    game.saved = saved;

    fp = fopen(savefilename, WRITE_MODE);
    if (fp == NULL) {
        fprintf(stderr,
                "Can't open file %s. Exiting.\n", savefilename);
        exit(EXIT_FAILURE);
    }

    savefile(fp, version);

    printf("cheat: %s created.\n", savefilename);

    return EXIT_SUCCESS;
}
