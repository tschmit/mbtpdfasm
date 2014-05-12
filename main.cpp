#ifdef WIN32
   #pragma warning(disable : 4201)
   #include <crtdbg.h>
//   #include <windows.h>
   #pragma warning(default: 4201)
   #define snprintf _snprintf
#else
   #include <string.h>
   #include <unistd.h>
   #define _snprintf snprintf
   #define stricmp strcasecmp
#endif

#define DLL_EXPORTS
#define RES_BUF_GROWTH 5000
#ifdef MBTPDFASM_DLL
#   include "mbtPdfAsmDll.h"
#endif

#include "mbtPdfAsmError.h"

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

#include "mpaMain.hpp"
#include "string.hpp"

#include "listeFichiers.hpp"
#include "pdfFile.hpp"

#define GET_NUMBER_OF_PAGES 0x0001
#define GET_FILE_NAME       0x0002
#define GET_CYPHER_INFOS    0x0004
#define GET_HEADER_LINE     0x0008
#define GET_META_AUTHOR     0x0010
#define GET_META_KW         0x0020
#define GET_META_SUBJECT    0x0040
#define GET_META_TITLE      0x0080
#define GET_OUTLINES        0x0100

#define MODE_DISPLAY_USE -1
#define MODE_ASSEMBLAGE   1
#define MODE_GET_INFOS    2
#define MODE_UPDATE       4
#define MODE_SPLIT        8
#define MODE_VERSION      16

int displayUse(FILE *output) {
FILE * org;
char tc[1000];
int i;

   fprintf(output, "mbtPdfAsm %s\r\nusing PCRE 4.4 (http://www.pcre.org)\r\nsee at %s", strVersion, strMPAURL);

   return 0;
}

/* *************************************************************************************
retourne le nombre de page insérées, -1 en cas d'erreur
*/

#define NOT_NEXT_ATOME *pc && ((*pc < '0') || (*pc > '9')) && (*pc != '*') && (*pc != '-') && (*pc != 'L')
int insertPageListToFrom(char *pl, C_pdfFile *to, C_pdfFile *from, int startPage, int nbPages, FILE *output) {
char *pc;
int max, min, i, res, vres;

   pc = pl;
   res = 0;
   vres = 0;
   if ( nbPages <= 0 ) {
      nbPages = 100000;
   }
   while ( NOT_NEXT_ATOME )
      ++pc; // on se positionne sur la première page à insérer
   while ( *pc ) {
      switch ( *pc ) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      case 'L':
         if ( *pc == 'L' ) {
            min = from->getNbPages();
            ++pc;
         }
         else {
            min = atoi(pc);
            //fixed by Stephen C. Morgana
            while ( *pc && (*pc >= '0') && (*pc <= '9') )
               ++pc;
         }
         max = min;
         while ( NOT_NEXT_ATOME )
            ++pc;
         if ( *pc == '-' ) {
            max = 0;
            ++pc;
         }
         if ( *pc && !max ) {
            while ( NOT_NEXT_ATOME )
               ++pc;
            switch ( *pc ) {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
               max = atoi(pc);
               while ( *pc && (*pc >= '0') && (*pc <= '9') )
                  ++pc;
               break;
            case '*':
               while ( *pc != '-' )
                  --pc;
               max = min;
               break;
            case 'L':
               ++pc;
               max = from->getNbPages();
               break;
            default:
               max = min;
               break;
            }
         }
         break;
      case '*':
         ++pc;
         min = 1;
         max = from->getNbPages();
         //min = max = -1; option incompatible avec la gestion de l'option -lP
         break;
      case '-':
         ++pc;
         while ( *pc && ((*pc < '0') || (*pc > '9')) && (*pc != '*') )
            ++pc;
         if ( *pc == '*' ) {
            min = from->getNbPages();
            max = 1;
            ++pc;
         }
         else
            min = 0;
         break;
      }
      while ( NOT_NEXT_ATOME ) {
         ++pc;
      }
      if ( min ) {
         if ( min == -1 ) {
         }
         else {
            if ( min <= max ) {
               for (i=min; (i <= max) && (res < nbPages); ++i) {
                  ++vres;
                  if ( vres < startPage )
                     continue;
                  if ( !to->insertPage(from, i, to->getRootPagesObjNum()) ) {
                     if ( output != 0 ) {
                        fprintf(output, strPageInserted, i, from->getFileName());
                     }
                     ++res;
                  }
                  else {
                     if ( output != 0 )
                        fprintf(output, strCantInsertPage, i, from->getFileName());
                  }
               }
            }
            else {
               for (i=min; (i >= max) && (res < nbPages); --i) {
                  if ( ! to->insertPage(from, i, to->getRootPagesObjNum()) ) {
                     ++res;
                     if ( output != 0 )
                        fprintf(output, strPageInserted, i, from->getFileName());
                  }
                  else {
                     if ( output != 0 )
                        fprintf(output, strCantInsertPage, i, from->getFileName());
                  }
               }
            }
         }
      }
   }

   return res;
}


//*************************************************************************************
//*************************************************************************************
//*************************************************************************************

#define SET_PDF_FILE_INSERTION_OPTION \
   if ( bdpOrientation != 0 ) \
      pPdfFile->setBdpOrientation((float)atof(bdpOrientation)); \
   if ( bdpX != 0 ) \
      pPdfFile->setBdpX(bdpX); \
   if ( bdpY != 0 ) \
      pPdfFile->setBdpY(bdpY); \
   if ( bdpSize != 0 ) \
      pPdfFile->setBdpSize(bdpSize); \
   if ( bdpColors != 0 ) \
      pPdfFile->setBdpRGB(bdpColors); \
   if ( bdpFontName != -1 ) \
      pPdfFile->setBdpFontName(bdpFontName); \
   if ( bdpStr != 0 ) \
      pPdfFile->setBdpStr(bdpStr); \
   if ( pnX != 0 ) \
      pPdfFile->setPageNumberX(pnX); \
   if ( pnY != 0 ) \
      pPdfFile->setPageNumberY(pnY); \
   if ( pnSize != 0 ) \
      pPdfFile->setPageNumberSize(pnSize); \
   if ( pnColors != 0 ) \
      pPdfFile->setPageNumberRGB(pnColors); \
   if ( pnFontName != -1 ) \
      pPdfFile->setPageNumberFontName(pnFontName); \
   if ( insertPageNumber) \
      pPdfFile->insertPageNumberInPage(true); \
   if ( pnFirstNum != 0 ) \
      pPdfFile->setPageNumberFirstNum(pnFirstNum); \
   if ( pnOrientation != 0 ) \
      pPdfFile->setPageNumberOrientation((float)atof(pnOrientation)); \
   if ( pageRotation != 0 ) \
      pPdfFile->setPageRotation(pageRotation);


#define MAKE_PDF_FILE_FROM_PAGES_LIST \
for (;;) { \
   i = insertPageListToFrom(p, pPdfFile, pdfOrg, startPage, pageLimitNumber - nbInsertedPages, output); \
   totalPage += i; \
   nbInsertedPages += i; \
   currentPageNumber += i; \
   if ( (nbInsertedPages == pageLimitNumber) && (pageLimitNumber != 0) ) { \
      nbInsertedPages = 0; \
      pPdfFile->close(); \
      delete pPdfFile; \
      ++resultFileCpt; \
      snprintf(tBuf, LG_TBUF - 1, "%s_%03i.pdf", d, resultFileCpt); \
      pPdfFile = new C_pdfFile(tBuf, write); \
      pPdfFile->setOptions(mdpU, mdpO, masterModeNoEncrypt, restrict, lgEncryptKey, pMetadataStr, metadataStr); \
      SET_PDF_FILE_INSERTION_OPTION \
      startPage += i; \
      pdfOrg->getXrefTable()->resetInserted(); \
   } \
   else { \
      startPage = 1; \
      break; \
   } \
   if ( currentPageNumber == pdfOrg->getNbPages() ) { \
      startPage = 1; \
      break; \
   } \
}

//*************************************************************************************
#ifdef MBTPDFASM_DLL
#   define ERROR_FLAGS(x) this->errorFlags |= x;
#   define RES_BUF this->resBuf
#   define RES_BUF_SIZE this->resBufSize
int CSmbtPdfAsm::mbtPdfAsm(int argc, char *argv[]) {
//*************************************************************************************
#else
#   define ERROR_FLAGS(x)
#   define RES_BUF resBuf
#   define RES_BUF_SIZE resBufSize
int main(int argc, char *argv[]) {
char *resBuf = 0;
int resBufSize = 0;
#endif
//*************************************************************************************
C_listeFichiers *lFile;
C_pdfFile *pPdfFile, *pdfOrg;
int mode;
char tBuf[LG_TBUF], *pc;
int res, i, totalPage, j;
int optFL; // options pour file list
char *m, *d, *o, *s; // m = mask, d = destination, o = outlines def, s = fichier de script
char *p; // une liste de pages à insérer
char *b; // répertoire de base
char *bdpStr;
char *bdpOrientation, *bdpColors;
int bdpX, bdpY, bdpSize, bdpFontName;
bool insertPageNumber;
int pnX, pnY, pnSize, pnFontName;
char *pnOrientation, *pnColors;
int pnFirstNum;

char *restrict, *mdpU, *mdpO;
bool masterModeNoEncrypt;

FILE *script;
int sizeof_fBuf;
char *fBuf;
char *MBuf;
   MBuf = 0;

int infoFlags;

int lgEncryptKey;

FILE *output;
   output = stdout;

#ifdef DEBUG_MEM_LEAK
#ifdef WIN32
   _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG)|_CRTDBG_LEAK_CHECK_DF);
   #define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
   #define new DEBUG_NEW
#endif
#endif

#ifdef WIN32
   _fmode = _O_BINARY;
#endif

/* ************************* */
char **metadataStr;
int pMetadataStr;
bool preserveExistingMetadata;
bool keepBakFile;
const char **pcc;

bool keepOutlines;

   pMetadataStr = 0;
   metadataStr = 0;
   preserveExistingMetadata = true;
   keepBakFile = true;
   i = C_pdfFile::getNumberOfMetadata();
   if ( i != 0 )
      metadataStr = (char **)calloc((i + 1) * 2, sizeof(void *));

//************************************
// gestion des limitations d'assemblage
int pageLimitNumber;
int sizeLimitValue;
int resultFileCpt;  //compteur de fichier résultat pour le mode SPLIT
int nbInsertedPages;
int startPage;
int currentPageNumber;

   pageLimitNumber = 0;
   sizeLimitValue  = 0;
   resultFileCpt   = 1;
   //nbInsertedPages = 0;
   //startPage       = 0;

   mode = MODE_ASSEMBLAGE;

   infoFlags = 0;
   masterModeNoEncrypt = false;
   lgEncryptKey = 0;
   optFL = OPTION_LISTEFICHIERS_CASELESS | OPTION_LISTEFICHIERS_SPLIT_MASK;
   m = d = o = s = p = b = restrict = mdpU = mdpO = bdpStr = bdpOrientation = bdpColors = pnOrientation = pnColors= (char *)0;
   pnFirstNum = 0;
   bdpFontName = -1;
   pnFontName = -1;
   bdpX = bdpY = bdpSize = pnX = pnY = pnSize = 0;
   insertPageNumber = false;
   keepOutlines = false;
   lFile = (C_listeFichiers *)0;
   fBuf = (char *)0;

   //*******************************
   //rotation des pages
   int pageRotation = 0;

   //analyse de la ligne de commande
   optFL |= OPTION_LISTEFICHIERS_ORDER_AZ;
   for (i = 1; i < argc; i++) {
      if ( (argv[i][0] == '-') || (argv[i][0] == '/') ) {
         switch ( argv[i][1] ) {
         case 'a': // traitement des masques dans l'ordre de leur apparition
            optFL = optFL & ((OPTION_LISTEFICHIERS_ORDER_AZ | OPTION_LISTEFICHIERS_ORDER_ZA) ^ 0xffffffff);
            if ( argv[i][2] == 's' ) {
                optFL |= OPTION_LISTEFICHIERS_ORDER_MASK;
            }
            break;
         case 'b': //spécifier un répertoire de base
            b = &argv[i][2];
            break;
         case 'c': //cryptage du fichier résultat
            switch ( argv[i][2] ) {
            case 'L': //mot de passe utilisateur
               lgEncryptKey = atoi(&argv[i][3]);
               if ((lgEncryptKey < 5) || (lgEncryptKey > 16))
                   lgEncryptKey = 5;
               break;
            case 'R': //restrictions: m pour modif, p pour impression, s select, a annotations/formulaires
               restrict = &argv[i][3];
               break;
            case 'U': //mot de passe utilisateur
               mdpU = &argv[i][3];
               break;
            case 'O': //mot de passe propriétaire
               mdpO = &argv[i][3];
               break;
            }
            break;
         case 'd': //fichier résultat
            d = &argv[i][2];
            break;
         case 'g': //obtenir une information sur les fichiers du masques
            j = 2;
            while ( argv[i][j] != 0 ) {
                switch ( argv[i][j] ) {
                    case 'A':
                        infoFlags |= GET_META_AUTHOR;
                        break;
                    case 'C':
                        infoFlags |= GET_CYPHER_INFOS;
                        break;
                    case 'F':
                        infoFlags |= GET_FILE_NAME;
                        break;
                    case 'H':
                        infoFlags |= GET_HEADER_LINE;
                        break;
                    case 'K':
                        infoFlags |= GET_META_KW;
                        break;
                    case 'N':
                        infoFlags |= GET_NUMBER_OF_PAGES;
                         break;
				    case 'O':
						//keepOutlines = true;
                        infoFlags |= GET_OUTLINES;
                         break;
                    case 'S':
                        infoFlags |= GET_META_SUBJECT;
                        break;
                    case 'T':
                        infoFlags |= GET_META_TITLE;
                        break;
                }
                ++j;
            }
            break;
         case 'h':
            mode = MODE_DISPLAY_USE;
            i = argc;
            break;
         case 'l': // mode limitation de taille
            mode = MODE_SPLIT;
            switch ( argv[i][2] ) {
            case 'P':
               pageLimitNumber = atoi(&argv[i][3]);
               break;
            case 'S':
               sizeLimitValue = atoi(&argv[i][3]);
               break;
            default:
               mode = MODE_DISPLAY_USE;
               i = argc;
            }
            break;
         case 'm': // masque
            m = &argv[i][2];
            break;
         case 'M': //on dégrade les regexp
            if (MBuf == 0) {
               MBuf = new char[LG_FBUF];
               pc = &argv[i][2];
               j = 1;
               MBuf[0] = '^';
               while ( *pc ) {
                  switch ( *pc ) {
                  case '.':
                     MBuf[j++] = '\\';
                     MBuf[j++] = '.';
                     break;
                  case '*':
                     MBuf[j++] = '.';
                     MBuf[j++] = '*';
                     break;
                  case '?':
                     MBuf[j++] = '.';
                     break;
                  case ';':
                  case ',':
                     MBuf[j++] = '$';
                     MBuf[j++] = ';';
                     MBuf[j++] = '^';
                     break;
                  default :
                     MBuf[j++] = *pc;
                  }
                  ++pc;
                  if ( j > LG_FBUF - 3 )
                     break;
               }
               MBuf[j++] = '$';
               MBuf[j] = 0;
               //if ( output != 0 )
               //   fprintf(output, "regexp = %s\r\n", MBuf);
               m = MBuf;
            }
            break;
         case 'n':
            insertPageNumber = true;
            break;
         case 'N':
            switch ( argv[i][2] ) {
            case '0':
               pnFirstNum = atoi(argv[i] + 3);
               break;
            case 'c':
               pnColors = argv[i] + 3;
               break;
            case 'f':
               pnFontName = atoi(argv[i] + 3);
               break;
            case 'o':
               pnOrientation = argv[i] + 3;
               break;
            case 's':
               pnSize = atoi(argv[i] + 3);
               break;
            case 'x':
               pnX = atoi(argv[i] + 3);
               break;
            case 'y':
               pnY = atoi(argv[i] + 3);
               break;
            }
            break;
         case 'o': // outlines
            switch ( argv[i][2] ) {
            case 'O':
               keepOutlines = true;
               break;
            default:
               o = &argv[i][2];
               break;
            }
            break;
         case 'p': // listes de pages à extraire du fichier
            p = &argv[i][2];
            pc = p;
            while ( *pc ) {
               if ( (*pc == ';') || (*pc == ',') ) {
                  *pc = ' ';
               }
               ++pc;
            }
            break;
         case 'r':
            optFL |= OPTION_LISTEFICHIERS_RECURSE_SUBDIR;
            break;
         case 'R':
            pageRotation = atoi(argv[i] + 2);
            if ( (pageRotation != 90) && (pageRotation != 180) && (pageRotation != 270) ) {
               pageRotation = 0;
            }
            break;
         case 's': // script ou mise à jour des metadatas.
            if ( (argv[i][2] == 'A') || (argv[i][2] == 'K') || (argv[i][2] == 'S') || (argv[i][2] == 'T') ) {
               switch ( argv[i][2] ) {
               case 'A':
                  metadataStr[pMetadataStr ++] = "/Author";
                  break;
               case 'K':
                  metadataStr[pMetadataStr ++] = "/Keywords";
                  break;
               case 'S':
                  metadataStr[pMetadataStr ++] = "/Subject";
                  break;
               case 'T':
                  metadataStr[pMetadataStr ++] = "/Title";
                  break;
               }
               metadataStr[pMetadataStr ++] = &argv[i][3];
               pc = &argv[i][3];
               while ( *pc ) {
                  if ( *pc == '_' ) {
                     *pc = ' ';
                  }
                  ++pc;
               }
            }
            else {
               s = &argv[i][2];
               break;
            }
            break;
         case 'S':
            output = 0;
            break;
         case 't':
            bdpStr = &argv[i][2];
            break;
         case 'T':
            switch ( argv[i][2] ) {
            case 'c':
               bdpColors = argv[i] + 3;
               break;
            case 'f':
               bdpFontName = atoi(argv[i] + 3);
               break;
            case 'o':
               bdpOrientation = argv[i] + 3;
               break;
            case 's':
               bdpSize = atoi(argv[i] + 3);
               break;
            case 'x':
               bdpX = atoi(argv[i] + 3);
               break;
            case 'y':
               bdpY = atoi(argv[i] + 3);
               break;
            }
            break;
         case 'u':
            mode = MODE_UPDATE;
            pc = &argv[i][2];
            while ( *pc ) {
               switch ( *pc ) {
               case 'K':
                  keepBakFile = false;
                  break;
               case 'P':
                  preserveExistingMetadata = false;
                  break;
               }
               ++pc;
            }
            break;
         case 'v': // affichage de la version
            i = argc;
            mode = MODE_VERSION;
            break;
         case 'x':
            masterModeNoEncrypt = true;
            break;
         case 'z': // ordre inverse des fichiers
            optFL |= OPTION_LISTEFICHIERS_ORDER_ZA;
            optFL = optFL & ((OPTION_LISTEFICHIERS_ORDER_AZ | OPTION_LISTEFICHIERS_ORDER_MASK) ^ 0xffffffff);
            break;
         default:
            mode = MODE_DISPLAY_USE;
            i = argc;
         }
      }
      else {
         // le commutateur de la ligne de commande n'est pas valide
         mode = MODE_DISPLAY_USE;
         i = argc;
      }
   }

   //test de la ligne de commande
   if ( argc < 2 ) {
      if ( output != 0 )
         displayUse(output);
      ERROR_FLAGS(ERROR_ARGC_LESS_THAN_2)
      return RETURN_ARGC_LESS_THAN_2;
   }

   if ( infoFlags != 0 )
      mode = MODE_GET_INFOS;

   C_listeFichiers::setPath(".\\");

   totalPage = 0;

   switch ( mode ) {
   case MODE_DISPLAY_USE:
      ERROR_FLAGS(ERROR_DISPLAY_USE)
      if ( output != 0 )
         displayUse(output);
      break;
   //**********************************************************************************************
   case MODE_ASSEMBLAGE:
   case MODE_SPLIT:
      if ( !((d && m) || (d && s)) ) {
         if ( output != 0 )
            displayUse(output);
         break;
      }

      if ( output != 0 )
         fprintf(output, "mbtPdfAsm version %s\r\n", strVersion);

      //initialisation du fichier résultat
      if ( pageLimitNumber != 0 ) {
           i = strlen(d);
		   //corrigé par Bernard Dautrevaux
           if ( i > 4 && stricmp(d+(i - 4), ".pdf") == 0 )
              d[i - 4] = 0;
           snprintf(tBuf, LG_TBUF - 1, "%s_%03i.pdf", d, 1);
         if ( p == 0 )
            p = "*";
      }
      else {
         snprintf(tBuf, LG_TBUF - 1, "%s", d);
      }
      pPdfFile = new C_pdfFile(tBuf, write);

      if ( keepOutlines ) {
         pPdfFile->setKeepOutlines(true);
      }

      if ( pPdfFile && (pPdfFile->getStatus()) ) {
         pPdfFile->setOptions(mdpU, mdpO, masterModeNoEncrypt, restrict, lgEncryptKey, pMetadataStr, metadataStr);

         SET_PDF_FILE_INSERTION_OPTION

         startPage = 1;
         nbInsertedPages = 0;
      }
      else {
         if ( output != 0 )
            fprintf(output, strResImp, d);
         m = 0;
         s = 0;
         o = 0;
      }


      if ( m ) { // recherche des fichiers selon le mask
         lFile = new C_listeFichiers(b, m, d, optFL);
         if ( lFile->getNbFichiers() > 0 ) {
            if ( p ) { //********** traitement en lot d'une liste de pages
               lFile->firstFile(tBuf, LG_TBUF - 1);
               do {
                  currentPageNumber = 0;
                  if ( output != 0 )
                     fprintf(output, strDebTrait, tBuf);

                  pdfOrg = new C_pdfFile(tBuf, read);
                  if ( !pdfOrg || !pdfOrg->getStatus() ) { //on peut pas ouvrir le fichier pdf
                     ERROR_FLAGS(ERROR_UNABLE_TO_OPEN_ORG_PDF_FILE)
                     if ( output != 0 )
                        fprintf(output, strErrFile, tBuf);
                  }
                  else { //fichier pdf d'origine ouvert
                     if ( pdfOrg->getXrefTable()->getEncryptObj() != 0 ) {
                        if ( !pdfOrg->encrypt->isDecryptPossible() ) {
                           ERROR_FLAGS(ERROR_UNABLE_TO_DECRYPT_PDF_FILE)
                           if ( output != 0 )
                              fprintf(output, "impossible de déchiffrer le fichier %s\r\n", fBuf);
                           delete pdfOrg;
                           pdfOrg = 0;
                           continue;
                        }
                     }
                     MAKE_PDF_FILE_FROM_PAGES_LIST
                  }
                  if ( pdfOrg ) {
                     delete pdfOrg;
                     pdfOrg = 0;
                  }

               } while (lFile->nextFile(tBuf, LG_TBUF - 1));
            }
            else {
               lFile->firstFile(tBuf, LG_TBUF - 1);
               do {
                  if ( (res = pPdfFile->merge(tBuf)) != -1 ) {
                     totalPage += res;
                     if ( output != 0 )
                        fprintf(output, strAjout, tBuf, res);
                  }
                  else {
                     res = pPdfFile->getMergeError();
                     switch ( res ) {
                     case PDF_MERGE_CANT_MERGE_READ_ONLY_FILE:
                        pc = "impossible de merger un fichier non vide\r\n";
                        break;
                     case PDF_MERGE_CANT_OPEN_ORG_FILE:
                        pc = (char *)strErrFile;
                        break;
                     case PDF_MERGE_CANT_MERGE_ENCRYPTED_FILE:
                        pc = (char *)strErrEncrypt;
                        break;
                     case PDF_MERGE_CANT_HANDLE_ENCRYPT_VERSION:
                        pc = (char *)strErrEncryptVer;
                        break;
                     default:
                        pc = (char *)strErrFile;
                        break;
                     }
                     if ( output != 0 )
                        fprintf(output, pc, tBuf);
                  }
               }
               while (lFile->nextFile(tBuf, LG_TBUF - 1));
            }

            lFile->closeListe();
         }
         else {
            ERROR_FLAGS(ERROR_NO_MATCHING_FILE)
            if ( output != 0 )
               fprintf(output, strNoMFile);
         }
      }

      if ( s ) { // interpretation du script
      char *startFileName;

         script = fopen(s, "rb");
         if ( !script ) {
            ERROR_FLAGS(ERROR_UNABLE_TO_OPEN_SCRIPT)
            if ( output != 0 )
               fprintf(output, strCantOpenScript, s);
         }
         else {
            sizeof_fBuf = 4 * LGMAX_FILE_NAME;
            fBuf = new char[sizeof_fBuf];
            while ( fgets(fBuf, sizeof_fBuf, script) ) {
               pc = fBuf;
               startFileName = fBuf;
               if ( *pc < ' ')
                  continue;
               char end;
			   if ( (*pc == '"') || (*pc == '\'') ) {
                  end = *pc++;
                  ++startFileName;
               }
               else {
				   end = ' ';
               }
			   while ( (*pc != end) && ((pc - fBuf) < sizeof_fBuf) && ((unsigned char)(*pc) >= ' ') ) {
				   ++pc;
			   }
               *pc++ = 0; //il faut avancer pc pour lire la liste des pages à a assembler.
               pdfOrg = new C_pdfFile(startFileName, read);
               if ( !pdfOrg || !pdfOrg->getStatus() ) { //on peut pas ouvrir le fichier pdf
                  ERROR_FLAGS(ERROR_UNABLE_TO_OPEN_ORG_PDF_FILE_SCRIPT)
                  if ( output != 0 )
                     fprintf(output, strErrFile, fBuf);
                  if ( pdfOrg ) {
                     delete pdfOrg;
                  }
               }
               else { //fichier pdf d'origine ouvert
               char *lpc;
                  lpc = pc;
                  while ( *lpc >= ' ' )
                     ++lpc;
                  *lpc = 0;
                  lpc = pc;
                  while ( *lpc >= ' ' ) {
                     if ( *lpc == '-' ) {
                        if ( (*(lpc + 1) == 'c') && (*(lpc + 2) == 'U') ) {
                           pdfOrg->encrypt->setMdpU(lpc + 3);
                           *(lpc - 1) = 0;
                           break;
                        }
                     }
                     ++lpc;
                  }
                  if ( pdfOrg->getXrefTable()->getEncryptObj() != 0 ) {
                     if ( pdfOrg->encrypt->isDecryptPossible() ) {
                        pPdfFile->setEncryptStandardP(mostRestrictive(pPdfFile->getEncryptStandardP(), pdfOrg->getEncryptStandardP()));
                     }
                     else {
                        ERROR_FLAGS(ERROR_UNABLE_TO_DECRYPT_PDF_FILE_SCRIPT)
                        if ( output != 0 )
                           fprintf(output, "impossible de déchiffrer le fichier %s\r\n", fBuf);
                        delete pdfOrg;
                        continue;
                     }
                  }
                  p = pc;
                  MAKE_PDF_FILE_FROM_PAGES_LIST
                  pdfOrg->close();
                  delete pdfOrg;
               }
            }
            fclose(script);
         }
      }

      if ( (pageLimitNumber != 0) && (nbInsertedPages == 0) ) {
         pPdfFile->close();
         remove(pPdfFile->getFileName());
         delete pPdfFile;
         pPdfFile = 0;
         //--resultFileCpt;
      }

      //traitement du fichier de définition des outlines

      if ( (o != 0) && (mode != MODE_SPLIT) ) {
         if ( pPdfFile->loadOutlines(o) ) {
            if ( pPdfFile->flushOutlines() ) {
               if ( output != 0 )
                  fprintf(output, "signets integres\r\n");
            }
            else {
               ERROR_FLAGS(ERROR_UNABLE_TO_FLUSH_OUTLINES)
               if ( output != 0 )
                  fprintf(output, strErrSignet);
            }
         }
         else {
            ERROR_FLAGS(ERROR_UNABLE_TO_LOAD_OUTLINES)
            if ( output != 0 )
               fprintf(output, "impossible de charger %s\r\n", o);
         }
      }

      // fin de la création du fichier PDF
      if ( pPdfFile ) {
         pPdfFile->close();
         delete pPdfFile;
      }

      if ( output != 0 )
         fprintf(output, strFinAsm, totalPage);
      break;
   /* ********************************************************************** */
   case MODE_GET_INFOS:
   FILE *infOutput;

      if ( !(m && (infoFlags != 0)) ) {
          if ( output != 0 )
             displayUse(output);
          break;
      }

      infOutput = output;
      if ( infOutput == 0 )
         infOutput = stdout;

      if ( RES_BUF == 0 ) {
         RES_BUF_SIZE = RES_BUF_GROWTH;
         RES_BUF = new char[RES_BUF_SIZE];
		 RES_BUF[0] = 0;
      }

      i = 0; 
      if ( infoFlags & GET_HEADER_LINE ) {
         if ( infoFlags & GET_FILE_NAME )
            i += snprintf(RES_BUF + i, RES_BUF_SIZE - i, "%s;", strHLFileName);
         if ( infoFlags & GET_NUMBER_OF_PAGES )
            i += snprintf(RES_BUF + i, RES_BUF_SIZE - i, "%s;", strHLNumberOfPages);
         if ( infoFlags & GET_META_AUTHOR )
            i += snprintf(RES_BUF + i, RES_BUF_SIZE - i, "%s;", strHLAuthor);
         if ( infoFlags & GET_META_KW )
            i += snprintf(RES_BUF + i, RES_BUF_SIZE - i, "%s;", strHLKW);
         if ( infoFlags & GET_META_SUBJECT )
            i += snprintf(RES_BUF + i, RES_BUF_SIZE - i, "%s;", strHLSubject);
         if ( infoFlags & GET_META_TITLE )
            i += snprintf(RES_BUF + i, RES_BUF_SIZE - i, "%s;", strHLTitle);
         if ( infoFlags & GET_CYPHER_INFOS )
            i += snprintf(RES_BUF + i, RES_BUF_SIZE - i, "%s;%s;%s;", strHLEFilter, strHLEKeyLength, strHLUserMdp);
         fprintf(infOutput, "%s\r\n", RES_BUF);
         i = 0;
      }

      lFile = new C_listeFichiers(b, m, 0, optFL);
      if ( lFile->getNbFichiers() > 0 ) {
         lFile->firstFile(tBuf, LG_TBUF - 1);
         do {
            if (infoFlags & GET_FILE_NAME)
               i += snprintf(RES_BUF + i, RES_BUF_SIZE - i, "%s;", tBuf);
            pdfOrg = new C_pdfFile(tBuf, read);
            if ( pdfOrg != 0 ) {
               if (infoFlags & GET_NUMBER_OF_PAGES)
                  i += snprintf(RES_BUF + i, RES_BUF_SIZE - i, "%i;", pdfOrg->getNbPages());
               if (infoFlags & GET_META_AUTHOR ) {
                  pdfOrg->getMetadataStr(tBuf, LG_TBUF - 1, "/Author");
                  i += snprintf(RES_BUF + i, RES_BUF_SIZE - i, "%s;", tBuf);
               }
               if (infoFlags & GET_META_KW ) {
                  pdfOrg->getMetadataStr(tBuf, LG_TBUF - 1, "/Keywords");
                  i += snprintf(RES_BUF + i, RES_BUF_SIZE - i, "%s;", tBuf);
               }
               if (infoFlags & GET_META_SUBJECT ) {
                  pdfOrg->getMetadataStr(tBuf, LG_TBUF - 1, "/Subject");
                  i += snprintf(RES_BUF + i, RES_BUF_SIZE - i, "%s;", tBuf);
               }
               if (infoFlags & GET_META_TITLE ) {
                  pdfOrg->getMetadataStr(tBuf, LG_TBUF - 1, "/Title");
                  i += snprintf(RES_BUF + i, RES_BUF_SIZE - i, "%s;", tBuf);
               }
               if (infoFlags & GET_CYPHER_INFOS) {
                  i += snprintf(RES_BUF + i, RES_BUF_SIZE - i, "%s;%i;%i;%i;", pdfOrg->encrypt->getEncryptFilter(tBuf, LG_TBUF - 1), pdfOrg->encrypt->getEncryptLength(), pdfOrg->encrypt->userProtect());
               }
			   fprintf(infOutput, "%s", RES_BUF);
               
			   if (infoFlags & GET_OUTLINES) {
				   pdfOrg->flushFormatedOutlinesToDest(OUTLINES_FLUSH_FORMAT_MBT, infOutput);
				   fprintf(infOutput, ";");
               }

               delete pdfOrg;
            }
			fprintf(infOutput, "\r\n");
            i = 0;
         } while (lFile->nextFile(tBuf, LG_TBUF - 1));
         lFile->closeListe();
      }
      else {
         //i += snprintf(RES_BUF + i, RES_BUF_SIZE - i, "0");
         snprintf(RES_BUF + i, RES_BUF_SIZE - i, "0");
         fprintf(infOutput, "0");
      }
      break;
   //*****************************************************************************
   case MODE_UPDATE:
      if ( !m ) {
         if ( output != 0 )
            displayUse(output);
         break;
      }

      lFile = new C_listeFichiers(b, m, "pdfbak", optFL);

      if ( lFile->getNbFichiers() > 0 ) {
      char tBuf2[LG_TBUF];

         lFile->firstFile(tBuf, LG_TBUF - 1);
         do {
            _snprintf(tBuf2, LG_TBUF - 1, "%s.pdfbak", tBuf);
            i = 1;
            if ( rename(tBuf, tBuf2) != 0 ) {
               i = 0;
               remove(tBuf2);
               if ( rename(tBuf, tBuf2) == 0 ) {
                  i = 1;
               }
            }
            if ( i ) {
               pPdfFile = new C_pdfFile(tBuf, write);
               pPdfFile->setOptions(mdpU, mdpO, masterModeNoEncrypt, restrict, lgEncryptKey, pMetadataStr, metadataStr);
               if ( preserveExistingMetadata ) {
#define LG_METADATA_BUF 500
                  pdfOrg = new C_pdfFile(tBuf2, read);
                  pcc = C_pdfFile::pdfMetadataStrName;
                  if ( fBuf == 0 ) {
                     sizeof_fBuf = LG_METADATA_BUF * 2;
                     fBuf = new char[sizeof_fBuf];
                  }
                  while ( *pcc != 0 ) {
                     if ( (i = pdfOrg->getMetadataStr(fBuf, LG_METADATA_BUF - 1, *pcc)) != 0 ) {
                        if ( pPdfFile->getMetadataStr(fBuf + LG_METADATA_BUF, LG_METADATA_BUF - 1, *pcc) == 0 ) {
                           pPdfFile->setMetadataStr(*pcc, fBuf, i);
                        }

                     }
                     ++pcc;
                  }
                  delete pdfOrg;
               }
               //pPdfFile->merge(tBuf2);
               pPdfFile->fullMerge(tBuf2);
               //pPdfFile->close();
               delete pPdfFile;
               if ( !keepBakFile ) {
                  remove(tBuf2);
               }
               if ( output != 0 )
                  fprintf(output, strUpdateRes, tBuf);
            }
            else {
               ERROR_FLAGS(ERROR_DURING_UPDATE)
               if ( output != 0 )
                  fprintf(output, strUpdateResErr, tBuf);
            }
         } while (lFile->nextFile(tBuf, LG_TBUF - 1));
         lFile->closeListe();
      }
      else {
         ERROR_FLAGS(ERROR_NO_MATCHING_FILE_UPDATE)
         if ( output != 0 )
            fprintf(output, strNoMFile);
      }
      break;
      //***********************************************************************************************
   case MODE_VERSION:
      if ( RES_BUF == 0 ) {
         RES_BUF_SIZE = RES_BUF_GROWTH;
         RES_BUF = new char[RES_BUF_SIZE];
      }
      snprintf(RES_BUF, RES_BUF_SIZE, "%s", strVersion);
      if ( output != 0 )
         fprintf(output, "%s\r\n", RES_BUF);
      break;
   }

   if ( lFile ) {
      delete lFile;
   }
   if ( fBuf ) {
      delete fBuf;
   }

   if ( metadataStr )
      free(metadataStr);

   if ( MBuf )
      delete MBuf;

#ifndef MBTPDFASM_DLL
   if ( RES_BUF )
      delete RES_BUF;
#endif

   return totalPage;
}
