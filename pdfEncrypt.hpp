#ifndef _PDF_ENCRYPT_HPP
#define _PDF_ENCRYPT_HPP

#include <stdio.h>
#include "pdfXrefTable.hpp"
#include "md5.h"
#include "rc4.hpp"

#define ENCRYPT_FILTER_LENGTH 255
#define ENCRYPT_KEY_LENGTH 255

#define ENCRYPT_FULL_RIGHT      0xFFFFFFFC
#define ENCRYPT_REVISION3_RIGHT 0xFFFFFFC0
#define ENCRYPT_MUST_BE_1       0xFFFFF0C0
#define ENCRYPT_MUST_BE_0       0x00000003

#define ENCRYPT_ALLOW_PRINT     0x00000004
#define ENCRYPT_ALLOW_MODIFY    0x00000008
#define ENCRYPT_ALLOW_EXTRACT   0x00000010
#define ENCRYPT_ALLOW_FILL      0x00000020

#define allowPrint(x)          (x & ENCRYPT_ALLOW_PRINT)
#define allowModifiy(x)        (x & ENCRYPT_ALLOW_MODIFY)
#define allowExtract(x)        (x & ENCRYPT_ALLOW_EXTRACT)
#define allowFill(x)           (x & ENCRYPT_ALLOW_FILL)
#define mostRestrictive(x, y)  (x & y)
#define forceToRevision2(x)    (ENCRYPT_REVISION3_RIGHT | x)

class pdfEncrypt{
private:
   C_pdfXrefTable *xrefTable;
   FILE *pf;

   bool newMdpU;
   char encryptFilter[ENCRYPT_FILTER_LENGTH];
   int encryptV;
   int encryptLength;
   bool canDecrypt;
   int encryptStandardR;
   int encryptStandardP;
   pdfString encryptStandardO;
   pdfString encryptStandardU;
   char encryptKey[ENCRYPT_KEY_LENGTH];
   char mdpO[ENCRYPT_KEY_LENGTH];
   int lgMdpO;
   char mdpU[ENCRYPT_KEY_LENGTH];
   int lgMdpU;
   int encryptKeyLength;

   int getEncryptKey(char *c, int lg);
   int makeEncryptKey(char *, int);
   int encryptBuf(unsigned char *buf, int lgBuf, unsigned char *ckey, int lgCKey);
   bool isMdpUserMdp(char *mdp, int lg);
   bool isMdpOwnerMdp(char *mdp);

public:
   pdfEncrypt(C_pdfXrefTable *xrefTable);
   pdfEncrypt(C_pdfXrefTable *xrefTable, FILE *pf);
   ~pdfEncrypt();

   bool isDecryptPossible();
   int setEncryptRestriction(int i);
   int setEncryptKeyLength(int i);
   int setMdpO(const char *);
   int setMdpU(const char *);
   int setMdpU(const char *mdp, int lg);
   inline FILE *setPf(FILE *f) {return this->pf = f;}
   inline int getEncryptStandardR() { return this->encryptStandardR; }
   inline int getEncryptStandardP() { return this->encryptStandardP; }
   inline int setEncryptStandardP(int i) { return this->encryptStandardP = i; }
   inline int getEncryptLength() { return this->encryptLength;}
   inline int getLgMdpU() {return this->lgMdpU;}
   inline int getEncryptV() {return this->encryptV;}
   inline pdfString *getEncryptStandardO() {return &this->encryptStandardO;}
   inline pdfString *getEncryptStandardU() {return &this->encryptStandardU;}
   inline const char *getEncryptFilter() {return this->encryptFilter;}
   char *getEncryptFilter(char *, int lg);
   bool userProtect();
   bool ownerProtect();

   int encryptPdf(FILE *);
   int encryptObj(char *pc, int lg, int sizeofBuf, bool wholePdfEncryption);
};

#endif
