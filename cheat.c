#define DEFINE_GLOBALS_FROM_INCLUDES
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

long NUL;
long BACK;
long LOOK;
long CAVE;
long FIND;
long INVENT;
long SAY;
long ENTER;
long STREAM;
long DPRSSN;
long ENTRNC;
long LOCK;
long THROW;

FILE  *logfp = NULL, *rfp = NULL;
bool oldstyle = false;
bool editline = true;
bool prompt = true;

int main(int argc, char *argv[])
{
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
    game.numdie = -1000;
    game.saved = 1;
    
    fp = fopen("cheat_numdie.adv", WRITE_MODE);
    if (fp == NULL)
    {
        printf("Can't open file. Exiting.\n");
        exit(0);
    }        

    savefile(fp);
    printf("cheat: tests/cheat_numdie.adv created.\n");
    return 0;
}
