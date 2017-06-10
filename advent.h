#include <stdio.h>
#include <stdbool.h>

#include "sizes.h"

#define LINESIZE	100
#define NDWARVES	6
#define PIRATE		NDWARVES	/* must be NDWARVES-1 when zero-origin */
#define DALTLC		18		/* alternate dwarf location; low room */
#define MINTRS		50
#define MAXTRS		79
#define MAXPARMS	25
#define INVLIMIT	7
#define INTRANSITIVE	-1	/* illegal object number */

typedef struct lcg_state
{
  unsigned long a, c, m, x;
} lcg_state;

typedef long token_t;	/* word token - someday this will be char[TOKLEN+1] */
typedef long vocab_t;	/* index into a vocabulary array */

struct game_t {
    long abbnum;
    long blklin;
    long bonus;
    long chloc;
    long chloc2;
    long clock1;
    long clock2;
    long clshnt;
    long closed;
    long closng;
    long conds;
    long detail;
    long dflag;
    long dkill;
    long dtotal;
    long foobar;
    long holdng;
    long iwest;
    long knfloc;
    long limit;
    long lmwarn;
    long loc;
    long newloc;
    long novice;
    long numdie;
    long oldloc;
    long oldlc2;
    long oldobj;
    long panic;
    long saved;
    long setup;
    long tally;
    long thresh;
    long trndex;
    long trnluz;
    long turns;
    long wzdark;
    long zzword;
    long abbrev[LOCSIZ+1];
    long atloc[LOCSIZ+1];
    long dseen[NDWARVES+1];
    long dloc[NDWARVES+1];
    long odloc[NDWARVES+1];
    long fixed[NOBJECTS+1];
    long link[NOBJECTS*2 + 1];
    long place[NOBJECTS+1];
    long hinted[HNTSIZ+1];
    long hintlc[HNTSIZ+1];
    long prop[NOBJECTS+1];
};

extern struct game_t game;

extern long LNLENG, LNPOSN, PARMS[];
extern char rawbuf[LINESIZE], INLINE[LINESIZE+1];
extern const char ascii_to_advent[];
extern const char advent_to_ascii[];
extern FILE *logfp;
extern bool oldstyle;
extern lcg_state lcgstate;

/* b is not needed for POSIX but harmless */
#define READ_MODE "rb"
#define WRITE_MODE "wb"

extern void SPEAK(vocab_t);
extern void PSPEAK(vocab_t,int);
extern void RSPEAK(vocab_t);
extern void SETPRM(long,long,long);
extern bool GETIN(FILE *,token_t*,token_t*,token_t*,token_t*);
extern long YES(FILE *,vocab_t,vocab_t,vocab_t);
extern long GETTXT(bool,bool,bool);
extern token_t MAKEWD(long);
extern void PUTTXT(token_t,long*,long);
extern void SHFTXT(long,long);
extern void TYPE0(void);

extern void fSAVWDS(long*,long*,long*,long*,long*,long*,long*);
#define SAVWDS(W1,W2,W3,W4,W5,W6,W7) fSAVWDS(&W1,&W2,&W3,&W4,&W5,&W6,&W7)
extern void fSAVARR(long*,long);
#define SAVARR(ARR,N) fSAVARR(ARR,N)
extern void fSAVWRD(long,long*);
#define SAVWRD(OP,WORD) fSAVWRD(OP,&WORD)

extern long VOCAB(long,long);
extern void DSTROY(long);
extern void JUGGLE(long);
extern void MOVE(long,long);
extern long PUT(long,long,long);
extern void CARRY(long,long);
extern void DROP(long,long);
extern long ATDWRF(long);
extern long SETBIT(long);
extern bool TSTBIT(long,int);
extern long RNDVOC(long,long);
extern void BUG(long) __attribute__((noreturn));
extern void MAPLIN(FILE *);
extern void TYPE(void);

extern void fSAVEIO(long,long,long*);
#define SAVEIO(OP,IN,ARR) fSAVEIO(OP,IN,ARR)
extern void DATIME(long*, long*);

extern long MOD(long,long);

extern void set_seed(long);
extern unsigned long get_next_lcg_value(void);
extern long randrange(long);
extern void score(long);

/*  Statement functions
 *
 *  AT(OBJ)	= true if on either side of two-placed object
 *  CNDBIT(L,N) = true if COND(L) has bit n set (bit 0 is units bit)
 *  DARK(DUMMY) = true if location "LOC" is dark
 *  FORCED(LOC) = true if LOC moves without asking for input (COND=2)
 *  FOREST(LOC)  = true if LOC is part of the forest
 *  GSTONE(OBJ)  = true if OBJ is a gemstone
 *  HERE(OBJ)	= true if the OBJ is at "LOC" (or is being carried)
 *  LIQ(DUMMY)	= object number of liquid in bottle
 *  LIQLOC(LOC) = object number of liquid (if any) at LOC
 *  PCT(N)       = true N% of the time (N integer from 0 to 100)
 *  TOTING(OBJ) = true if the OBJ is being carried */

#define TOTING(OBJ)	(game.place[OBJ] == -1)
#define AT(OBJ) (game.place[OBJ] == game.loc || game.fixed[OBJ] == game.loc)
#define HERE(OBJ)	(AT(OBJ) || TOTING(OBJ))
#define LIQ2(PBOTL)	((1-(PBOTL))*WATER+((PBOTL)/2)*(WATER+OIL))
#define LIQ(DUMMY)	(LIQ2(game.prop[BOTTLE]<0 ? -1-game.prop[BOTTLE] : game.prop[BOTTLE]))
#define LIQLOC(LOC)	(LIQ2((MOD(COND[LOC]/2*2,8)-5)*MOD(COND[LOC]/4,2)+1))
#define CNDBIT(L,N)	(TSTBIT(COND[L],N))
#define FORCED(LOC)	(COND[LOC] == 2)
#define DARK(DUMMY)	((!CNDBIT(game.loc,0)) && (game.prop[LAMP] == 0 || !HERE(LAMP)))
#define PCT(N)	(randrange(100) < (N))
#define GSTONE(OBJ)	((OBJ) == EMRALD || (OBJ) == RUBY || (OBJ) == AMBER || (OBJ) == SAPPH)
#define FOREST(LOC)	((LOC) >= 145 && (LOC) <= 166)
#define VOCWRD(LETTRS,SECT)	(VOCAB(MAKEWD(LETTRS),SECT))

/*  The following two functions were added to fix a bug (game.clock1 decremented
 *  while in forest).  They should probably be replaced by using another
 *  "cond" bit.  For now, however, a quick fix...  OUTSID(LOC) is true if
 *  LOC is outside, INDEEP(LOC) is true if LOC is "deep" in the cave (hall
 *  of mists or deeper).  Note special kludges for "Foof!" locs. */

#define OUTSID(LOC)	((LOC) <= 8 || FOREST(LOC) || (LOC) == PLAC[SAPPH] || (LOC) == 180 || (LOC) == 182)
#define INDEEP(LOC)	((LOC) >= 15 && !OUTSID(LOC) && (LOC) != 179)

/* vocabulary items */ 
extern long AMBER, ATTACK, AXE, BACK, BATTER, BEAR,
   BIRD, BLOOD, BOTTLE, CAGE, CAVE, CAVITY, CHAIN, CHASM, CHEST,
   CLAM, COINS, DOOR, DPRSSN, DRAGON, DWARF, EGGS,
   EMRALD, ENTER, ENTRNC, FIND, FISSUR, FOOD, GRATE, HINT, INVENT,
   JADE, KEYS, KNIFE, LAMP, LOCK, LOOK, MAGZIN, MESSAG, MIRROR, NUGGET, NUL,
   OGRE, OIL, OYSTER, PANIC, PEARL, PILLOW, PLANT, PLANT2, PYRAM,
   RESER, ROD, ROD2, RUBY, RUG, SAPPH, SAY, SIGN, SNAKE,
   STEPS, STICK, STREAM, THROW, TRIDNT, TROLL, TROLL2,
   URN, VASE, VEND, VOLCAN, WATER;
/* everything else */
extern long K, SPK, WD1, WD1X, WD2, WD2X;

enum speechpart {unknown, intransitive, transitive};
