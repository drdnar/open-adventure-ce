#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include "advent.h"
#include "dungeon.h"

FILE  *logfp = NULL, *rfp = NULL;
bool oldstyle = false;
bool prompt = true;

int main(int argc, char *argv[])
{
    int ch;
    char *savefilename = NULL;
    long numdie = 0;
    long saved = 1;
    long version = 0;

    /*  Options. */
    const char* opts = "d:s:v:o:";
    const char* usage = "Usage: %s [-d numdie] [-s numsaves] [-v version] -o savefilename \n";
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
            fprintf(stderr,
                    "        -d number of deaths. Signed integer value.'\n");
            fprintf(stderr,
                    "        -s number of saves. Signed integer value.\n");
            fprintf(stderr,
                    "        -v version number of save format.\n");
            fprintf(stderr,
                    "        -o file name of save game to write.\n");
            exit(EXIT_FAILURE);
            break;
        }
    }

    if (savefilename == NULL) {
        fprintf(stderr,
                usage, argv[0]);
        fprintf(stderr,
                "ERROR: filename required\n");
        exit(EXIT_FAILURE);
    }

    FILE *fp = NULL;

    /*  Initialize game variables */
    initialise();

    make_zzword(game.zzword);

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
    return 0;
}
