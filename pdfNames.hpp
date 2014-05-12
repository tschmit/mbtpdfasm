#ifndef _PDF_NAMES_
#define _PDF_NAMES_

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "pdfXrefTable.hpp"
#include "pdfEncrypt.hpp"
#include "pdfString.hpp"

typedef struct nameStruct {
   pdfString         str;
   int               nObj;
   int               version;
   struct nameStruct *next;
} T_name;

typedef struct {
   const char *name;
   T_name **tree;
   int rootObj;
} T_name2tree;

class pdfNames {
private:
   T_name *Dests;
   T_name *AP;
   T_name *JavaScript;
   T_name *Pages;
   T_name *Templates;
   T_name *IDS;
   T_name *URLS;

   T_name **currentTreeRoot;
   T_name *currentTreeLeaf;

   T_name2tree *n2t;

   C_pdfXrefTable *pXT;
   FILE           *pFile;
   pdfEncrypt     *pEncrypt;
   T_name *buildNameTree(int nObj);

   int rootNamesObj;

   int pN2T;

public:
   pdfNames(int nObj, C_pdfXrefTable *px, FILE *pf, pdfEncrypt *encrypt);
   ~pdfNames();

   int mergeNames(pdfNames *orgNames);

   int flushNames();

   inline getRootNamesObj() {return this->rootNamesObj;}

   const T_name2tree *getFirstN2T();
   const T_name2tree *getNextN2T();

};

#endif
