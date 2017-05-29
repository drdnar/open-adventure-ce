#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "main.h"
#include "share.h"	/* for SETUP */
#include "misc.h"
#include "funcs.h"
#include "database.h"

/* hack to ignore GCC Unused Result */
#define IGNORE(r) do{if(r){}}while(0)

/*  I/O routines (SPEAK, PSPEAK, RSPEAK, SETPRM, GETIN, YES) */

#undef SPEAK
void fSPEAK(long N) {
long BLANK, CASE, I, K, L, NEG, NPARMS, PARM, PRMTYP, STATE;

/*  Print the message which starts at LINES(N).  Precede it with a blank line
 *  unless BLKLIN is false. */


	if(N == 0)return;
	BLANK=BLKLIN;
	K=N;
	NPARMS=1;
L10:	L=IABS(LINES[K])-1;
	K=K+1;
	LNLENG=0;
	LNPOSN=1;
	STATE=0;
	for (I=K; I<=L; I++) {
	PUTTXT(LINES[I],STATE,2);
	} /* end loop */
	LNPOSN=0;
L30:	LNPOSN=LNPOSN+1;
L32:	if(LNPOSN > LNLENG) goto L40;
	if(INLINE[LNPOSN] != 63) goto L30;
	{long x = LNPOSN+1; PRMTYP=INLINE[x];}
/*  63 is a "%"; the next character determine the type of parameter:  1 (!) =
 *  suppress message completely, 29 (S) = NULL If PARM=1, else 'S' (optional
 *  plural ending), 33 (W) = word (two 30-bit values) with trailing spaces
 *  suppressed, 22 (L) or 31 (U) = word but map to lower/upper case, 13 (C) =
 *  word in lower case with first letter capitalised, 30 (T) = text ending
 *  with a word of -1, 65-73 (1-9) = number using that many characters,
 *  12 (B) = variable number of blanks. */
	if(PRMTYP == 1)return;
	if(PRMTYP == 29) goto L320;
	if(PRMTYP == 30) goto L340;
	if(PRMTYP == 12) goto L360;
	if(PRMTYP == 33 || PRMTYP == 22 || PRMTYP == 31 || PRMTYP == 13) goto
		L380;
	PRMTYP=PRMTYP-64;
	if(PRMTYP < 1 || PRMTYP > 9) goto L30;
	SHFTXT(LNPOSN+2,PRMTYP-2);
	LNPOSN=LNPOSN+PRMTYP;
	PARM=IABS(PARMS[NPARMS]);
	NEG=0;
	if(PARMS[NPARMS] < 0)NEG=9;
	/* 390 */ for (I=1; I<=PRMTYP; I++) {
	LNPOSN=LNPOSN-1;
	INLINE[LNPOSN]=MOD(PARM,10)+64;
	if(I == 1 || PARM != 0) goto L390;
	INLINE[LNPOSN]=NEG;
	NEG=0;
L390:	PARM=PARM/10;
	} /* end loop */
	LNPOSN=LNPOSN+PRMTYP;
L395:	NPARMS=NPARMS+1;
	 goto L32;

L320:	SHFTXT(LNPOSN+2,-1);
	INLINE[LNPOSN]=55;
	if(PARMS[NPARMS] == 1)SHFTXT(LNPOSN+1,-1);
	 goto L395;

L340:	SHFTXT(LNPOSN+2,-2);
	STATE=0;
	CASE=2;
L345:	if(PARMS[NPARMS] < 0) goto L395;
	{long x = NPARMS+1; if(PARMS[x] < 0)CASE=0;}
	PUTTXT(PARMS[NPARMS],STATE,CASE);
	NPARMS=NPARMS+1;
	 goto L345;

L360:	PRMTYP=PARMS[NPARMS];
	SHFTXT(LNPOSN+2,PRMTYP-2);
	if(PRMTYP == 0) goto L395;
	for (I=1; I<=PRMTYP; I++) {
	INLINE[LNPOSN]=0;
	LNPOSN=LNPOSN+1;
	} /* end loop */
	 goto L395;

L380:	SHFTXT(LNPOSN+2,-2);
	STATE=0;
	CASE= -1;
	if(PRMTYP == 31)CASE=1;
	if(PRMTYP == 33)CASE=0;
	I=LNPOSN;
	PUTTXT(PARMS[NPARMS],STATE,CASE);
	{long x = NPARMS+1; PUTTXT(PARMS[x],STATE,CASE);}
	if(PRMTYP == 13 && INLINE[I] >= 37 && INLINE[I] <=
		62)INLINE[I]=INLINE[I]-26;
	NPARMS=NPARMS+2;
	 goto L32;

L40:	if(BLANK)TYPE0();
	BLANK=false;
	TYPE();
	K=L+1;
	if(LINES[K] >= 0) goto L10;
	return;
}



#define SPEAK(N) fSPEAK(N)
#undef PSPEAK
void fPSPEAK(long MSG,long SKIP) {
long I, M;

/*  Find the skip+1st message from msg and print it.  MSG should be the index of
 *  the inventory message for object.  (INVEN+N+1 message is PROP=N message). */


	M=PTEXT[MSG];
	if(SKIP < 0) goto L9;
	for (I=0; I<=SKIP; I++) {
L1:	M=IABS(LINES[M]);
	if(LINES[M] >= 0) goto L1;
	/*etc*/ ;
	} /* end loop */
L9:	SPEAK(M);
	return;
}



#define PSPEAK(MSG,SKIP) fPSPEAK(MSG,SKIP)
#undef RSPEAK
void fRSPEAK(long I) {
;

/*  Print the I-TH "random" message (section 6 of database). */


	if(I != 0)SPEAK(RTEXT[I]);
	return;
}



#define RSPEAK(I) fRSPEAK(I)
#undef SETPRM
void fSETPRM(long FIRST, long P1, long P2) {
;

/*  Stores parameters into the PRMCOM parms array for use by speak.  P1 and P2
 *  are stored into PARMS(FIRST) and PARMS(FIRST+1). */


	if(FIRST >= 25)BUG(29);
	PARMS[FIRST]=P1;
	{long x = FIRST+1; PARMS[x]=P2;}
	return;
}



#define SETPRM(FIRST,P1,P2) fSETPRM(FIRST,P1,P2)
#undef GETIN
#define WORD1 (*wORD1)
#define WORD1X (*wORD1X)
#define WORD2 (*wORD2)
#define WORD2X (*wORD2X)
bool fGETIN(FILE *input, long *wORD1, long *wORD1X, long *wORD2, long *wORD2X) {
long JUNK;

/*  Get a command from the adventurer.  Snarf out the first word, pad it with
 *  blanks, and return it in WORD1.  Chars 6 thru 10 are returned in WORD1X, in
 *  case we need to print out the whole word in an error message.  Any number of
 *  blanks may follow the word.  If a second word appears, it is returned in
 *  WORD2 (chars 6 thru 10 in WORD2X), else WORD2 is -1. */


L10:	if(BLKLIN)TYPE0();
	MAPLIN(input);
	if (feof(input))
	    return false;
	WORD1=GETTXT(true,true,true);
	if(BLKLIN && WORD1 < 0) goto L10;
	WORD1X=GETTXT(false,true,true);
L12:	JUNK=GETTXT(false,true,true);
	if(JUNK > 0) goto L12;
	WORD2=GETTXT(true,true,true);
	WORD2X=GETTXT(false,true,true);
L22:	JUNK=GETTXT(false,true,true);
	if(JUNK > 0) goto L22;
	if(GETTXT(true,true,true) <= 0)return true;
	RSPEAK(53);
	 goto L10;
}



#undef WORD1
#undef WORD1X
#undef WORD2
#undef WORD2X
#define GETIN(SRC,WORD1,WORD1X,WORD2,WORD2X) fGETIN(SRC,&WORD1,&WORD1X,&WORD2,&WORD2X)
#undef YES
long fYES(FILE *input, long X, long Y, long Z) {

long YES, REPLY, JUNK1, JUNK2, JUNK3;

/*  Print message X, wait for yes/no answer.  If yes, print Y and return true;
 *  if no, print Z and return false. */

L1:	RSPEAK(X);
	GETIN(input, REPLY,JUNK1,JUNK2,JUNK3);
	if(REPLY == MAKEWD(250519) || REPLY == MAKEWD(25)) goto L10;
	if(REPLY == MAKEWD(1415) || REPLY == MAKEWD(14)) goto L20;
	RSPEAK(185);
	 goto L1;
L10:	YES=true;
	RSPEAK(Y);
	return(YES);
L20:	YES=false;
	RSPEAK(Z);
	return(YES);
}





/*  Line-parsing routines (GETNUM, GETTXT, MAKEWD, PUTTXT, SHFTXT, TYPE0)
		*/

/*  The routines on this page handle all the stuff that would normally be
 *  taken care of by format statements.  We do it this way instead so that
 *  we can handle textual data in a machine independent fashion.  All the
 *  machine dependent i/o stuff is on the following page.  See that page
 *  for a description of MAPCOM's inline array. */

#define YES(X,Y,Z) fYES(X,Y,Z)
#undef GETNUM
long fGETNUM(FILE *source) {
long DIGIT, GETNUM, SIGN;

/*  Obtain the next integer from an input line.  If K>0, we first read a
 *  new input line from a file; if K<0, we read a line from the keyboard;
 *  if K=0 we use a line that has already been read (and perhaps partially
 *  scanned).  If we're at the end of the line or encounter an illegal
 *  character (not a digit, hyphen, or blank), we return 0. */


	if(source != NULL)MAPLIN(source);
	GETNUM=0;
L10:	if(LNPOSN > LNLENG)return(GETNUM);
	if(INLINE[LNPOSN] != 0) goto L20;
	LNPOSN=LNPOSN+1;
	 goto L10;

L20:	SIGN=1;
	if(INLINE[LNPOSN] != 9) goto L32;
	SIGN= -1;
L30:	LNPOSN=LNPOSN+1;
L32:	if(LNPOSN > LNLENG || INLINE[LNPOSN] == 0) goto L42;
	DIGIT=INLINE[LNPOSN]-64;
	if(DIGIT < 0 || DIGIT > 9) goto L40;
	GETNUM=GETNUM*10+DIGIT;
	 goto L30;

L40:	GETNUM=0;
L42:	GETNUM=GETNUM*SIGN;
	LNPOSN=LNPOSN+1;
	return(GETNUM);
}



#define GETNUM(K) fGETNUM(K)
#undef GETTXT
long fGETTXT(long SKIP,long ONEWRD, long UPPER) {
long CHAR, GETTXT, I; static long SPLITTING = -1;

/*  Take characters from an input line and pack them into 30-bit words.
 *  Skip says to skip leading blanks.  ONEWRD says stop if we come to a
 *  blank.  UPPER says to map all letters to uppercase.  If we reach the
 *  end of the line, the word is filled up with blanks (which encode as 0's).
 *  If we're already at end of line when GETTXT is called, we return -1. */

	if(LNPOSN != SPLITTING)SPLITTING = -1;
	GETTXT= -1;
L10:	if(LNPOSN > LNLENG)return(GETTXT);
	if((!SKIP) || INLINE[LNPOSN] != 0) goto L11;
	LNPOSN=LNPOSN+1;
	 goto L10;

L11:	GETTXT=0;
	/* 15 */ for (I=1; I<=5; I++) {
	GETTXT=GETTXT*64;
	if(LNPOSN > LNLENG || (ONEWRD && INLINE[LNPOSN] == 0)) goto L15;
	CHAR=INLINE[LNPOSN];
	if(CHAR >= 63) goto L12;
	SPLITTING = -1;
	if(UPPER && CHAR >= 37)CHAR=CHAR-26;
	GETTXT=GETTXT+CHAR;
	 goto L14;

L12:	if(SPLITTING == LNPOSN) goto L13;
	GETTXT=GETTXT+63;
	SPLITTING = LNPOSN;
	 goto L15;

L13:	GETTXT=GETTXT+CHAR-63;
	SPLITTING = -1;
L14:	LNPOSN=LNPOSN+1;
L15:	/*etc*/ ;
	} /* end loop */

	return(GETTXT);
}



#define GETTXT(SKIP,ONEWRD,UPPER) fGETTXT(SKIP,ONEWRD,UPPER)
#undef MAKEWD
long fMAKEWD(long LETTRS) {
long I, L, MAKEWD;

/*  Combine five uppercase letters (represented by pairs of decimal digits
 *  in lettrs) to form a 30-bit value matching the one that GETTXT would
 *  return given those characters plus trailing blanks.  Caution:
 *  lettrs will overflow 31 bits if 5-letter word starts with V-Z.  As a
 *  kludgey workaround, you can increment a letter by 5 by adding 50 to
 *  the next pair of digits. */


	MAKEWD=0;
	I=1;
	L=LETTRS;
L10:	MAKEWD=MAKEWD+I*(MOD(L,50)+10);
	I=I*64;
	if(MOD(L,100) > 50)MAKEWD=MAKEWD+I*5;
	L=L/100;
	if(L != 0) goto L10;
	I=64L*64L*64L*64L*64L/I;
	MAKEWD=MAKEWD*I;
	return(MAKEWD);
}



#define MAKEWD(LETTRS) fMAKEWD(LETTRS)
#undef PUTTXT
#define STATE (*sTATE)
void fPUTTXT(long WORD, long *sTATE, long CASE) {
long ALPH1, ALPH2, BYTE, DIV, I, W;

/*  Unpack the 30-bit value in word to obtain up to 5 integer-encoded chars,
 *  and store them in inline starting at LNPOSN.  If LNLENG>=LNPOSN, shift
 *  existing characters to the right to make room.  STATE will be zero when
 *  puttxt is called with the first of a sequence of words, but is thereafter
 *  unchanged by the caller, so PUTTXT can use it to maintain state across
 *  calls.  LNPOSN and LNLENG are incremented by the number of chars stored.
 *  If CASE=1, all letters are made uppercase; if -1, lowercase; if 0, as is.
 *  any other value for case is the same as 0 but also causes trailing blanks
 *  to be included (in anticipation of subsequent additional text). */


	ALPH1=13*CASE+24;
	ALPH2=26*IABS(CASE)+ALPH1;
	if(IABS(CASE) > 1)ALPH1=ALPH2;
/*  ALPH1&2 DEFINE RANGE OF WRONG-CASE CHARS, 11-36 OR 37-62 OR EMPTY. */
	DIV=64L*64L*64L*64L;
	W=WORD;
	/* 18 */ for (I=1; I<=5; I++) {
	if(W <= 0 && STATE == 0 && IABS(CASE) <= 1)return;
	BYTE=W/DIV;
	if(STATE != 0 || BYTE != 63) goto L12;
	STATE=63;
	 goto L18;

L12:	SHFTXT(LNPOSN,1);
	STATE=STATE+BYTE;
	if(STATE < ALPH2 && STATE >= ALPH1)STATE=STATE-26*CASE;
	INLINE[LNPOSN]=STATE;
	LNPOSN=LNPOSN+1;
	STATE=0;
L18:	W=(W-BYTE*DIV)*64;
	} /* end loop */
	return;
}



#undef STATE
#define PUTTXT(WORD,STATE,CASE) fPUTTXT(WORD,&STATE,CASE)
#undef SHFTXT
void fSHFTXT(long FROM, long DELTA) {
long I, II, JJ;

/*  Move INLINE(N) to INLINE(N+DELTA) for N=FROM,LNLENG.  Delta can be
 *  negative.  LNLENG is updated; LNPOSN is not changed. */


	if(LNLENG < FROM || DELTA == 0) goto L2;
	for (I=FROM; I<=LNLENG; I++) {
	II=I;
	if(DELTA > 0)II=FROM+LNLENG-I;
	JJ=II+DELTA;
	INLINE[JJ]=INLINE[II];
	} /* end loop */
L2:	LNLENG=LNLENG+DELTA;
	return;
}



#define SHFTXT(FROM,DELTA) fSHFTXT(FROM,DELTA)
#undef TYPE0
void fTYPE0() {
long TEMP;

/*  Type a blank line.  This procedure is provided as a convenience for callers
 *  who otherwise have no use for MAPCOM. */


	TEMP=LNLENG;
	LNLENG=0;
	TYPE();
	LNLENG=TEMP;
	return;
}



#define TYPE0() fTYPE0()


/*  Suspend/resume I/O routines (SAVWDS, SAVARR, SAVWRD) */

#undef SAVWDS
void fSAVWDS(long *W1, long *W2, long *W3, long *W4, long *W5, long *W6, long *W7) {

/*  Write or read 7 variables.  See SAVWRD. */


	SAVWRD(0,(*W1));
	SAVWRD(0,(*W2));
	SAVWRD(0,(*W3));
	SAVWRD(0,(*W4));
	SAVWRD(0,(*W5));
	SAVWRD(0,(*W6));
	SAVWRD(0,(*W7));
	return;
}


#define SAVWDS(W1,W2,W3,W4,W5,W6,W7) fSAVWDS(&W1,&W2,&W3,&W4,&W5,&W6,&W7)
#undef SAVARR
void fSAVARR(long ARR[], long N) {
long I;

/*  Write or read an array of N words.  See SAVWRD. */


	for (I=1; I<=N; I++) {
	SAVWRD(0,ARR[I]);
	} /* end loop */
	return;
}



#define SAVARR(ARR,N) fSAVARR(ARR,N)
#undef SAVWRD
#define WORD (*wORD)
void fSAVWRD(long OP, long *wORD) {
static long BUF[250], CKSUM = 0, H1, HASH = 0, N = 0, STATE = 0;

/*  If OP<0, start writing a file, using word to initialise encryption; save
 *  word in the file.  If OP>0, start reading a file; read the file to find
 *  the value with which to decrypt the rest.  In either case, if a file is
 *  already open, finish writing/reading it and don't start a new one.  If OP=0,
 *  read/write a single word.  Words are buffered in case that makes for more
 *  efficient disk use.  We also compute a simple checksum to catch elementary
 *  poking within the saved file.  When we finish reading/writing the file,
 *  we store zero into WORD if there's no checksum error, else nonzero. */


	if(OP != 0){long ifvar; ifvar=(STATE); switch (ifvar<0? -1 : ifvar>0? 1 :
		0) { case -1: goto L30; case 0: goto L10; case 1: goto L30; }}
	if(STATE == 0)return;
	if(N == 250)SAVEIO(1,STATE > 0,BUF);
	N=MOD(N,250)+1;
	H1=MOD(HASH*1093L+221573L,1048576L);
	HASH=MOD(H1*1093L+221573L,1048576L);
	H1=MOD(H1,1234)*765432+MOD(HASH,123);
	N--;
	if(STATE > 0)WORD=BUF[N]+H1;
	BUF[N]=WORD-H1;
	N++;
	CKSUM=MOD(CKSUM*13+WORD,1000000000L);
	return;

L10:	STATE=OP;
	SAVEIO(0,STATE > 0,BUF);
	N=1;
	if(STATE > 0) goto L15;
	HASH=MOD(WORD,1048576L);
	BUF[0]=1234L*5678L-HASH;
L13:	CKSUM=BUF[0];
	return;

L15:	SAVEIO(1,true,BUF);
	HASH=MOD(1234L*5678L-BUF[0],1048576L);
	 goto L13;

L30:	if(N == 250)SAVEIO(1,STATE > 0,BUF);
	N=MOD(N,250)+1;
	if(STATE > 0) goto L32;
	N--; BUF[N]=CKSUM; N++;
	SAVEIO(1,false,BUF);
L32:	N--; WORD=BUF[N]-CKSUM; N++;
	SAVEIO(-1,STATE > 0,BUF);
	STATE=0;
	return;
}





/*  Data struc. routines (VOCAB, DSTROY, JUGGLE, MOVE, PUT, CARRY, DROP, ATDWRF)
		*/

#undef WORD
#define SAVWRD(OP,WORD) fSAVWRD(OP,&WORD)
#undef VOCAB
long fVOCAB(long ID, long INIT) {
long I, VOCAB;

/*  Look up ID in the vocabulary (ATAB) and return its "definition" (KTAB), or
 *  -1 if not found.  If INIT is positive, this is an initialisation call setting
 *  up a keyword variable, and not finding it constitutes a bug.  It also means
 *  that only KTAB values which taken over 1000 equal INIT may be considered.
 *  (Thus "STEPS", which is a motion verb as well as an object, may be located
 *  as an object.)  And it also means the KTAB value is taken modulo 1000. */

	/* 1 */ for (I=1; I<=TABSIZ; I++) {
	if(KTAB[I] == -1) goto L2;
	if(INIT >= 0 && KTAB[I]/1000 != INIT) goto L1;
	if(ATAB[I] == ID) goto L3;
L1:	/*etc*/ ;
	} /* end loop */
	BUG(21);

L2:	VOCAB= -1;
	if(INIT < 0)return(VOCAB);
	BUG(5);

L3:	VOCAB=KTAB[I];
	if(INIT >= 0)VOCAB=MOD(VOCAB,1000);
	return(VOCAB);
}



#define VOCAB(ID,INIT) fVOCAB(ID,INIT)
#undef DSTROY
void fDSTROY(long OBJECT) {
;

/*  Permanently eliminate "OBJECT" by moving to a non-existent location. */


	MOVE(OBJECT,0);
	return;
}



#define DSTROY(OBJECT) fDSTROY(OBJECT)
#undef JUGGLE
void fJUGGLE(long OBJECT) {
long I, J;

/*  Juggle an object by picking it up and putting it down again, the purpose
 *  being to get the object to the front of the chain of things at its loc. */


	I=PLACE[OBJECT];
	J=FIXED[OBJECT];
	MOVE(OBJECT,I);
	MOVE(OBJECT+100,J);
	return;
}



#define JUGGLE(OBJECT) fJUGGLE(OBJECT)
#undef MOVE
void fMOVE(long OBJECT, long WHERE) {
long FROM;

/*  Place any object anywhere by picking it up and dropping it.  May already be
 *  toting, in which case the carry is a no-op.  Mustn't pick up objects which
 *  are not at any loc, since carry wants to remove objects from ATLOC chains. */


	if(OBJECT > 100) goto L1;
	FROM=PLACE[OBJECT];
	 goto L2;
L1:	{long x = OBJECT-100; FROM=FIXED[x];}
L2:	if(FROM > 0 && FROM <= 300)CARRY(OBJECT,FROM);
	DROP(OBJECT,WHERE);
	return;
}



#define MOVE(OBJECT,WHERE) fMOVE(OBJECT,WHERE)
#undef PUT
long fPUT(long OBJECT, long WHERE, long PVAL) {
long PUT;

/*  PUT is the same as MOVE, except it returns a value used to set up the
 *  negated PROP values for the repository objects. */


	MOVE(OBJECT,WHERE);
	PUT=(-1)-PVAL;
	return(PUT);
}



#define PUT(OBJECT,WHERE,PVAL) fPUT(OBJECT,WHERE,PVAL)
#undef CARRY
void fCARRY(long OBJECT, long WHERE) {
long TEMP;

/*  Start toting an object, removing it from the list of things at its former
 *  location.  Incr holdng unless it was already being toted.  If OBJECT>100
 *  (moving "fixed" second loc), don't change PLACE or HOLDNG. */


	if(OBJECT > 100) goto L5;
	if(PLACE[OBJECT] == -1)return;
	PLACE[OBJECT]= -1;
	HOLDNG=HOLDNG+1;
L5:	if(ATLOC[WHERE] != OBJECT) goto L6;
	ATLOC[WHERE]=LINK[OBJECT];
	return;
L6:	TEMP=ATLOC[WHERE];
L7:	if(LINK[TEMP] == OBJECT) goto L8;
	TEMP=LINK[TEMP];
	 goto L7;
L8:	LINK[TEMP]=LINK[OBJECT];
	return;
}



#define CARRY(OBJECT,WHERE) fCARRY(OBJECT,WHERE)
#undef DROP
void fDROP(long OBJECT, long WHERE) {
;

/*  Place an object at a given loc, prefixing it onto the ATLOC list.  Decr
 *  HOLDNG if the object was being toted. */


	if(OBJECT > 100) goto L1;
	if(PLACE[OBJECT] == -1)HOLDNG=HOLDNG-1;
	PLACE[OBJECT]=WHERE;
	 goto L2;
L1:	{long x = OBJECT-100; FIXED[x]=WHERE;}
L2:	if(WHERE <= 0)return;
	LINK[OBJECT]=ATLOC[WHERE];
	ATLOC[WHERE]=OBJECT;
	return;
}



#define DROP(OBJECT,WHERE) fDROP(OBJECT,WHERE)
#undef ATDWRF
long fATDWRF(long WHERE) {
long ATDWRF, I;

/*  Return the index of first dwarf at the given location, zero if no dwarf is
 *  there (or if dwarves not active yet), -1 if all dwarves are dead.  Ignore
 *  the pirate (6th dwarf). */


	ATDWRF=0;
	if(DFLAG < 2)return(ATDWRF);
	ATDWRF= -1;
	for (I=1; I<=5; I++) {
	if(DLOC[I] == WHERE) goto L2;
	if(DLOC[I] != 0)ATDWRF=0;
	} /* end loop */
	return(ATDWRF);

L2:	ATDWRF=I;
	return(ATDWRF);
}




#define ATDWRF(WHERE) fATDWRF(WHERE)



/*  Utility routines (SETBIT, TSTBIT, set_seed, get_next_lcg_value,
 *  randrange, RNDVOC, BUG) */

#undef SETBIT
long fSETBIT(long BIT) {
long I, SETBIT;

/*  Returns 2**bit for use in constructing bit-masks. */


	SETBIT=1;
	if(BIT <= 0)return(SETBIT);
	for (I=1; I<=BIT; I++) {
	SETBIT=SETBIT+SETBIT;
	} /* end loop */
	return(SETBIT);
}



#define SETBIT(BIT) fSETBIT(BIT)
#undef TSTBIT
long fTSTBIT(long MASK, long BIT) {
long TSTBIT;

/*  Returns true if the specified bit is set in the mask. */


	TSTBIT=MOD(MASK/SETBIT(BIT),2) != 0;
	return(TSTBIT);
}



#define TSTBIT(MASK,BIT) fTSTBIT(MASK,BIT)

void set_seed(long seedval)
{
  lcgstate.x = (unsigned long) seedval % lcgstate.m;
}

unsigned long get_next_lcg_value(void)
{
  /* Return the LCG's current value, and then iterate it. */
  unsigned long old_x = lcgstate.x;
  lcgstate.x = (lcgstate.a * lcgstate.x + lcgstate.c) % lcgstate.m;
  return(old_x);
}

long randrange(long range)
{
  /* Return a random integer from [0, range). */
  long result = range * get_next_lcg_value() / lcgstate.m;
  return(result);
}

#undef RNDVOC
long fRNDVOC(long CHAR, long FORCE) {
/*  Searches the vocabulary for a word whose second character is char, and
 *  changes that word such that each of the other four characters is a
 *  random letter.  If force is non-zero, it is used as the new word.
 *  Returns the new word. */

	long RNDVOC;

	RNDVOC=FORCE;

	if (RNDVOC == 0) {
	  for (int I = 1; I <= 5; I++) {
	    long J = 11 + randrange(26);
	    if (I == 2)
	      J = CHAR;
	    RNDVOC = RNDVOC * 64 + J;
	  }
	}

	long DIV = 64L * 64L * 64L;
	for (int I = 1; I <= TABSIZ; I++) {
	  if (MOD(ATAB[I]/DIV, 64L) == CHAR)
	    {
	      ATAB[I] = RNDVOC;
	      break;
	    }
	}

	return(RNDVOC);
}



#define RNDVOC(CHAR,FORCE) fRNDVOC(CHAR,FORCE)
#undef BUG
void fBUG(long NUM) {

/*  The following conditions are currently considered fatal bugs.  Numbers < 20
 *  are detected while reading the database; the others occur at "run time".
 *	0	Message line > 70 characters
 *	1	Null line in message
 *	2	Too many words of messages
 *	3	Too many travel options
 *	4	Too many vocabulary words
 *	5	Required vocabulary word not found
 *	6	Too many RTEXT messages
 *	7	Too many hints
 *	8	Location has cond bit being set twice
 *	9	Invalid section number in database
 *	10	Too many locations
 *	11	Too many class or turn messages
 *	20	Special travel (500>L>300) exceeds goto list
 *	21	Ran off end of vocabulary table
 *	22	Vocabulary type (N/1000) not between 0 and 3
 *	23	Intransitive action verb exceeds goto list
 *	24	Transitive action verb exceeds goto list
 *	25	Conditional travel entry with no alternative
 *	26	Location has no travel entries
 *	27	Hint number exceeds goto list
 *	28	Invalid month returned by date function
 *	29	Too many parameters given to SETPRM */

	printf("Fatal error %ld.  See source code for interpretation.\n",
	   NUM);
	exit(0);
}





/*  Machine dependent routines (MAPLIN, TYPE, MPINIT, SAVEIO) */

#define BUG(NUM) fBUG(NUM)
#undef MAPLIN
void fMAPLIN(FILE *OPENED) {
long I, VAL;

/*  Read a line of input, from the specified input source,
 *  translate the chars to integers in the range 0-126 and store
 *  them in the common array "INLINE".  Integer values are as follows:
 *     0   = space [ASCII CODE 40 octal, 32 decimal]
 *    1-2  = !" [ASCII 41-42 octal, 33-34 decimal]
 *    3-10 = '()*+,-. [ASCII 47-56 octal, 39-46 decimal]
 *   11-36 = upper-case letters
 *   37-62 = lower-case letters
 *    63   = percent (%) [ASCII 45 octal, 37 decimal]
 *   64-73 = digits, 0 through 9
 *  Remaining characters can be translated any way that is convenient;
 *  The "TYPE" routine below is used to map them back to characters when
 *  necessary.  The above mappings are required so that certain special
 *  characters are known to fit in 6 bits and/or can be easily spotted.
 *  Array elements beyond the end of the line should be filled with 0,
 *  and LNLENG should be set to the index of the last character.
 *
 *  If the data file uses a character other than space (e.g., tab) to
 *  separate numbers, that character should also translate to 0.
 *
 *  This procedure may use the map1,map2 arrays to maintain static data for
 *  the mapping.  MAP2(1) is set to 0 when the program starts
 *  and is not changed thereafter unless the routines on this page choose
 *  to do so. */

	if(MAP2[1] == 0)MPINIT();

	if (!oldstyle && SETUP && OPENED == stdin)
		fputs("> ", stdout);
	do {
		IGNORE(fgets(rawbuf,sizeof(rawbuf)-1,OPENED));
	} while
		(!feof(OPENED) && rawbuf[0] == '#');
	if (feof(OPENED)) {
		if (logfp && OPENED == stdin)
			fclose(logfp);
	} else {
		if (logfp && OPENED == stdin)
			IGNORE(fputs(rawbuf, logfp));
		else if (!isatty(0))
			IGNORE(fputs(rawbuf, stdout));
		strcpy(INLINE+1, rawbuf);
		LNLENG=0;
		for (I=1; I<=sizeof(INLINE) && INLINE[I]!=0; I++) {
		VAL=INLINE[I]+1;
		INLINE[I]=MAP1[VAL];
		if(INLINE[I] != 0)LNLENG=I;
		} /* end loop */
		LNPOSN=1;
	}
}
#define MAPLIN(FIL) fMAPLIN(FIL)

#undef TYPE
void fTYPE(void) {
long I, VAL;

/*  Type the first "LNLENG" characters stored in inline, mapping them
 *  from integers to text per the rules described above.  INLINE(I),
 *  I=1,LNLENG may be changed by this routine. */


	if(LNLENG != 0) goto L10;
	printf("\n");
	return;

L10:	if(MAP2[1] == 0)MPINIT();
	for (I=1; I<=LNLENG; I++) {
	VAL=INLINE[I];
	{long x = VAL+1; INLINE[I]=MAP2[x];}
	} /* end loop */
	{long x = LNLENG+1; INLINE[x]=0;}
	printf("%s\n",INLINE+1);
	return;
}



#define TYPE() fTYPE()
#undef MPINIT
void fMPINIT(void) {
long FIRST, I, J, LAST, VAL;
static long RUNS[7][2] = {32,34, 39,46, 65,90, 97,122, 37,37, 48,57, 0,126};


	for (I=1; I<=128; I++) {
	MAP1[I]= -1;
	} /* end loop */
	VAL=0;
	for (I=0; I<7; I++) {
	FIRST=RUNS[I][0];
	LAST=RUNS[I][1];
	/* 22 */ for (J=FIRST; J<=LAST; J++) {
	J++; if(MAP1[J] >= 0) goto L22;
	MAP1[J]=VAL;
	VAL=VAL+1;
L22:	J--;
	} /* end loop */
	/*etc*/ ;
	} /* end loop */
	MAP1[128]=MAP1[10];
/*  For this version, tab (9) maps to space (32), so del (127) uses tab's value */
	MAP1[10]=MAP1[33];
	MAP1[11]=MAP1[33];

	for (I=0; I<=126; I++) {
	I++; VAL=MAP1[I]+1; I--;
	MAP2[VAL]=I*('B'-'A');
	if(I >= 64)MAP2[VAL]=(I-64)*('B'-'A')+'@';
	} /* end loop */

	return;
}



#define MPINIT() fMPINIT()
#undef SAVEIO
void fSAVEIO(long OP, long IN, long ARR[]) {
static FILE *F; char NAME[50];

/*  If OP=0, ask for a file name and open a file.  (If IN=true, the file is for
 *  input, else output.)  If OP>0, read/write ARR from/into the previously-opened
 *  file.  (ARR is a 250-integer array.)  If OP<0, finish reading/writing the
 *  file.  (Finishing writing can be a no-op if a "stop" statement does it
 *  automatically.  Finishing reading can be a no-op as long as a subsequent
 *  SAVEIO(0,false,X) will still work.)  If you can catch errors (e.g., no such
 *  file) and try again, great.  DEC F40 can't. */


	{long ifvar; ifvar=(OP); switch (ifvar<0? -1 : ifvar>0? 1 : 0) { case -1:
		goto L10; case 0: goto L20; case 1: goto L30; }}

L10:	fclose(F);
	return;

L20:	printf("\nFile name: ");
	IGNORE(fgets(NAME, sizeof(NAME), stdin));
	F=fopen(NAME,(IN ? READ_MODE : WRITE_MODE));
	if(F == NULL) {printf("Can't open file, try again.\n"); goto L20;}
	return;

L30:	if(IN)IGNORE(fread(ARR,sizeof(long),250,F));
	if(!IN)fwrite(ARR,sizeof(long),250,F);
	return;

}



void DATIME(long* D, long* T) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  *D = (long) tv.tv_sec;
  *T = (long) tv.tv_usec;
}
long fIABS(N)long N; {return(N<0? -N : N);}
long fMOD(N,M)long N, M; {return(N%M);}
