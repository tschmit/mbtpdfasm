#ifndef __PDFXREFTABLE__
#define __PDFXREFTABLE__

#pragma warning(disable: 4514)

#include <stdio.h>
#include "pdfString.hpp"

#define LG_TBUF 1050
#define LG_FBUF 5000

#define NEW_TABLE_XREF 500

//taille estimée du trailer
#define SIZEOF_TRAILER 250

int findWinBuf(char *w, char *buf, int lg);
int findWinPdfObj(char *w, char *buf, int lg);

typedef struct {
   int offSet;
   int offsetBeforeEncrypt;
   int creat;
   char status;
   int inserted;
   int compObj;
   int indexCompObj;
} typeXref;

class C_pdfXrefTable {
   private:
      int nbObj;
      typeXref *table;
      int sizeof_table;
      int root, info, encrypt;
      int lgF;
      bool update; //la table peut être mise à jour
      pdfString ID[2];
      char pdfVersion[4];

      int lastFreeObject;

   public:
      C_pdfXrefTable(char *fName);
      C_pdfXrefTable();
      ~C_pdfXrefTable();

      inline int getRootObj() {return root;}
      inline int getInfoObj() {return info;}
      inline int getEncryptObj() {return encrypt;} // vaut 0 si le fichier n'a pas de restriction
      inline int getNbObj() {return nbObj;}

      inline int getOffSetObj(int i) {if (i < nbObj) return table[i].offSet; return 0;}
      inline int getOffsetObjBeforeEncrypt(int i) {if (i < nbObj) return table[i].offsetBeforeEncrypt; return 0;}
      inline int getCompObj(int i) {if (i < nbObj) return table[i].compObj; return 0;}
      inline int getIndexCompObj(int i) {if (i < nbObj) return table[i].indexCompObj; return 0;}

      inline pdfString *getID(int i) {if (i < 2) return this->ID + i; return (pdfString *)0;}
      int makeID();

      inline void setEncrypt(int i) {this->encrypt = i;}

      int addObj(int offset, int creat, char status);
      int removeLastObj();
      void setNumInfo(int i);
      void setNumRoot(int i);
      void setOffset(int i, int off);
      void setStatus(int i, char stat);
      void setFreeObject(int i);
      char getStatus(int i);
      int getInserted(int i);
      int setInserted(int n, int i);
      int flushTable(FILE *f);

      inline const char *getPdfVersion() {return (const char *)this->pdfVersion; }
      const char *setPdfVersion(int, int);

      int resetInserted();
};

#endif
