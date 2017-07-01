#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include "advent.h"
#include "linenoise/linenoise.h"
#include "dungeon.h"

struct game_t game = {
    .chloc = LOC_DEADEND12,
    .chloc2 = LOC_DEADEND13,
    .dloc[1] = LOC_KINGHALL,
    .dloc[2] = LOC_WESTBANK,
    .dloc[3] = LOC_Y2,
    .dloc[4] = LOC_ALIKE3,
    .dloc[5] = LOC_COMPLEX,
    .dloc[6] = LOC_DEADEND12,
    .abbnum = 5,
    .clock1 = WARNTIME,
    .clock2 = FLASHTIME,
    .blklin = true
};

FILE  *logfp = NULL, *rfp = NULL;
bool oldstyle = false;
bool editline = true;
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

    game.lcg_a = 1093;
    game.lcg_c = 221587;
    game.lcg_m = 1048576;
    srand(time(NULL));
    long seedval = (long)rand();
    set_seed(seedval);

    /*  Initialize game variables */
    initialise();

    make_zzword(game.zzword);
    game.newloc = LOC_START;
    game.loc = LOC_START;
    game.limit = GAMELIMIT;

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
