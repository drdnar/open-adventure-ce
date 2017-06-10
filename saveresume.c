#include <stdlib.h>

#include "advent.h"
#include "database.h"
#include "linenoise/linenoise.h"

#define VRSION	25	/* bump on save format change */

static void fSAVWDS(long*,long*,long*,long*,long*,long*,long*);
#define SAVWDS(W1,W2,W3,W4,W5,W6,W7) fSAVWDS(&W1,&W2,&W3,&W4,&W5,&W6,&W7)
static void fSAVARR(long*,long);
#define SAVARR(ARR,N) fSAVARR(ARR,N)
static void fSAVWRD(long,long*);
#define SAVWRD(OP,WORD) fSAVWRD(OP,&WORD)
static void fSAVEIO(long,long,long*);
#define SAVEIO(OP,IN,ARR) fSAVEIO(OP,IN,ARR)

/*  Suspend/resume I/O routines (SAVWDS, SAVARR, SAVWRD) */

static void fSAVWDS(long *W1, long *W2, long *W3, long *W4,
	     long *W5, long *W6, long *W7)
/* Write or read 7 variables.  See SAVWRD. */
{
    SAVWRD(0,(*W1));
    SAVWRD(0,(*W2));
    SAVWRD(0,(*W3));
    SAVWRD(0,(*W4));
    SAVWRD(0,(*W5));
    SAVWRD(0,(*W6));
    SAVWRD(0,(*W7));
}

static void fSAVARR(long arr[], long n)
/* Write or read an array of n words.  See SAVWRD. */
{
    long i;

    for (i=1; i<=n; i++) {
	SAVWRD(0,arr[i]);
    }
    return;
}

static void fSAVWRD(long op, long *pword) 
/*  If OP<0, start writing a file, using word to initialise encryption; save
 *  word in the file.  If OP>0, start reading a file; read the file to find
 *  the value with which to decrypt the rest.  In either case, if a file is
 *  already open, finish writing/reading it and don't start a new one.  If OP=0,
 *  read/write a single word.  Words are buffered in case that makes for more
 *  efficient disk use.  We also compute a simple checksum to catch elementary
 *  poking within the saved file.  When we finish reading/writing the file,
 *  we store zero into *PWORD if there's no checksum error, else nonzero. */
{
    static long buf[250], cksum = 0, h1, hash = 0, n = 0, state = 0;

    if (op != 0)
    {
	long ifvar = state; 
	switch (ifvar<0 ? -1 : (ifvar>0 ? 1 : 0)) 
	{ 
	case -1:
	case 1:
	    if (n == 250)SAVEIO(1,state > 0,buf);
	    n=MOD(n,250)+1;
	    if (state <= 0) {
		n--; buf[n]=cksum; n++;
		SAVEIO(1,false,buf);
	    }
	    n--; *pword=buf[n]-cksum; n++;
	    SAVEIO(-1,state > 0,buf);
	    state=0;
	    break;
	case 0:	/* FIXME: Huh? should be impossible */
	    state=op;
	    SAVEIO(0,state > 0,buf);
	    n=1;
	    if (state <= 0) {
		hash=MOD(*pword,1048576L);
		buf[0]=1234L*5678L-hash;
	    }
	    SAVEIO(1,true,buf);
	    hash=MOD(1234L*5678L-buf[0],1048576L);
	    cksum=buf[0];
	    return;
	}
    }
    if (state == 0)
	return;
    if (n == 250)
	SAVEIO(1,state > 0,buf);
    n=MOD(n,250)+1;
    h1=MOD(hash*1093L+221573L,1048576L);
    hash=MOD(h1*1093L+221573L,1048576L);
    h1=MOD(h1,1234)*765432+MOD(hash,123);
    n--;
    if (state > 0)
	*pword=buf[n]+h1;
    buf[n]=*pword-h1;
    n++;
    cksum=MOD(cksum*13+*pword,1000000000L);
}

static void fSAVEIO(long op, long in, long arr[]) 
/*  If OP=0, ask for a file name and open a file.  (If IN=true, the file is for
 *  input, else output.)  If OP>0, read/write ARR from/into the previously-opened
 *  file.  (ARR is a 250-integer array.)  If OP<0, finish reading/writing the
 *  file.  (Finishing writing can be a no-op if a "stop" statement does it
 *  automatically.  Finishing reading can be a no-op as long as a subsequent
 *  SAVEIO(0,false,X) will still work.) */
{
    static FILE *fp = NULL;
    char* name;

    switch (op < 0 ? -1 : (op > 0 ? 1 : 0)) 
    { 
    case -1:
	fclose(fp);
	break;
    case 0:
	while (fp == NULL) {
	    name = linenoise("File name: ");
	    fp = fopen(name,(in ? READ_MODE : WRITE_MODE));
	    if (fp == NULL)
		printf("Can't open file %s, try again.\n", name); 
	}
	linenoiseFree(name);
	break;
    case 1: 
	if (in)
	    IGNORE(fread(arr,sizeof(long),250,fp));
	else
	    IGNORE(fwrite(arr,sizeof(long),250,fp));
	break;
    }
}

int saveresume(FILE *input, bool resume)
/* Suspend and resume */
{
    int kk;
    long i;
    if (!resume) {
	/*  Suspend.  Offer to save things in a file, but charging
	 *  some points (so can't win by using saved games to retry
	 *  battles or to start over after learning zzword). */
	SPK=201;
	RSPEAK(260);
	if (!YES(input,200,54,54)) return(2012);
	game.saved=game.saved+5;
	kk= -1;
    }
    else
    {
	/*  Resume.  Read a suspended game back from a file. */
	kk=1;
	if (game.loc != 1 || game.abbrev[1] != 1) {
	    RSPEAK(268);
	    if (!YES(input,200,54,54)) return(2012);
	}
    }

    /*  Suspend vs resume cases are distinguished by the value of kk
     *  (-1 for suspend, +1 for resume). */

    /* 
     * FIXME: This is way more complicated than it needs to be in C.
     * What we ought to do is define a save-block structure that
     * includes a game state block and then use a single fread/fwrite
     * for I/O. All the SAV* functions can be scrapped.
     */

    DATIME(&i,&K);
    K=i+650*K;
    SAVWRD(kk,K);
    K=VRSION;
    SAVWRD(0,K);
    if (K != VRSION) {
	SETPRM(1,K/10,MOD(K,10));
	SETPRM(3,VRSION/10,MOD(VRSION,10));
	RSPEAK(269);
	return(2000);
    }
    /* Herewith are all the variables whose values can change during a game,
     * omitting a few (such as I, J) whose values between turns are
     * irrelevant and some whose values when a game is
     * suspended or resumed are guaranteed to match.  If unsure whether a value
     * needs to be saved, include it.  Overkill can't hurt.  Pad the last savwds
     * with junk variables to bring it up to 7 values. */
    SAVWDS(game.abbnum,game.blklin,game.bonus,game.clock1,game.clock2,game.closed,game.closng);
    SAVWDS(game.detail,game.dflag,game.dkill,game.dtotal,game.foobar,game.holdng,game.iwest);
    SAVWDS(game.knfloc,game.limit,K,game.lmwarn,game.loc,game.newloc,game.numdie);
    SAVWDS(K,game.oldlc2,game.oldloc,game.oldobj,game.panic,game.saved,game.setup);
    SAVWDS(SPK,game.tally,game.thresh,game.trndex,game.trnluz,game.turns,OBJTXT[OYSTER]);
    SAVWDS(K,WD1,WD1X,WD2,game.wzdark,game.zzword,OBJSND[BIRD]);
    SAVWDS(OBJTXT[SIGN],game.clshnt,game.novice,K,K,K,K);
    SAVARR(game.abbrev,LOCSIZ);
    SAVARR(game.atloc,LOCSIZ);
    SAVARR(game.dloc,NDWARVES);
    SAVARR(game.dseen,NDWARVES);
    SAVARR(game.fixed,NOBJECTS);
    SAVARR(game.hinted,HNTSIZ);
    SAVARR(game.hintlc,HNTSIZ);
    SAVARR(game.link,NOBJECTS*2);
    SAVARR(game.odloc,NDWARVES);
    SAVARR(game.place,NOBJECTS);
    SAVARR(game.prop,NOBJECTS);
    SAVWRD(kk,K);
    if (K != 0) {
	RSPEAK(270);
	exit(0);
    }
    K=NUL;
    game.zzword=RNDVOC(3,game.zzword);
    if (kk > 0) return(8);
    RSPEAK(266);
    exit(0);
}
