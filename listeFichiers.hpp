#ifndef LISTEFICHIERS_HPP
#define LISTEFICHIERS_HPP

#pragma warning(disable: 4514)

#include <string>
using namespace std;

#include <stdio.h>

#include "strMatcher.hpp"

#define LGMAX_FILE_NAME 5000

#define LG_PATH_LISTE_FICHIERS 5000

#define OPTION_LISTEFICHIERS_RECURSE_SUBDIR 0x01
#define OPTION_LISTEFICHIERS_ORDER_AZ       0x02
#define OPTION_LISTEFICHIERS_ORDER_ZA       0x04
#define OPTION_LISTEFICHIERS_SPLIT_MASK     0x08
#define OPTION_LISTEFICHIERS_CASELESS       0x10
#define OPTION_LISTEFICHIERS_ORDER_MASK     0x20

//structure utilisée pour le tri
typedef struct {
   int i;
   char val[LG_PATH_LISTE_FICHIERS];
} T_tri;

class C_listeFichiers {
private:
   static int nFL;
   static char path[LG_PATH_LISTE_FICHIERS]; // c'est là que sont créé les fichiers de liste de fichiers.

   int build(char *rep);
   int build(char *rep, char *locMask);
   int nbFichiers; //nombre de fichier dans la listes
   int options;
   int strmOptions;
   char baseRep[LGMAX_FILE_NAME];
   strMatcher *exMask;
   char listeName[LGMAX_FILE_NAME];
   FILE *hFile;

   strMatcher **lMask;
   string *lFile;
   int nbMask;

public:
   C_listeFichiers(char *rep, char *m, char *em, int o);
   ~C_listeFichiers();

   char *firstFile(char *buf, int lgBuf);
   char *nextFile(char *buf, int lgBuf);
   void closeListe();
   inline int getNbFichiers() {return nbFichiers;};

   static int setPath(const char *);
   static char *getPath(char *, int lg);
};

#endif //LISTEFICHIERS_HPP

