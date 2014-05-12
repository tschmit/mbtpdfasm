#ifndef __PDFOBJ_HPP__
#define __PDFOBJ_HPP__

#include <stdio.h>
#include <string.h>

#include "diversPdf.hpp"

typedef struct S_attrib {
   const char *nom;       // nom de l'attribut
   int id;
   char *valeur;          // valeur de l'attribut
   int lgValeur;
   struct S_attrib *next; //attribut suivant dans la liste
} T_attrib;

extern int compareAttribToAttrib(T_attrib *attrib1, bool chain1, T_attrib *attrib2, bool chain2, bool skipIndirectRef);

class C_pdfObject {
private:
   T_attrib *listAttrib;
   char *stream;
   int lgStream;     // longueur du stream
   int sizeofStream; // longueur du buffer contenant le stream
   C_pdfObject *next;

   bool loadStatus; //true si le chargement de l'objet depuis un buffer s'est bien faite
   
   int numObj;

   int initVar();

public:
   C_pdfObject();
   C_pdfObject(char *pBuf, int lgPBuf, T_keyWord *pKWT, bool loadStream); //charge un objet à partir d'un buffer
   C_pdfObject(C_pdfObject *prev);
   ~C_pdfObject();

   int insertAttrib(T_attrib *pta);
   int insertAttrib(const char *nom, char *valeur, int lgValeur, int id);
   int appendStream(char *, int);
   int flush(FILE *dest, int num);

   const char *getAttrib(int idAttrib);

   char *getStream(char *destBuf, int *lgDestBuf, int *lgStream, bool filter);

   inline bool getLoadStatus() {return this->loadStatus;};

   char *getObjectFromCompObj(int k, char *destBuf, int *lgDestBuf, int *lgObj);
   inline int getNumObj() {return this->numObj;}
   inline int setNumObj(int i) {return this->numObj = i;}

   inline C_pdfObject *getNext() {return this->next;}
   
   int chainObjectToThis(C_pdfObject *ppo);
   
   int compareThisToSingleObject(C_pdfObject *ppo, bool chain);
};

#endif
