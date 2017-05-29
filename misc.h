#include <stdio.h>
#include <stdbool.h>

/* b is not needed for POSIX but harmless */
#define READ_MODE "rb"
#define WRITE_MODE "wb"

extern void fSPEAK(long);
#define SPEAK(N) fSPEAK(N)
extern void fPSPEAK(long,long);
#define PSPEAK(MSG,SKIP) fPSPEAK(MSG,SKIP)
extern void fRSPEAK(long);
#define RSPEAK(I) fRSPEAK(I)
extern void fSETPRM(long,long,long);
#define SETPRM(FIRST,P1,P2) fSETPRM(FIRST,P1,P2)
extern bool fGETIN(FILE *,long*,long*,long*,long*);
#define GETIN(input,WORD1,WORD1X,WORD2,WORD2X) fGETIN(input,&WORD1,&WORD1X,&WORD2,&WORD2X)
extern long fYES(FILE *,long,long,long);
#define YES(input,X,Y,Z) fYES(input,X,Y,Z)
extern long fGETNUM(FILE *);
#define GETNUM(K) fGETNUM(K)
extern long fGETTXT(long,long,long);
#define GETTXT(SKIP,ONEWRD,UPPER) fGETTXT(SKIP,ONEWRD,UPPER)
extern long fMAKEWD(long);
#define MAKEWD(LETTRS) fMAKEWD(LETTRS)
extern void fPUTTXT(long,long*,long);
#define PUTTXT(WORD,STATE,CASE) fPUTTXT(WORD,&STATE,CASE)
extern void fSHFTXT(long,long);
#define SHFTXT(FROM,DELTA) fSHFTXT(FROM,DELTA)
extern void fTYPE0();
#define TYPE0() fTYPE0()
extern void fSAVWDS(long*,long*,long*,long*,long*,long*,long*);
#define SAVWDS(W1,W2,W3,W4,W5,W6,W7) fSAVWDS(&W1,&W2,&W3,&W4,&W5,&W6,&W7)
extern void fSAVARR(long*,long);
#define SAVARR(ARR,N) fSAVARR(ARR,N)
extern void fSAVWRD(long,long*);
#define SAVWRD(OP,WORD) fSAVWRD(OP,&WORD)
extern long fVOCAB(long,long);
#define VOCAB(ID,INIT) fVOCAB(ID,INIT)
extern void fDSTROY(long);
#define DSTROY(OBJECT) fDSTROY(OBJECT)
extern void fJUGGLE(long);
#define JUGGLE(OBJECT) fJUGGLE(OBJECT)
extern void fMOVE(long,long);
#define MOVE(OBJECT,WHERE) fMOVE(OBJECT,WHERE)
extern long fPUT(long,long,long);
#define PUT(OBJECT,WHERE,PVAL) fPUT(OBJECT,WHERE,PVAL)
extern void fCARRY(long,long);
#define CARRY(OBJECT,WHERE) fCARRY(OBJECT,WHERE)
extern void fDROP(long,long);
#define DROP(OBJECT,WHERE) fDROP(OBJECT,WHERE)
extern long fATDWRF(long);
#define ATDWRF(WHERE) fATDWRF(WHERE)
extern long fSETBIT(long);
#define SETBIT(BIT) fSETBIT(BIT)
extern long fTSTBIT(long,long);
#define TSTBIT(MASK,BIT) fTSTBIT(MASK,BIT)
extern long fRNDVOC(long,long);
#define RNDVOC(CHAR,FORCE) fRNDVOC(CHAR,FORCE)
extern void fBUG(long);
#define BUG(NUM) fBUG(NUM)
extern void fMAPLIN(FILE *);
#define MAPLIN(FIL) fMAPLIN(FIL)
extern void fTYPE();
#define TYPE() fTYPE()
extern void fMPINIT();
#define MPINIT() fMPINIT()
extern void fSAVEIO(long,long,long*);
#define SAVEIO(OP,IN,ARR) fSAVEIO(OP,IN,ARR)
extern void DATIME(long*, long*);
extern long fIABS(long);
#define IABS(N) fIABS(N)
extern long fMOD(long,long);
#define MOD(N,M) fMOD(N,M)
extern void set_seed(long);
extern unsigned long get_next_lcg_value(void);
extern long randrange(long);
