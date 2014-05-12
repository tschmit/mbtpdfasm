#include "listeFichiers.hpp"
#include <io.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#ifdef DEBUG_MEM_LEAK
#ifdef WIN32
   #include <crtdbg.h>
   #define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
   #define new DEBUG_NEW
#endif
#endif

int C_listeFichiers::nFL = 0;
char C_listeFichiers::path[] = "";

C_listeFichiers::C_listeFichiers(char *rep, char *m, char *em, int o) {
int cpt, i, j;
T_tri *pt;
bool go;
char *pc, *lBuf;
bool b;

   
    
   nFL++;
   this->options = o;
   this->strmOptions = NO_OPTION;

   b = true;
   if ( this->options & OPTION_LISTEFICHIERS_SPLIT_MASK )
       b = false;

   if ( this->options & OPTION_LISTEFICHIERS_CASELESS ) {
      this->strmOptions |= STRM_OPTION_CASELESS;
   }

   if ( !(m && (m[0] != 0)) ) {
       m = ".*";
   }

       lBuf = new char[strlen(m) + 1];
       strcpy(lBuf, m);
       pc = lBuf;
       this->nbMask = 1;
       if ( this->options & OPTION_LISTEFICHIERS_ORDER_MASK ) {
           while ( *pc != 0 ) {
               if ( (*pc == ';') || (*pc == ',') ) {
                   *pc = 0;
                   if ( *(pc + 1) != 0 ) {
                       ++this->nbMask;
                   }
               }
               ++pc;
           }
       }
       this->lMask = new strMatcher * [this->nbMask];
       this->lFile = new string[this->nbMask];
       pc = lBuf;
       for (i = 0; i < this->nbMask; i++) {
           lMask[i] = new strMatcher(pc, b, this->strmOptions);
           while ( *pc != 0 )
               ++pc;
           ++pc;
       }

       delete lBuf;

   if ( em && (em[0] != 0)) {
       this->exMask = new strMatcher(em, b, this->strmOptions);
   }
   else
       exMask = 0;

    cpt = 0;
    if ( rep ) {
        while ( rep[cpt] && (cpt < LGMAX_FILE_NAME) ) {
            baseRep[cpt] = rep[cpt++];
        }
        baseRep[cpt] = 0;
        if (cpt > 0) cpt--; else { baseRep[0] = '.'; baseRep[1] = '\\'; baseRep[2] = 0;};
        if ( cpt && (rep[cpt] != '\\') ) {
            baseRep[++cpt] = '\\';
            baseRep[++cpt] = 0;
        }
    }
    else { baseRep[0] = '.'; baseRep[1] = '\\'; baseRep[2] = 0;}
    time_t t;
    time(&t);
    _snprintf(listeName, LGMAX_FILE_NAME, "%s\\listeFichier_%i.tmp", path, t);
    hFile = fopen(listeName, "wb");
    if ( hFile ) {
        nbFichiers = build(baseRep);
        for (i = 1; i < this->nbMask; i++) {
            if ( (j = strlen(lFile[i].c_str())) > 0 ) {
                
                fprintf(hFile, "%s", lFile[i].c_str());
            }
        }
        fclose(hFile);
        hFile = (FILE *)0;
    }
    else {
        fprintf(stdout, "impossible d'ouvire le fichier temporaire %s\n", listeName);
        nbFichiers = 0;
    }

   // TRI **************************
   if ( (options & (OPTION_LISTEFICHIERS_ORDER_AZ | OPTION_LISTEFICHIERS_ORDER_ZA)) && (nbFichiers > 1)) {
      hFile = fopen(listeName, "rb");
      //on charge la liste en mémoire
      pt = new T_tri[nbFichiers];
      for (i = 0; i < nbFichiers; i++) {
         fgets(pt[i].val, LG_PATH_LISTE_FICHIERS - 1, hFile);
         pc = pt[i].val;
         while ( *(pc++) > 0x1f );
         *(--pc) = 0;
         pt[i].i = i;
      }
      fclose(hFile);
      if ( options & OPTION_LISTEFICHIERS_ORDER_AZ ) {
         do {
            go = false;
            for (i = 0; i < nbFichiers - 1; i++) {
               if ( strcmp(pt[pt[i].i].val, pt[pt[i+1].i].val) > 0 ) {
                  go = true;
                  j = pt[i+1].i;
                  pt[i+1].i = pt[i].i;
                  pt[i].i = j;
               }
            }
         } while (go);
      }
      else {
         do {
            go = false;
            for (i = 0; i < nbFichiers - 1; i++) {
               if ( strcmp(pt[pt[i].i].val, pt[pt[i+1].i].val) < 0 ) {
                  go = true;
                  j = pt[i+1].i;
                  pt[i+1].i = pt[i].i;
                  pt[i].i = j;
               }
            }
         } while (go);
      }
      // flush du fichier trier
      hFile = fopen(listeName, "wb");
      for (i = 0; i < nbFichiers; i++)
         fprintf(hFile, "%s\r\n", pt[pt[i].i].val);
      delete pt;
      fclose(hFile);
   }
   hFile = NULL;
}

//**************************************************************************

C_listeFichiers::~C_listeFichiers() {
   --this->nFL;

   if ( this->lMask != 0 ) {
       for (int i = 0; i < this->nbMask; i++ ) {
           delete lMask[i];
       }
       delete[] this->lMask;
   }
   if ( this->lFile != 0 ) {
       delete[] this->lFile;
   }

   if ( this->exMask != 0 )
       delete this->exMask;

   if ( this->hFile ) fclose(this->hFile);
   remove(listeName);
}

//**************************************************************************

int C_listeFichiers::build(char *rep) {
char rep1[LGMAX_FILE_NAME];
char rep3[LGMAX_FILE_NAME];
struct _finddata_t fName;
int cpt;
int hListFile;

   cpt = 0;
   _snprintf(rep1, LGMAX_FILE_NAME, "%s*.*", rep);
   if ( (hListFile = _findfirst(rep1, &fName)) != -1 ) {
      for (;;) {
         if ( fName.attrib & _A_SUBDIR ) {
            if ( (options & OPTION_LISTEFICHIERS_RECURSE_SUBDIR) && ( fName.name[0] != '.') ) {
                  _snprintf(rep3, LGMAX_FILE_NAME, "%s%s\\", rep, fName.name);
                  cpt += build(rep3);
            }
         }
         else {
            if ( (this->lMask[0]->doesStrMatchMask(fName.name) == 1) && ((exMask == 0) || !(this->exMask->doesStrMatchMask(fName.name) == 1)) ) {
               fprintf(hFile, "%s%s\r\n", rep, fName.name);
               ++cpt;
            }
            for (int i= 1; i < this->nbMask; i++ ) {
                if ( (this->lMask[i]->doesStrMatchMask(fName.name) == 1) && ((exMask == 0) || !(this->exMask->doesStrMatchMask(fName.name) == 1)) ) {
                   _snprintf(rep3, LGMAX_FILE_NAME, "%s%s\r\n", rep, fName.name);
                   lFile[i] += rep3;
                   ++cpt;
                }
            }
         }
         if ( _findnext(hListFile, &fName) ) break;

      }
      _findclose(hListFile);
   }

   return cpt;
}

//**************************************************************************

char *C_listeFichiers::firstFile(char *buf, int lgBuf) {
unsigned char *pc;

   if ( hFile )
      rewind(hFile);
   else
      hFile = fopen(listeName, "r");
   if ( fgets(buf, lgBuf, hFile) == NULL )
      return NULL;
   pc = (unsigned char *)buf;
   while ( *(pc++) > 0x1F );
   *(--pc) = 0;

   return buf;
}

char *C_listeFichiers::nextFile(char *buf, int lgBuf) {
unsigned char *pc;

   if ( fgets(buf, lgBuf, hFile) == NULL )
      return NULL;
   pc = (unsigned char *)buf;
   while ( *(pc++) > 0x1F );
   *(--pc) = 0;

   return buf;
}

void C_listeFichiers::closeListe() {
   if (hFile) {
      fclose(hFile);
   }
   hFile = NULL;
}

//**************************************************************************

int C_listeFichiers::setPath(const char *p) {
int i;
   for (i=0; (i < LG_PATH_LISTE_FICHIERS) && (p[i] != 0); i++)
      path[i] = p[i];
   if ( i < LG_PATH_LISTE_FICHIERS ) {
      if ( (path[i-i] == '/') || (path[i-1] == '\\') )
         --i;
      path[i] = 0;
      return i;
   }

   return -1;
}

char *C_listeFichiers::getPath(char *p, int lg) {
int i;
   for (i=0; (i < lg) && (path[i] != 0); i++)
      p[i] = path[i];
   if ( i < lg ) {
      p[i] = 0;
      return p;
   }
   return NULL;
}
