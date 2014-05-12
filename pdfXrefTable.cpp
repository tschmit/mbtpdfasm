#include "pdfXrefTable.hpp"
#include "pdfFile.hpp"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mpaMain.hpp"
#include "md5.h"
#include "time.h"
#include "calc.hpp"

#ifdef DEBUG_MEM_LEAK
#ifdef WIN32
   #include <crtdbg.h>
   #define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
   #define new DEBUG_NEW
#endif
#endif


/* **********************************
fonction permettant de trouver la première position de w dans buf de taille lg
*/

int findWinBuf(char *w, char *buf, int lg) {
int i, j, max, lgw;
    
   lgw = strlen(w);
   max = lg - lgw;
   j = 0;
   for (i=0; i <= max; i++) {
      if ( buf[i] == w[j] ) {
         while ( (buf[i + j] == w[j]) && (j < lgw) ) {
            j++;
         }
         if ( j == lgw ) return i;
         j = 0;
      }
   }
   return -1;
}

int findWinPdfObj(char *w, char *buf, int lg) {
int i;

   i = findWinBuf("endobj", buf, lg);
   if ( i == -1 ) {
      i = findWinBuf("stream", buf, lg);
   }
   if ( i == -1 ) return i;
   
   return findWinBuf(w, buf, i);
}


/* ********************************************
         CONSTRUCTEURS

  */

/* ******************************
fonction pour créer la table de référence croisée d'un fichier PDF
fName = nom du fichier
*/

#define STR_TEST_PDF "%PDF"

C_pdfXrefTable::C_pdfXrefTable(char *fName) {
int sizeof_tBuf;
char *tBuf;
int sizeof_fBuf;
char *fBuf;
int pos, posTrailer, i, j;
FILE *f;
char *pc;
bool startXref;
bool v15flag;
C_pdfObject *pPdfObj;
const char *pcc;
int k, l, max;
char * lpc;

   //initialisation
   v15flag = false;
   pPdfObj = NULL;
   nbObj = 0;
   table = NULL;
   root = 0;
   update = false;
   info = 0;
   encrypt = 0;

   lastFreeObject = 0;

   //tests de validité
   f = fopen(fName, "rb");
   if ( !f ) { //erreur à l'ouverture du fichier
      return;
   }
   sizeof_tBuf = LG_TBUF;
   tBuf = new char[sizeof_tBuf];
   fread(tBuf, 8, 1, f);
   if ( strncmp(tBuf, STR_TEST_PDF, strlen(STR_TEST_PDF)) ) {
      delete tBuf;
      fclose(f);
      return; // ce n'est pas un fichier PDF
   }

   this->pdfVersion[0] = tBuf[5];
   this->pdfVersion[1] = '.';
   this->pdfVersion[2] = tBuf[7];
   this->pdfVersion[3] = 0;

   fseek(f, 0, SEEK_END);
   lgF = ftell(f);
   if ( lgF < sizeof_tBuf )
      rewind(f);
   else
      fseek(f, -sizeof_tBuf, SEEK_END); // on se positionne à la fin du fichier
   lgF = fread(tBuf, 1, sizeof_tBuf, f); // lecture de la fin du fichier
   if ( (pos = findWinBuf("startxref", tBuf, lgF)) != -1 ) { //recherche de la position de startxref
      i = 0;
      while ( pos != -1 ) {
         i += pos + sizeof("startxref");
         pos = findWinBuf("startxref", tBuf + i, lgF - i);
      }
      pos = i - sizeof("startxref");
   }
   if ( pos == -1 ) {
      fclose(f);
      delete tBuf;
      return;
   }
   pos = atoi(&tBuf[pos + sizeof("startxref")]); //conversion de la position de xref, 4 = sizeof("xref")

   //recherche de la taille de la table des références croisées
   if ( (posTrailer = findWinBuf("trailer", tBuf, sizeof_tBuf - 10)) != -1 ) {
      i = findWinBuf("/Size", tBuf, sizeof_tBuf - 10);
      if ( i != -1 ) {
         nbObj = atoi(tBuf + i + 6); // 6 = sizeof("/Size ")
      }
      
      table = new typeXref[nbObj];
      sizeof_table = nbObj;
      max = nbObj * sizeof(typeXref);
      lpc = (char *)table;
      for ( k = 0; k < max; ++k )
         *(lpc++) = 0;
   }

   sizeof_fBuf = LG_FBUF;
   fBuf = new char[sizeof_fBuf];

#pragma warning(disable : 4127)
   while ( 1 ) {
#pragma warning(default : 4127)
      fseek(f, pos, SEEK_SET);
      i = fread(fBuf, 1, sizeof_fBuf, f);  // lecture de la table xref
      pc = fBuf;
      while ( (unsigned char)*pc <= ' ' ) 
           ++pc;
       
      if ( pc[0] != 'x' ) {
          //on est eu... c'est un fichier en version > 1.4
          v15flag = true;
          fBuf = loadObjFromFileOffset(fBuf, &sizeof_fBuf, f, pos, &i);
          pPdfObj = new C_pdfObject(fBuf, sizeof_fBuf, xrefObjKeyWords, true);
          if ( !pPdfObj->getLoadStatus() )
              break;
          
          if ( this->nbObj == 0 ) {
              pcc = pPdfObj->getAttrib(__Size);
              if ( pcc != NULL ) {
                  this->nbObj = atoi(pcc);
                  this->table = new typeXref[nbObj];
                  this->sizeof_table = this->nbObj;
                  max = nbObj * sizeof(typeXref);
                  lpc = (char *)table;
                  for ( k = 0; k < max; ++k )
                      *(lpc++) = 0;
              }
          }

          fBuf = pPdfObj->getStream(fBuf, &sizeof_fBuf, &i, true);

          //*******************************************************************************
          //traitement du contenu du stream
          //détermination de la structure du stream:
          int w1, w2, w22, w3, pIndex;
          unsigned char *upc;
          pcc = pPdfObj->getAttrib(__W);
          if ( pcc == NULL ) {
              break;
          }
          j = 0;
          while ( (pcc[j] < '0') || (pcc[j] > '9') )
              ++j;
          w1 = atoi(pcc + j);
          while ( pcc[j] >= '0' && pcc[j] <= '9' )
              ++j;
          while ( (pcc[j] < '0') || (pcc[j] > '9') )
              ++j;
          w2 = atoi(pcc + j);
          while ( pcc[j] >= '0' && pcc[j] <= '9' )
              ++j;
          while ( (pcc[j] < '0') || (pcc[j] > '9') )
              ++j;
          w3 = atoi(pcc + j);

          pcc = pPdfObj->getAttrib(__Index);
          if ( pcc == NULL ) {
              i = 0;
              pcc = pPdfObj->getAttrib(__Size);
              if ( pcc != NULL )
                  j = atoi(pcc);
              else
                  j = nbObj;
              pIndex = -1;
          }
          else {
              pIndex = 0;
              while ( (pcc[pIndex] < '0') || (pcc[pIndex] > '9') )
                 ++pIndex;
              i = atoi(pcc + pIndex);
              while ( pcc[pIndex] >= '0' && pcc[pIndex] <= '9' )
                  ++pIndex;
              while ( (pcc[pIndex] < '0') || (pcc[pIndex] > '9') )
                  ++pIndex;
              j = atoi(pcc + pIndex);
          }
          upc = (unsigned char *)fBuf;
          w22 = w1 + w2 - 1;

          do {
              j += i;
              for (; i < j;i++) {
                  if ( this->table[i].status != 0 ) {
                      upc += (w1 + w2 + w3);
                      continue;
                  }
                  switch ( upc[0]) {
                  case 0:
                      this->table[i].offSet = 0;
                      this->table[i].creat = 0;
                      this->table[i].status = 'f';
                      this->table[i].inserted = 0;
                      this->table[i].compObj = 0;
                      this->table[i].indexCompObj = 0;
                      break;
                  case 1:
                      k = 0;
                      for (l = 0; l < w2; l++) {
                          k += upc[w22 - l] * Power256[l];
                      }
                      this->table[i].offSet = k;
                      this->table[i].creat = 0;
                      this->table[i].status = 'n';
                      this->table[i].inserted = 0;
                      this->table[i].compObj = 0;
                      this->table[i].indexCompObj = 0;
                      break;
                  case 2:
                      k = 0;
                      for (l = 0; l < w2; l++) {
                          k += upc[w22 - l] * Power256[l];
                      }
                      this->table[i].offSet = 0;
                      this->table[i].creat = 0;
                      this->table[i].status = 'n';
                      this->table[i].inserted = 0;
                      this->table[i].compObj = k;
                      this->table[i].indexCompObj = upc[w1 + w2 + w3];
                      break;
                  }
                  upc += (w1 + w2 + w3);
              }

              if ( pIndex != -1 ) {
                  while ( pcc[pIndex] >= '0' && pcc[pIndex] <= '9' )
                      ++pIndex;
                  while ( ((pcc[pIndex] < '0') || (pcc[pIndex] > '9')) && (pcc[pIndex] != ']') )
                      ++pIndex;
                  if ( pcc[pIndex] == ']' )
                      pIndex = -1;
                  else {
                      i = atoi(pcc + pIndex);
                      while ( pcc[pIndex] >= '0' && pcc[pIndex] <= '9' )
                          ++pIndex;
                      while ( (pcc[pIndex] < '0') || (pcc[pIndex] > '9') )
                          ++pIndex;
                      j = atoi(pcc + pIndex);
                  }
              }
          }
          while (pIndex != -1);

          //*******************************************************************************
          if ( this->root == 0 ) {
              pcc = pPdfObj->getAttrib(_Root);
              if ( pcc != NULL ) {
                  root = atoi(pcc);
              }
          }
          if ( this->info == 0 ) {
              pcc = pPdfObj->getAttrib(_Info);
              if ( pcc != NULL ) {
                  this->info = atoi(pcc);
              }
          }
          if ( this->encrypt == 0 ) {
              pcc = pPdfObj->getAttrib(_Encrypt);
              if ( pcc != NULL ) {
                  this->encrypt = atoi(pcc);
              }
          }
          if ( this->ID[0].getLength() == 0 ) {
              j = 0;
              pcc = pPdfObj->getAttrib(_ID);
              if ( pcc != NULL ) {
                  while ( (pcc[j] != '<') && (pcc[j] != '(') )
                      ++j;
                  ID[0].copy(pcc + j);
              }
          }
          pcc = pPdfObj->getAttrib(_Prev);
          if ( pcc == NULL ) {
              break;
          }
          pos = atoi(pcc);          
          delete pPdfObj;
          pPdfObj = NULL;
      }
      else {
          //************************************************************************************************************
          /* *************************************************************************
           * on s'assure que fBuf comporte bien toute la table et le trailer qui suit
           */
          if ( findWinBuf("startxref", fBuf, i) == -1 ) {
          int k;
             startXref = false;
             while ( (*pc < '0') || (*pc > '9') )
                ++pc;
             i = atoi(pc); // conversion du numéro du premier élément décrit
             while (*(pc++) != ' ');
             while ( (*pc < '0') || (*pc > '9') ) // positionnement sur le nombre d'élément dans la sous table
                pc++;
             j = atoi(pc);
             sizeof_fBuf = (j + i) * 20 + SIZEOF_TRAILER;
             while ( !startXref ) { //il faut agrandir fBuf pour pouvoir lire toute la table et le trailer
                startXref = true;
                delete fBuf;
                fBuf = new char[sizeof_fBuf];
                fseek(f, pos, SEEK_SET);
                k = fread(fBuf, 1, sizeof_fBuf, f);
                
                if ( findWinBuf("startxref", fBuf, k) == -1 ) {
                   startXref = false;
                   sizeof_fBuf += SIZEOF_TRAILER;    
                }
             }
             pc = fBuf;
          }

          /* ************************************************************************* */
          while ( (*pc < '0') || (*pc > '9') ) // positionnement sur le nombre d'élément dans la sous table
              pc++;
          while ( (*pc != 't') ) {
              i = atoi(pc); // conversion du numéro du premier élément décrit
              while (*(pc++) != ' ');
              while ( (*pc < '0') || (*pc > '9') ) // positionnement sur le nombre d'élément dans la sous table
                  pc++;
              j = atoi(pc);
              if ( i + j >= nbObj ) { //il faut agrandir table
              typeXref *lpxr;
              int lk;
                  lpxr = new typeXref[i + j];
                  for (lk = 0; lk < this->nbObj; lk++) {
                      lpxr[lk].offSet = table[lk].offSet;
                      lpxr[lk].creat = table[lk].creat;
                      lpxr[lk].status = table[lk].status;
                      lpxr[lk].inserted = table[lk].inserted;
					  lpxr[lk].compObj = table[lk].compObj;
					  lpxr[lk].indexCompObj = table[lk].indexCompObj;
                  }
                  this->nbObj = i + j;
                  for (; lk < this->nbObj; lk++) {
                      lpxr[lk].offSet = 0;
                      lpxr[lk].creat = 0;
                      lpxr[lk].status = 0;
                      lpxr[lk].inserted = 0;
					  lpxr[lk].compObj = 0;
					  lpxr[lk].indexCompObj = 0;
                  }
                  this->sizeof_table = this->nbObj;
                  delete this->table;
                  this->table = lpxr;
                  nbObj = i + j;
              }
              pos = j;
              while ( pos > 0 ) {         //
                  ++pc;                    //
                  pos = (int)(pos / 10);   //positionnement au début de la table
              }                           //
              while ( (*pc < 0x30) || (*pc > 0x39) ) {      //
                  pc++;                    //
              }                           //
              while ( j > 0 ) { //j nombre d'objets dans la sous table
                  if ( table[i].offSet == 0 ) {
                      table[i].offSet = atoi(pc);
                      pc += 11;
                      table[i].creat = atoi(pc);
                      pc += 6;
                      table[i].status = *pc;
                      pc += 3;
                      table[i].inserted = 0;
                  }
                  else {
                      pc += 20;
                  }
                  --j;
                  ++i;
              }
              //bug signalé par Michael Collins, tolérance par rapport au format
              while ( *pc < ' ' ) {
                  ++pc;
              }
          }
          // fin de l'insertion des j objets dans la table
          while ( (*pc != '<') && (*(pc + 1) != '<') ) //
              ++pc;                                     // on se positionne au début du trailer
          pc += 2;                                     //
          pos = 0;
          j = 1;
          // mesure de la taille du trailer
          pos = findWinBuf("startxref", pc, sizeof_fBuf - (pc - fBuf));
          //pos vaut la taille du trailer

          if ( root == 0 ) {
              i = findWinBuf("/Root", pc, pos);
              if ( i != -1 ) {
                  root = atoi(pc + i + 6); // 6 = sizeof("/Root ")
              }
          }
          if ( info == 0 ) {
              i = findWinBuf("/Info", pc, pos);
              if ( i != -1 ) {
                  info = atoi(pc + i + 6); // 6 = sizeof("/Info ")
              }
          }
          if ( encrypt == 0 ) {
              i = findWinBuf("/Encrypt", pc, pos);
              if ( i != -1 ) {
                  encrypt = atoi(pc + i + 9); // 9 = sizeof("/Encrypt ")
              }
          }
          if ( ID[0].getLength() == 0 ) {
              j = findWinBuf("/ID", pc, pos);
              if ( j != -1 ) {
                  //on cherche un tableau de 2 string
                  while ( (pc[j + 3] != '<') && (pc[j + 3] != '(') )
                      ++j;
                  ID[0].copy(pc + j + 3);
              }
          }
          j = findWinBuf("/Prev", pc, pos);
          if ( j == -1 )
              break;
          pos = atoi(pc + j + 6); // 6 = sizeof("/Prev ")
      }
   }

   //fin
   delete tBuf;
   delete fBuf;
   if ( pPdfObj ) 
       delete pPdfObj;
   fclose(f);
}


C_pdfXrefTable::C_pdfXrefTable() {

   update = true;
   info = 0;
   encrypt = 0;
   nbObj = 1;
   table = new typeXref[NEW_TABLE_XREF];
   sizeof_table = NEW_TABLE_XREF;
   table[0].offSet = 0;
   table[0].creat = 65535;
   table[0].status = 'f';
   table[0].inserted = 0;
   root = 0;

   lastFreeObject = 0;

   this->pdfVersion[1] = '.';
   this->pdfVersion[3] = 0;
}


/* ********************************************
         DESTRUCTEURS

  */


C_pdfXrefTable::~C_pdfXrefTable() {
   if ( this->table ) {
      delete this->table;
   }
}


/* ********************************************
         METHODE

  */


int C_pdfXrefTable::addObj(int offset, int creat, char status) {
typeXref *pXref;

   if ( !update )
      return 0;

   if ( sizeof_table == nbObj ) {
      //on agrandit la table des références croisées
      pXref = table;
      sizeof_table = nbObj + NEW_TABLE_XREF;
      table = new typeXref[sizeof_table];
      memcpy(table, pXref, nbObj * sizeof(typeXref));
      delete pXref;
   }

   table[nbObj].offSet = offset;
   table[nbObj].creat = creat;
   table[nbObj].status = status;
   table[nbObj++].inserted = 0;

   return nbObj;
}

int C_pdfXrefTable::removeLastObj() {
   if ( !update )
      return 0;

   return --nbObj;
}

//*********************************

void C_pdfXrefTable::setNumInfo(int i) {
   if ( update )
      info = i;
}

void C_pdfXrefTable::setNumRoot(int i) {
   if ( update )
      root = i;
}

//****************************************************

void C_pdfXrefTable::setOffset(int i, int off) {
   if ( i >= this->nbObj )
      return;
   if ( update ) {
      table[i].offsetBeforeEncrypt = table[i].offSet;
      table[i].offSet = off;
   }
}

//****************************************************

void C_pdfXrefTable::setStatus(int i, char stat) {
   if ( update )
      table[i].status = stat;
}

char C_pdfXrefTable::getStatus(int i) {
   return table[i].status;
}

//*************************************************

int C_pdfXrefTable::flushTable(FILE *f) {
int i, pt;

   if ( !update )
      return 0;

   pt = ftell(f);
   fprintf(f, "xref\r\n0 %u\r\n", nbObj);
   for (i=0; i < nbObj; i++) {
      fprintf(f, "%010u %05u %c\r\n", table[i].offSet, table[i].creat, table[i].status);
   }
   fprintf(f, "trailer\r\n<<\r\n/Size %u\r\n/Root %u 0 R\r\n/Info %u 0 R\r\n",nbObj, root, info);
   if ( this->encrypt != 0 ) {
#define LG_LBUF 255
   char lBuf[LG_LBUF];
      fprintf(f,"/Encrypt %u 0 R\r\n", encrypt);
      ID[0].snprint(lBuf, LG_LBUF, true);
      fprintf(f, "/ID [<%s><%s>]\r\n", lBuf, lBuf);
   }
   fprintf(f,">>\r\nstartxref\r\n%u\r\n%%%%EOF\r\n", pt);

   return 0;
}

//***************************************************

int C_pdfXrefTable::getInserted(int i) {
   return table[i].inserted;
}

int C_pdfXrefTable::setInserted(int n, int i) {
   return table[n].inserted = i;
}

//***************************************************

int C_pdfXrefTable::makeID() {
md5_state_t mst;
time_t t;
unsigned char lBuf[16];

   if ( ID[0].getLength() != 0 ) {
      return 0;
   }

   md5_init(&mst);
   t = time(0);
   md5_append(&mst, (const unsigned char *)&t, sizeof(time_t));
   t = rand();
   md5_append(&mst, (const unsigned char *)&t, sizeof(time_t));
   md5_finish(&mst, lBuf);
   ID[0].copy((const char *)lBuf, 16, _hexaType);
   ID[1].copy((const char *)lBuf, 16, _hexaType);

   //ID[0].copy("<de178789075b64297944260750efb05e>");
   //ID[1].copy("<de178789075b64297944260750efb05e>");

   return 0;
}

const char * C_pdfXrefTable::setPdfVersion(int major, int minor) {
   this->pdfVersion[0] = (char)('0' + (char)major);
   this->pdfVersion[2] = (char)('0' + (char)minor);

   return (const char *)pdfVersion;
}

int C_pdfXrefTable::resetInserted() {
int i;

   for (i = 0; i < this->nbObj; i++) {
      this->table[i].inserted = 0;
   }

   return 0;
}

void C_pdfXrefTable::setFreeObject(int i) {
   this->table[lastFreeObject].offSet = i;
   this->table[i].creat = 1;
   this->table[i].status = 'f';
   lastFreeObject = i;
}
