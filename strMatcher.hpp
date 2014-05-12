#ifndef __STRMATCHER_HPP
#define __STRMATCHER_HPP

#include "pcre.h"

#define LG_OVECTOR (10 * 3)

#define NO_OPTION            0x00
#define STRM_OPTION_CASELESS 0x01

typedef struct _Tregexp {
   pcre       *regExp;
   pcre_extra *regExpExtra;
   const char *reErrMes;
   int        reErrOffset;
   const char *reeErrMes;
} TregExp;

class strMatcher {
private:
    TregExp *reTable;
    int     nbRE;
    int     strmOptions;
    int     pcreOptions;
	int     ovector[LG_OVECTOR];
	int     rc; //nombre de résultat dans ovector
public:
   strMatcher(char *patternList, bool unique, int options);
   ~strMatcher();

   int doesStrMatchMask(const char *);

   int getOffsetOfMatchingStr();
   int getOffsetOfCaptured(int);
   int getLengthOfCaptured(int);
};

#endif
