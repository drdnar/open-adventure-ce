#define DEFINE_GLOBALS_FROM_INCLUDES
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include "advent.h"
#include "database.h"
#include "linenoise/linenoise.h"
#include "newdb.h"

struct game_t game;

long LNLENG, LNPOSN;
char rawbuf[LINESIZE], INLINE[LINESIZE + 1];

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

    /*  Options. */
    const char* opts = "d:s:o:";
    const char* usage = "Usage: %s [-d numdie] [-s numsaves] -o savefilename \n";
    while ((ch = getopt(argc, argv, opts)) != EOF) {
        switch (ch) {
        case 'd':
            numdie = (long)atoi(optarg);
            break;
        case 's':
            saved = (long)atoi(optarg);
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
                    "        -o file name of save game to write.\n");
            exit(-1);
            break;
        }
    }
    
    if (savefilename == NULL)
    {
        fprintf(stderr,
                usage, argv[0]);
        fprintf(stderr,
                "ERROR: filename required\n");
        exit(-1);
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

    game.zzword = rndvoc(3, 0);
    game.newloc = LOC_START;
    game.loc = LOC_START;
    game.limit = GAMELIMIT;
    
    // apply cheats
    game.numdie = numdie;
    game.saved = saved;
    
    fp = fopen(savefilename, WRITE_MODE);
    if (fp == NULL)
    {
        printf("Can't open file %s. Exiting.\n", savefilename);
        exit(-1);
    }        

    savefile(fp);
    printf("cheat: %s created.\n", savefilename);
    return 0;
}
