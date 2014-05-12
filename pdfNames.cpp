#ifdef DEBUG_MEM_LEAK
#ifdef WIN32
   #include <crtdbg.h>
   #define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
   #define new DEBUG_NEW
#endif
#endif

#include "pdfFile.hpp"
#include "diversPdf.hpp"
#include "math.h"

const char *nameOfTrees[] = {
   "Dests",
   "AP",
   "JavaScript",
   "Pages",
   "Templates",
   "IDS",
   "URLS",
   ""
};

T_name *C_pdfFile::buildNameTree(int nObj) {
char *lBuf;
int lgLBuf;
int lgObj;
int i, lgStr;
T_name *ptn;

   lgLBuf = 0;
   lgObj = 0;
   lBuf = loadObjFromFileOffset(0, &lgLBuf, this->pf, this->xrefTable->getOffSetObj(nObj), &lgObj);

   i = findWinBuf("/Kids", lBuf, lgObj);
   if ( i != -1 ) {
      i+= 5;
      while ( (lBuf[i] != ']') && (i < lgObj) ) {
         while ( (lBuf[i] < '0') || (lBuf[i] > '9') )
            ++i;
         this->buildNameTree(atoi(lBuf + i));
         while ( (lBuf[i] != 'R') )
            ++i;
         ++i;
         while ( lBuf[i] <= ' ' )
            ++i;
      }
   }
   i = findWinBuf("/Names", lBuf, lgObj);
   if ( i != -1 ) {
      if ( this->encrypt ) {
         while ( this->encrypt->encryptObj(lBuf, lgObj, lgLBuf, false) == -1 ) {
            delete lBuf;
            lgLBuf += 200;
            lBuf = loadObjFromFileOffset(0, &lgLBuf, this->pf, this->xrefTable->getOffSetObj(nObj), &lgObj);
         }
      }
      i += 6;
      while ( (lBuf[i] != '[') && (i < lgObj) )
         ++i;
      ++i;
      while ( (lBuf[i] != ']') && (i < lgObj) ) {
         while ( lBuf[i] <= ' ' )
            ++i;
         if ( i >= lgObj )
            break;
         ptn = new T_name;
         if ( (lBuf[i] == '(') || (lBuf[i] == '<') ) {
            lgStr = ptn->str.copy(lBuf + i);
            i += lgStr;
            while ( (lBuf[i] != ')') && (lBuf[i] != '>') ) {
               ++i;
            }
         }
         else {
         //il faut aller chercher la chaine dans un sous objet
         char *lBuf2;
         int lgLBuf2, lgObj2, offset2, i2;
            offset2 = this->xrefTable->getOffSetObj(atoi(lBuf + i));
            lBuf2 = loadObjFromFileOffset(0, &lgLBuf2, this->pf, offset2, &lgObj2);
            if ( this->encrypt ) {
               while ( this->encrypt->encryptObj(lBuf2, lgObj2, lgLBuf2, false) == -1 ) {
                  delete lBuf;
                  lgLBuf2 += 200;
                  lBuf2 = loadObjFromFileOffset(0, &lgLBuf2, this->pf, offset2, &lgObj2);
               }
            }
            i2 = 0;
            while ( lBuf2[i2] != '(' )
               ++i2;
            ptn->str.copy(lBuf2 + i2);
            delete lBuf2;
            while ( lBuf[i] != 'R' )
               ++i;
         }
         ptn->next = 0;
         if ( *this->currentTreeRoot == 0 ) {
            *this->currentTreeRoot = ptn;
            this->currentTreeLeaf = ptn;
         }
         else {
            this->currentTreeLeaf->next = ptn;
            this->currentTreeLeaf = ptn;
         }
         while ( (lBuf[i] < '0') || (lBuf[i] > '9') ) {
            ++i;
         }
         ptn->nObj = atoi(lBuf + i);
         while ( lBuf[i] != ' ' ) {
            ++i;
         }
         while ( (lBuf[i] < '0') || (lBuf[i] > '9') ) {
            ++i;
         }
         ptn->version = atoi(lBuf + i);
         while ( lBuf[i] < 'R' ) {
            ++i;
         }
         ++i;
         while ( lBuf[i] <= ' ' )
            ++i;
      }
   }

   if ( lBuf ) {
      delete lBuf;
   }

   return 0;
}

int C_pdfFile::mergeNames(C_pdfFile *orgNames) {
int i;
T_name *ptn1, *ptn2, *ptnNew, *ptn1prev;
int cpt;

   this->namesLoaded = true;
   
   if ( this->n2t ) {
      i = 0;
      while ( this->n2t[i].name != 0 ) {
         if ( *orgNames->n2t[i].tree != 0 ) {
            ptn2 = *(orgNames->n2t[i].tree);
            if ( *(this->n2t[i].tree) != 0 ) {
               ptn1 = *this->n2t[i].tree;
               ptn1prev = ptn1;
               while ( ptn2 ) {
                  while ( ptn1 && (ptn1->str.compare(&ptn2->str) <= 0) ) {
                     ptn1prev = ptn1;
                     ptn1 = ptn1->next;
                  }
                  ptnNew = new T_name;
                  ptnNew->str.copy(&ptn2->str);
                  ptnNew->nObj = this->insertObj(orgNames, ptn2->nObj);
                  ptnNew->version = ptn2->version;
                  if ( ptn1 == *this->n2t[i].tree ) {
                     ptnNew->next = *this->n2t[i].tree;
                     *this->n2t[i].tree = ptnNew;
                  }
                  else {
                     if ( ptn1 == 0 ) {
                        ptn1prev->next = ptnNew;
                        ptnNew->next = 0;
                     }
                     else {
                        ptnNew->next = ptn1;
                        ptn1prev->next = ptnNew;
                     }
                  }
                  ptn1prev = ptnNew;
                  ptn2 = ptn2->next;
               }
            }
            else {
               cpt = 1;
               ptnNew = new T_name;
               ptnNew->str.copy(&ptn2->str);
               ptnNew->nObj = this->insertObj(orgNames, ptn2->nObj);
               ptnNew->version = ptn2->version;
               *(this->n2t[i].tree) = ptnNew;
               ptn1 = ptnNew;
               ptn2 = ptn2->next;
               while ( ptn2 ) {
                  ++cpt;
                  ptnNew = new T_name;
                  ptnNew->str.copy(&ptn2->str);
                  ptnNew->nObj = this->insertObj(orgNames, ptn2->nObj);
                  ptnNew->version = ptn2->version;
                  ptn1->next = ptnNew;
                  ptn1 = ptnNew;
                  ptn2 = ptn2->next;
               }
               ptn1->next = 0;
            }
         }
         ++i;
      }
   }

   return 0;
}

//*****************************************************************************************
//*****************************************************************************************
//*****************************************************************************************

#define MAX_NAMES_BY_LEAF 10
#define LG_LBUF 500
#define ECRIRE_NAME \
   lgStr = ptn->str.snprint(lBuf, LG_LBUF, true); \
   fwrite("(", 1, 1, this->pf); \
   fwrite(lBuf, 1, lgStr, this->pf); \
   fprintf(this->pf, ") %i 0 R\r\n", ptn->nObj);
class T_limits {
public:
    pdfString least;
    pdfString last;
};

int C_pdfFile::flushNames() {
int i, j, k, lgStr;
int nbLeaf;
int nbNames;
T_name *ptn;
int *leafTable;
T_limits *limitsTable;
int nObj;
int cptObj;
int least;
bool flushRootNode;

char *lBuf;

   flushRootNode = false;
   if ( this->n2t ) {
      i = 0;
      lBuf = new char[LG_LBUF];
      while ( (this->n2t[i]).name != 0 ) {
         ptn = *this->n2t[i].tree;
         if ( ptn != 0 ) {
            flushRootNode = true;
            nbNames = 1;
            while ( ptn->next != 0 ) {
               ++nbNames;
               ptn = ptn->next;
            }
            nbLeaf = (int)ceil((double)nbNames / MAX_NAMES_BY_LEAF);
            leafTable = new int[nbLeaf];
            limitsTable = new T_limits[nbLeaf];
            //on écrit les LEAFs.
            ptn = *n2t[i].tree;
            for ( j = 0; j < nbLeaf; j ++ ) {
               leafTable[j] = this->xrefTable->addObj(ftell(this->pf), 0, 'n') - 1;
               fprintf(this->pf, "%i 0 obj \r\n<<\r\n/Names [\r\n", leafTable[j]);
               limitsTable[j].least.copy(&ptn->str);
               for ( k = 0; (k < MAX_NAMES_BY_LEAF - 1) && (ptn->next != 0); k ++) {
                  ECRIRE_NAME
                  ptn = ptn->next;
               }
               limitsTable[j].last.copy(&ptn->str);
               ECRIRE_NAME
               ptn = ptn->next;
               fprintf(this->pf, "]\r\n/Limits [(");
               lgStr = limitsTable[j].least.snprint(lBuf, LG_LBUF, true);
               fwrite(lBuf, 1, lgStr, this->pf);
               fprintf(this->pf, ") (");
               lgStr = limitsTable[j].last.snprint(lBuf, LG_LBUF, true);
               fwrite(lBuf, 1, lgStr, this->pf);
               fprintf(this->pf, ")]\r\n>>\r\nendobj\r\n");
            }
            //on créé l'arbre.
            while (nbLeaf != 1) {
               cptObj = 0;
               for (j = 0; j < nbLeaf; j++) {
                  nObj = this->xrefTable->addObj(ftell(this->pf), 0, 'n') - 1;
                  fprintf(this->pf, "%i 0 obj \r\n<<\r\n/Kids [\r\n", nObj);
                  least = j;
                  for (k = 0; k < MAX_NAMES_BY_LEAF; k++, j++) {
                     if (j == nbLeaf)
                        break;
                     fprintf(this->pf, "%i 0 R\r\n", leafTable[j]);
                  }
                  fprintf(this->pf, "]\r\n/Limits [(");
                  lgStr = limitsTable[least].least.snprint(lBuf, LG_LBUF, true);
                  fwrite(lBuf, 1, lgStr, this->pf);
                  fprintf(this->pf, ") (");
                  lgStr = limitsTable[j - 1].last.snprint(lBuf, LG_LBUF, true);
                  fwrite(lBuf, 1, lgStr, this->pf);
                  fprintf(this->pf, ")]\r\n>>\r\nendobj\r\n");
                  leafTable[cptObj] = nObj;
                  limitsTable[cptObj].least.copy(&limitsTable[least].least);
                  limitsTable[cptObj].last.copy(&limitsTable[j - 1].last);
                  ++cptObj;
               }
               nbLeaf = cptObj;
            }
            n2t[i].rootObj = leafTable[0];

            delete leafTable;
            delete[] limitsTable;
         }
         ++i;
      }
      delete lBuf;

      if ( flushRootNode ) {
         nObj = this->xrefTable->addObj(ftell(this->pf), 0, 'n') - 1;
         this->rootNamesObj = nObj;
         fprintf(this->pf, "%i 0 obj \r\n<<\r\n", nObj);
         i = 0;
         while ( this->n2t[i].name != 0 ) {
            ptn = *this->n2t[i].tree;
            if ( ptn != 0 ) {
                fprintf(this->pf, "/%s %i 0 R\r\n", n2t[i].name, n2t[i].rootObj);
            }
            ++i;
         }
         fprintf(this->pf, ">>\r\nendobj\r\n");
      }

   }

   return 0;
}
#undef LG_LBUF

/*const T_name2tree *pdfNames::getFirstN2T() {
       if ( !this->n2t )
          return 0;
       this->pN2T = 1;
       return (const T_name2tree *)n2t;
}
const T_name2tree *pdfNames::getNextN2T() {
       if ( !this->n2t )
          return 0;
       if ( this->n2t[this->pN2T].name != 0 ) {
          return (const T_name2tree *)&this->n2t[this->pN2T++];
       }
       return 0;
}*/
