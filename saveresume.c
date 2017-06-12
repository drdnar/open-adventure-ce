#include <stdlib.h>
#include <string.h>

#include "advent.h"
#include "database.h"
#include "linenoise/linenoise.h"

/*
 * (ESR) This replaces  a bunch of particularly nasty FORTRAN-derived code;
 * see the history.adoc file in the source distribution for discussion.
 */

#define VRSION	26	/* bump on save format change */

/*
 * If you change the first three members, the resume function may not properly
 * reject saves from older versions.  Yes, this glues us to a hardware-
 * dependent length of long.  Later members can change, but bump the version
 * when you do that.
 */
struct save_t {
    long savetime;
    long mode;		/* not used, must be present for version detection */
    long version;
    struct game_t game;
    long bird;
    long bivalve;
};
struct save_t save;

int saveresume(FILE *input, bool resume)
/* Suspend and resume */
{
    long i, k;
    FILE *fp = NULL;
    char *name;
 
    if (!resume) {
	/*  Suspend.  Offer to save things in a file, but charging
	 *  some points (so can't win by using saved games to retry
	 *  battles or to start over after learning zzword). */
	RSPEAK(260);
	if (!YES(input,200,54,54)) return GO_CLEAROBJ;
	game.saved=game.saved+5;
    }
    else
    {
	/*  Resume.  Read a suspended game back from a file. */
	if (game.loc != 1 || game.abbrev[1] != 1) {
	    RSPEAK(268);
	    if (!YES(input,200,54,54)) return GO_CLEAROBJ;
	}
    }

    while (fp == NULL) {
	name = linenoise("\nFile name: ");
	if (name == NULL)
	    return GO_TOP;
	fp = fopen(name,(resume ? READ_MODE : WRITE_MODE));
	if (fp == NULL)
	    printf("Can't open file %s, try again.\n", name); 
    }
    linenoiseFree(name);
    
    DATIME(&i,&k);
    k=i+650*k;
    if (!resume)
    {
	save.savetime = k;
	save.mode = -1;
	save.version = VRSION;
	memcpy(&save.game, &game, sizeof(struct game_t));
	save.bird = OBJSND[BIRD];
	save.bivalve = OBJTXT[OYSTER];
	IGNORE(fwrite(&save, sizeof(struct save_t), 1, fp));
	fclose(fp);
	RSPEAK(266);
	exit(0);
    } else {
	IGNORE(fread(&save, sizeof(struct save_t), 1, fp));
	fclose(fp);
	if (save.version != VRSION) {
	    SETPRM(1,k/10,MOD(k,10));
	    SETPRM(3,VRSION/10,MOD(VRSION,10));
	    RSPEAK(269);
	} else {
	    memcpy(&game, &save.game, sizeof(struct game_t));
	    OBJSND[BIRD] = save.bird;
	    OBJTXT[OYSTER] = save.bivalve;
	    game.zzword=RNDVOC(3,game.zzword);
	}
	return GO_TOP;
    }
}

/* end */
