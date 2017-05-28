#include <stdbool.h>

#define LINESIZE	100

typedef struct lcg_state
{
  unsigned long a, c, m, x;
} lcg_state;

extern long ABB[], ATLOC[], BLKLIN, DFLAG, DLOC[], FIXED[], HOLDNG,
		LINK[], LNLENG, LNPOSN,
		PARMS[], PLACE[];
extern signed char rawbuf[LINESIZE], INLINE[LINESIZE+1], MAP1[], MAP2[];
extern FILE *logfp;
extern bool oldstyle;
extern lcg_state lcgstate;
