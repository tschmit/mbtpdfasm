





#include "listeFichiers.hpp"
#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/time.h>


int C_listeFichiers::nFL = 0;
char C_listeFichiers::path[] = "";

C_listeFichiers::C_listeFichiers(char *rep, char *m, char *em, int o) {
int cpt, i, j;
T_tri *pt;
char *pc, *lBuf;
bool go;
bool b;
struct timeval tv1;
struct timezone tz;
    
fprintf(stderr, "constructeur liste fichier\r\n");

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
    
fprintf(stderr, "nbMask: %i\r\n", this->nbMask);

   cpt = 0;
   if ( rep ) {
      while ( rep[cpt] && (cpt < LGMAX_FILE_NAME) ) {
         baseRep[cpt] = rep[cpt++];
      }
      baseRep[cpt] = 0;
      if (cpt > 0) cpt--; else { baseRep[0] = '.'; baseRep[1] = '/'; baseRep[2] = 0;}
      if ( cpt && (rep[cpt] != '/') ) {
         baseRep[++cpt] = '/';
         baseRep[++cpt] = 0;
      }
   }
   else { baseRep[0] = '.'; baseRep[1] = '/'; baseRep[2] = 0;}
   gettimeofday(&tv1, &tz);
   
   snprintf(listeName, LGMAX_FILE_NAME, "%s/listeFichier_%i.tmp", path, nFL + tv1.tv_usec);
   hFile = fopen(listeName, "w");
   if ( hFile ) {
      nbFichiers = build(baseRep);
      for (i = 1; i < this->nbMask; i++) {
          if ( (j = strlen(lFile[i].c_str())) > 0 ) {
              //lFile[i][j - 2] = 0; // pas dans la version windows ???
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
      hFile = fopen(listeName, "r");
      //on charge la liste en mémoire
      pt = new T_tri[nbFichiers];
      for (i = 0; i < nbFichiers; i++) {
         fgets(pt[i].val, LG_PATH_LISTE_FICHIERS - 1, hFile);
         pc = pt[i].val;
         while ( *(pc++) > 0x1F );
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
      hFile = fopen(listeName, "w");
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
   remove(this->listeName);
}

//**************************************************************************
//
int C_listeFichiers::build(char *rep) {
char rep2[LGMAX_FILE_NAME];
char rep3[LGMAX_FILE_NAME];
char *pc;
int cpt;
int hListFile;
DIR *dirp;
struct dirent *direntp;
struct stat pStat;

   cpt = 0;
   dirp = opendir(rep);
    if ( dirp != NULL ) {
        for (;;) {
            direntp = readdir(dirp);
            if ( direntp == NULL ) break;
            snprintf(rep2, LGMAX_FILE_NAME, "%s%s", rep, direntp->d_name);
            if ( stat(rep2, &pStat) == -1 ) {
                fprintf(stderr, "stat not allowed with %s\r\n", direntp->d_name);
                break;
            }
            if ( S_ISDIR(pStat.st_mode) ) {
                if ( (options & OPTION_LISTEFICHIERS_RECURSE_SUBDIR) && (*(direntp->d_name) != '.') ) {
                    snprintf(rep3, LGMAX_FILE_NAME, "%s/", rep2);
                    cpt += build(rep3);
                }
            }
            else {
                if ( (this->lMask[0]->doesStrMatchMask(direntp->d_name) == 1) && ((exMask == 0) || !(this->exMask->doesStrMatchMask(direntp->d_name) == 1)) ) {
   	                fprintf(hFile, "%s%s\r\n", rep, direntp->d_name);
                    ++cpt;
                }
                for (int i= 1; i < this->nbMask; i++ ) {
fprintf(stderr, "testing: %s\r\n", direntp->d_name);
                    if ( (this->lMask[i]->doesStrMatchMask(direntp->d_name) == 1) && ((exMask == 0) || !(this->exMask->doesStrMatchMask(direntp->d_name) == 1)) ) {
                        snprintf(rep3, LGMAX_FILE_NAME, "%s\r\n", rep2);
                        lFile[i] += rep3;
                        ++cpt;
                    }
                }
   	        }
        }
    }
    closedir(dirp);

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
   if (hFile)
      fclose(hFile);
   hFile = NULL;
}

//**************************************************************************

int C_listeFichiers::setPath(const char *p) {
int i;
   for (i=0; (i < LG_PATH_LISTE_FICHIERS) && (p[i] != 0); i++)
      path[i] = p[i];
   if ( i < LG_PATH_LISTE_FICHIERS ) {
      if ( (path[i-1] == '/') || (path[i-1] == '\\') )
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

