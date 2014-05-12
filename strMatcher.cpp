#include <string.h>
#include "pcre.h"
#include "strMatcher.hpp"

#define LG_LBUF 1024
strMatcher::strMatcher(char *Masks, bool unique, int o) {
char *pc;
char *lBuf;
int i;

    this->strmOptions = o;
    this->pcreOptions = 0;
	this->rc = 0;
    if ( this->strmOptions & STRM_OPTION_CASELESS ) {
       this->pcreOptions |= PCRE_CASELESS;
    }
    reTable = 0;
    if ( *Masks == 0 ) {
       this->nbRE = 0;
       return;
    }

    lBuf = new char[strlen(Masks) + 10];
    strcpy(lBuf, Masks);
    pc = lBuf;
    this->nbRE = 1;
    if ( !unique ) {
        while ( *pc != 0 ) {
            if ( (*pc == ';') || (*pc == ',') ) {
                *pc = 0;
                if ( *(pc + 1) != 0 ) {
                    ++this->nbRE;
                }
            }
            ++pc;
        }
    }

    this->reTable = new TregExp[nbRE];
    memset(this->reTable, 0, sizeof(TregExp) * nbRE);

    pc = lBuf;
    for ( i = 0; i < nbRE; i++ ) {
       if ( (this->reTable[i].regExp = pcre_compile(pc, this->pcreOptions, &this->reTable[i].reErrMes, &this->reTable[i].reErrOffset, 0)) != 0 ) {
          this->reTable[i].regExpExtra = pcre_study(this->reTable[i].regExp, 0, &this->reTable[i].reeErrMes);
       }
       else {
          --i;
          --this->nbRE;
       }
       if ( *pc == 0 )
           ++pc;
       while ( *pc )
           ++pc;
       ++pc;
    }

    delete[] lBuf;
}

strMatcher::~strMatcher() {
int i;
    if ( reTable != 0) {
        for ( i = 0; i < nbRE; i++ ) {
           if ( this->reTable[i].regExp != 0 ) {
              free(this->reTable[i].regExp);
           }
           if ( this->reTable[i].regExpExtra != 0 ) {
              free(this->reTable[i].regExpExtra);
           }
        }
        delete reTable;
    }
}


int strMatcher::doesStrMatchMask(const char *str) {
int i, lg;
 
    this->rc = 0;
    lg = strlen(str);
    
    for ( i = 0; i < nbRE; i++ ) {
		if ( (this->rc = pcre_exec(this->reTable[i].regExp, this->reTable[i].regExpExtra, str, lg, 0, 0, this->ovector, LG_OVECTOR)) > 0 ) {
            return 1;
		}
    }
    
    return 0;
}

int strMatcher::getOffsetOfCaptured(int c) {
	if ( this->rc ) {
		if ( (c >= 0) && (c < this->rc)  )
		    return this->ovector[c * 2];
	}

	return -1;
}

int strMatcher::getLengthOfCaptured(int c) {
	if ( this->rc ) {
		if ( (c >= 0) && (c < this->rc)  )
		    return this->ovector[c * 2 + 1] - this->ovector[c * 2];
	}

	return -1;
}

int strMatcher::getOffsetOfMatchingStr() {
	return this->getOffsetOfCaptured(0);
}
