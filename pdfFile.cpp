#include "pdfFile.hpp"
#include "pdfObject.hpp"
#include "diversPdf.hpp"
#include "mpaMain.hpp"
#include "string.hpp"
#include <time.h>
#include <math.h>

#ifdef DEBUG_MEM_LEAK
#ifdef WIN32
   #include <crtdbg.h>
   #define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
   #define new DEBUG_NEW
#endif
#endif


#ifdef WIN32
#define snprintf _snprintf
#endif

const char *C_pdfFile::pdfMetadataStrName[] = {
   "/Title",
   "/Author",
   "/Subject",
   "/Keywords",
   "/Creator",
   "/CreationDate",
   "/ModDate",
   0
};

#define SIZEOF_AF_LIST 200

/* ***************************************************************************************** */

char *C_pdfFile::debug_pc = 0;

#define LG_PDFNAME_BUF 500
C_pdfFile::C_pdfFile(char *nom, openMode mode) {
int i, j, lgObj, lgCatalog, k;
char *fBuf, *pc;
int sizeof_fBuf = LG_FBUF;
char *pdfNameBuf;
int sizeof_pdfNameBuf;
char **pcc;
C_pdfObject *pPdfObj;

   fBuf = new char[sizeof_fBuf];

   pPdfObj = 0;

   strncpy(this->nom, nom, LG_NOM - 1);
   this->mode = mode;
   status = true;
   rootPages = 0;
   xrefTable = (C_pdfXrefTable *)0;
   pf = (FILE *)0;

   //gestion des pages ****************************************
   nbPages = 0;
   pagesList = 0;
   pPage = -1;
   rootTreeNode = 0;
   childs = (int *)0;
   sizeof_childs = 0;
   pChilds = 0;

   this->pageRotation = 0;
   
   this->bdpStr  = 0;
   this->bdpObj  = 0;
   this->bdpFontObj = 0;
   this->bdpOrientation = 0.0;
   this->bdpX = 10;
   this->bdpY = 10;
   this->bdpSize = 10;
   this->bdpRgb = this->bdprGb = this->bdprgB = 0.0;
   this->bdpFontName = HELVETICA_FONT;
   
   this->pageNumberFontObj = 0;
   this->insertPageNumber = false;
   this->pageNumberX = 10;
   this->pageNumberY = 10;
   this->pageNumberSize = 10;
   this->pageNumberOrientation = 0.0;
   this->pageNumberFontName = HELVETICA_FONT;
   this->pageNumberRgb = this->pageNumberrGb = this->pageNumberrgB = 0.0;
   this->pageNumberFirstNum = 0;

   //gestion des outlines **************************************
   this->keepOutlines = false;
   this->outlines = 0;
   this->nbOutlines = 0;
   this->firstOutlines = 0;
   this->lastOutlines = 0;
   this->countOutlines = 0;
   this->outlinesLoaded = false;
   this->outlinesFlushed = false;
   this->pOutlines = (T_outlines *)0;
   this->sizeof_pOutlines = 0;

   this->rootAcroForm = 0;
   this->lgAfList = SIZEOF_AF_LIST;
   this->afList = new int[lgAfList];
   this->pAfList = 0;
   this->afList[0] = 0;
   this->acroDR = 0;

   this->closed = false;

   //gestion de l'encryption
   this->encrypt = 0;
   this->forceNoEncrypt = false;

   // gestion de l'option -u
   this->fullMergeFlag = false;

   //gestion des names
   this->Dests = 0;
   this->AP = 0;
   this->JavaScript = 0;
   this->Pages = 0;
   this->Templates = 0;
   this->IDS = 0;
   this->URLS = 0;
   this->namesLoaded = false;
   this->rootNamesObj = 0;

   pcc = (char **)nameOfTrees;
   i = 0;
   while ( pcc[i][0] != 0 )
      ++i;
   this->n2t = new T_name2tree[i + 1];
   i = 0;
   while ( pcc[i][0] != 0 ) {
      this->n2t[i].name = nameOfTrees[i];
      this->n2t[i].rootObj = 0;
      switch ( nameOfTrees[i][0] ) { //ceci est une implementation de feignant...
      case 'A':
         this->n2t[i].tree = &this->AP;
         break;
      case 'I':
         this->n2t[i].tree = &this->IDS;
         break;
      case 'D':
         this->n2t[i].tree = &this->Dests;
         break;
      case 'J':
         this->n2t[i].tree = &this->JavaScript;
         break;
      case 'P':
         this->n2t[i].tree = &this->Pages;
         break;
      case 'T':
         this->n2t[i].tree = &this->Templates;
         break;
      case 'U':
         this->n2t[i].tree = &this->URLS;
         break;
      }
      ++i;
   }
   this->n2t[i].name = 0;
   this->n2t[i].tree = 0;

   //  gestion des metadata
   this->pdfMetadataStrValue = 0;
   i = 0;
   while ( this->pdfMetadataStrName[i] != 0 )
       ++i;
   this->pdfMetadataStrValue = (pdfString **)malloc(sizeof(void *) * i);
   --i;
   for (; i >= 0; i--) {
      this->pdfMetadataStrValue[i] = new pdfString();
   }

   //gestion des polices de caractères (font)
   this->fontList = 0;
   /* *********************** */

   switch (mode) {
   case read:
      xrefTable = new C_pdfXrefTable(nom);
      if ( xrefTable ) {
         if ( !xrefTable->getNbObj() ) {
            delete xrefTable;
            xrefTable = (C_pdfXrefTable *)0;
            break;
         }
      }
      else
         break;

      //ouverture du flux de donnée PDF.
      this->pf = fopen(nom, "rb");
      if ( !this->pf )
         break;

      //gestion de l'encryption
      this->encrypt = new pdfEncrypt(this->xrefTable, this->pf);

      fBuf = this->getObj(this->xrefTable->getRootObj(), fBuf, &sizeof_fBuf, &lgCatalog, &pPdfObj, true);
      if ( lgCatalog == 0 ) {
          break;
      }
      if ( this->xrefTable->getEncryptObj() != 0 ) {
         this->encrypt->encryptObj(fBuf, lgCatalog + 6, LG_FBUF, false);
      }

      i = findWinBuf("/Pages", fBuf, lgCatalog);
      if ( i == -1 )
         break;
      rootPages = atoi(fBuf + i + 7); // 7 = sizeof("/Pages ")

      i = findWinBuf("/Outlines", fBuf, lgCatalog);
      if ( i != -1 )
         this->outlines = atoi(fBuf + i + 10); // 10 = sizeof("/Outlines ")

      //**********************************************************************************************************
      //**********************************************************************************************************
      i = findWinBuf("/Names", fBuf, lgCatalog);
      if ( i != -1 ) {
          sizeof_pdfNameBuf = LG_PDFNAME_BUF;
          pdfNameBuf = new char[sizeof_pdfNameBuf];
          this->rootNamesObj = atoi(fBuf + i + 7);
          pdfNameBuf = this->getObj(this->rootNamesObj, pdfNameBuf, &sizeof_pdfNameBuf, &lgObj, &pPdfObj, false);
          if ( lgObj == 0 )
              return;
          lgObj += 6;
          i = 0;
          while ( this->n2t[i].name != 0 ) {
             j = findWinBuf((char *)this->n2t[i].name, pdfNameBuf, lgObj);
             if ( j != -1 ) {
                while ((pdfNameBuf[j] != ' ') && (j < lgObj) ) {
                   ++j;
                }
                while ((pdfNameBuf[j] == ' ') && (j < lgObj) ) {
                   ++j;
                }
                if ( j < lgObj ) {
                   k = atoi(pdfNameBuf + j);
                   this->currentTreeRoot = this->n2t[i].tree;
                   this->buildNameTree(k);
                   this->n2t[i].rootObj = k;
                }
             }
             ++i;
          }
          this->namesLoaded = true;
          delete pdfNameBuf;
       }

      //**********************************************************************************************************
      //**********************************************************************************************************
      //recherche d'une racine pour les formulaires
      i = findWinBuf("/AcroForm", fBuf, lgCatalog);
      if ( i != - 1 ) {
          if (*(fBuf + i + 9) == ' ' ) {
              this->rootAcroForm = atoi(fBuf + i + 9);
              i = xrefTable->getOffSetObj(this->rootAcroForm);
              fseek(this->pf, i, SEEK_SET);
              fread(fBuf, 1, LG_FBUF, pf);
              i = findWinBuf("endobj", fBuf, sizeof_fBuf);
              while ( i == - 1 ) {
                  pc = new char[sizeof_fBuf + LG_FBUF];
                  memcpy(pc, fBuf, sizeof_fBuf);
                  fread(fBuf + sizeof_fBuf, 1, LG_FBUF, pf);
                  delete fBuf;
                  fBuf = pc;
                  sizeof_fBuf += LG_FBUF;
                  i = findWinBuf("endobj", fBuf, sizeof_fBuf);
              }
              if ( xrefTable->getEncryptObj() != 0 ) {
                 this->encrypt->encryptObj(fBuf, i + 6, LG_FBUF, false);
              }
              pc = fBuf;
              while ( *pc != '<' )
                  ++pc;
          }
          else {
              this->rootAcroForm = -1;
              //on a à faire à un dictionnaire
              i += 9;
              while ( fBuf[i] != '<' ) {
                 ++i;
              }
              pc = fBuf + i;
          }
          this->buildAcroFormTree(pc);
      }

      //**********************************************************************************************************
      //**********************************************************************************************************
      //on lit le nombre de pages dans le document
      fBuf = this->getObj(rootPages, fBuf, &sizeof_fBuf, &lgObj, &pPdfObj, false);
      i = findWinBuf("/Count", fBuf, sizeof_fBuf);
	  nbPages = atoi(fBuf + i + 7); // 7 = sizeof("/Count ")
	  if ( X_X_R.doesStrMatchMask(fBuf + i + 7) ) {
         fBuf = this->getObj(nbPages, fBuf, &sizeof_fBuf, &lgObj, &pPdfObj, false);
		 i = findWinBuf("obj", fBuf, sizeof_fBuf);
	     nbPages = atoi(fBuf + i + 3); // 3 = sizeof("obj")
	  }

      /* *********************************** */
      fBuf = this->getObj(this->xrefTable->getInfoObj(), fBuf, &sizeof_fBuf, &lgObj, &pPdfObj, false);
      if ( lgObj != 0 ) {
      char **pcc;
         if ( xrefTable->getEncryptObj() != 0 ) {
            this->encrypt->encryptObj(fBuf, lgObj + 6, LG_FBUF, false);
         }
         pcc = (char **)C_pdfFile::pdfMetadataStrName;
         j = 0;
         while ( pcc[j] != 0 ) {

			 if ( (i = findWinBuf(pcc[j], fBuf, lgObj)) != -1 ) {
               i += strlen(pcc[j]);
               while ( fBuf[i] == ' ' )
                  ++i;
               this->pdfMetadataStrValue[j]->copy(fBuf + i);
            }
            ++j;
         }
      }

      //****************************************************************************************************************
      //****************************************************************************************************************
      /* *********************************** */
      if ( this->outlines != 0 ) {
         pOutlinesGrowth(0);
         
         fBuf = this->getObj(this->outlines, fBuf, &sizeof_fBuf, &lgObj, &pPdfObj, false);

         if ( (i = findWinBuf("/First", fBuf, lgObj)) != -1 ) {
            this->firstOutlines = atoi(fBuf + i + 7);
         }
         if ( (i = findWinBuf("/Last", fBuf, lgObj)) != -1 ) {
            this->lastOutlines = atoi(fBuf + i + 6);
         }
         if ( (i = findWinBuf("/Count", fBuf, lgObj)) != -1 ) {
            this->countOutlines = atoi(fBuf + i + 7);
         }

         this->nbOutlines = readOutlines(this->firstOutlines);

      }

      /* *********************************** */
      
      break;
   case write:
      childs = new int[NB_CHILDS];
      sizeof_childs = NB_CHILDS;
      xrefTable = new C_pdfXrefTable();
      pf = fopen(this->nom, "wb+");
      if ( !pf )
         break;
      //en-tête du fichier PDF
      this->xrefTable->setPdfVersion(1, 4);
      fwrite("%PDF-1.4\r\n%éàèù\r\n",1, 17, this->pf);

      rootPages = xrefTable->addObj(0, 0, 'n') - 1; //attention, il faut mettre à jour l'offset à la validation du fichier

       break;
   }
   if ( !xrefTable || !pf ) status = false;

   if ( pPdfObj )
       delete pPdfObj;
   delete fBuf;
}
#undef LG_PDFNAME_BUF

//**************************************************************************************
//**************************************************************************************

void deleteTreeNode(T_pageTreeNode *o) {
int i;
T_attrib *pta1, *pta2;
   if ( o == 0 )
      return;
   if ( o->nbKids != 0 ) {
      for ( i = 0; i < o->nbKids; i++) {
         deleteTreeNode(o->kids[i]);
      }
      delete o->kids;
   }
   pta1 = o->listAttrib;
   while ( pta1 != 0 ) {
      delete pta1->valeur;
      pta2 = pta1->next;
      delete pta1;
      pta1 = pta2;

   }
   delete o;
}

C_pdfFile::~C_pdfFile() {
T_name *ptn, *ptnNext;
int i;
   // libération de la mémoire
   this->close();

   if ( xrefTable ) {
      delete xrefTable;
   }
   if ( this->encrypt ) {
      delete this->encrypt;
   }
   if ( childs ) {
      delete childs;
   }
   if ( pagesList ) {
      delete[] pagesList;
   }
   if ( afList ) {
      delete afList;
   }
   if ( pOutlines ) {
      for (i=0; i < this->sizeof_pOutlines; i++) {
         if ( pOutlines[i].titre ) {
            delete pOutlines[i].titre;
         }
      }
      delete pOutlines;
   }
   if ( this->n2t ) {
      i = 0;
      while ( this->n2t[i].name != 0 ) {
         if ( *this->n2t[i].tree != 0 ) {
            ptn = *this->n2t[i].tree;
            ptnNext = ptn->next;
#pragma warning (disable : 4127)
            while ( 1 )  {
#pragma warning (default : 4127)
               delete ptn;
               ptn = ptnNext;
               if ( ptn == 0 )
                  break;
               ptnNext = ptn->next;
            }
         }
         ++i;
      }
      delete this->n2t;
   }
   if ( this->pf != NULL ) {
      fclose(this->pf);
      this->pf = NULL;
   }
   if ( this->pdfMetadataStrValue != NULL ) {
      i  = 0;
      while ( this->pdfMetadataStrName[i] != 0 ) {
         delete this->pdfMetadataStrValue[i++];
      }
      free(this->pdfMetadataStrValue);
      this->pdfMetadataStrValue = 0;
   }
   if ( this->rootTreeNode != 0 ) {
      deleteTreeNode(this->rootTreeNode);
   }

   if ( this->fontList != 0 )  {
   C_pdfObject *ppo;
	   while ( this->fontList ) {
		   ppo = this->fontList;
		   this->fontList = this->fontList->getNext();
           delete ppo;
	   }
   }
}

/* ***************************************************************** */
/* ***************************************************************** */
/* ***************************************************************
                    METHODES
*/

int C_pdfFile::setMetadataStr(const char *name, char *org, int lgOrg) {
int i;

   i = 0;
   while ( this->pdfMetadataStrName[i] != 0 ) {
      if ( strcmp(name, this->pdfMetadataStrName[i]) == 0 )
         break;
      ++i;
   }
   if ( this->pdfMetadataStrName[i] != 0 ) {
      this->pdfMetadataStrValue[i]->copy(org, lgOrg, _carType);
      return i;
   }
   return -1;
}

/* ***************************************************************** */

int C_pdfFile::getNumberOfMetadata() {
int i;

   i = 0;
   while ( pdfMetadataStrName[i] != 0 ) 
      ++i;

   return i;
}

/* ***************************************************************** */

int C_pdfFile::getMetadataStr(char *dest, int lgDest, const char *metaName) {
int i;

   i = 0;
   while ( strcmp(pdfMetadataStrName[i], metaName) != 0 ) {
      ++i;
      if ( pdfMetadataStrName[i] == 0 )
         break;
   }
   if ( pdfMetadataStrName[i] == 0 )
      return 0;

   return this->pdfMetadataStrValue[i]->snprint(dest, lgDest, false);
}

/* ***************************************************************** */
/* ***************************************************************** */
/* ***************************************************************** */

#define TEMPORARY_CYPHER_NAME "%s.%i.pdf", this->nom, random
#define LG_LBUF 255

bool C_pdfFile::close() {
int np, i, random;
char lBuf[LG_LBUF + 1];

   if ( !status )
      return false;

   if ( closed )
      return true;

   random = 0;
   if ( mode == write ) {

      //élément info du fichier PDF /Info
      i = xrefTable->addObj(ftell(pf), 0, 'n') - 1;
      xrefTable->setNumInfo(i);
      fprintf(pf, "%u 0 obj\r\n<<\r\n/Producer (mbt PDF assembleur version %s)\r\n", i, strVersion);
      i = 0;
      while ( this->pdfMetadataStrName[i] != 0 ) {
         if ( this->pdfMetadataStrValue[i]->getLength() != 0 ) {
            this->pdfMetadataStrValue[i]->snprint(lBuf, LG_LBUF, true);
            fprintf(pf, "%s (%s)\r\n", this->pdfMetadataStrName[i], lBuf);
         }
         ++i;
      }
      fprintf(pf, ">>\r\nendobj\r\n");

      if ( !this->fullMergeFlag ) {
         //écriture de /Pages
         np = ftell(pf);
         fprintf(pf, "%u 0 obj\r\n<<\r\n/Type /Pages\r\n/Count %u\r\n", rootPages, nbPages);
         if ( this->pageRotation != 0 ) {
            fprintf(pf, "/Rotate %u\r\n", this->pageRotation);
         }
         fprintf(pf, "/Kids [\r\n");
         for (i=0; i < pChilds; i++) {
            fprintf(pf, "   %u 0 R\r\n", childs[i]);
         }
         fprintf(pf, "]\r\n>>\r\nendobj\r\n");
         xrefTable->setOffset(rootPages, np);

         //écriture de /AcroForm.
         if ( this->rootAcroForm != 0 ) {
             np = ftell(pf);
             xrefTable->setOffset(this->rootAcroForm, np);
             fprintf(pf, "%u 0 obj\r\n<<\r\n /Fields [ ", this->rootAcroForm);
             i = 0;
             while (this->afList[i] != 0) {
                 fprintf(pf, "%u 0 R ", this->afList[i]);
                 ++i;
             }
             fprintf(pf, "]\r\n");
             if ( this->acroDA.getLength() != 0 ) {
                 this->acroDA.snprint(lBuf, LG_LBUF - 1, true);
                 fprintf(pf, "/DA (%s)\r\n", lBuf);

             }
             if ( this->acroDR != 0 ) {
                fprintf(pf, "/DR %i 0 R\r\n", this->acroDR);
             }
             fprintf(pf, ">>\r\nendobj\r\n");
         }

         if ( this->namesLoaded ) {
            this->flushNames();
         }

         if ( this->nbOutlines != 0 )
            this->flushOutlines();

         //écriture du catalogue
         np = ftell(pf);
         np = xrefTable->addObj(np, 0, 'n') - 1;
         xrefTable->setNumRoot(np);
         fprintf(pf, "%u 0 obj\r\n<<\r\n/Type /Catalog\r\n/Pages %u 0 R\r\n", np, this->rootPages);
         if ( this->rootNamesObj != 0 ) {
             if ( this->getRootNamesObj() != 0 ) {
                fprintf(pf, "/Names %u 0 R\r\n", this->getRootNamesObj());
             }
         }
         if ( this->rootAcroForm != 0 ) {
             fprintf(pf, "/AcroForm %u 0 R\r\n", this->rootAcroForm);
         }
         if ( outlinesFlushed ) {
            fprintf(pf, "/PageMode /UseOutlines\r\n/Outlines %u 0 R\r\n", this->outlines);
         }
         fprintf(pf, ">>\r\nendobj\r\n");
      }

      //cryptage éventuel du fichier
      if ( this->encrypt != 0 ) {
         if ( ((this->encrypt->getEncryptStandardP() != ENCRYPT_FULL_RIGHT) || (this->encrypt->getLgMdpU() != 0) ) && !this->forceNoEncrypt ) {
         FILE *pf2;
            random = time(NULL);
            snprintf(lBuf, LG_LBUF, TEMPORARY_CYPHER_NAME);
            pf2 = fopen(lBuf, "wb");
            if ( pf2 ) {
                //en-tête du fichier PDF
                i = snprintf(lBuf, LG_LBUF, "%%PDF-%s\r\n%éàèù\r\n", this->xrefTable->getPdfVersion());
                fwrite(lBuf,1, i, pf2);
                this->encrypt->setPf(this->pf);
                this->encrypt->encryptPdf(pf2);
                np = ftell(pf2);
                np = xrefTable->addObj(np, 0, 'n') - 1;
                fprintf(pf2, "%u 0 obj\r\n<<\r\n/Filter %s\r\n/V %i\r\n/R %i\r\n/Length %i\r\n", np, this->encrypt->getEncryptFilter(), this->encrypt->getEncryptV(), this->encrypt->getEncryptStandardR(), this->encrypt->getEncryptLength());
                i = this->encrypt->getEncryptStandardO()->snprint(lBuf, LG_LBUF, true);
                fprintf(pf2, "/O (");
                fwrite(lBuf, 1, i, pf2);
                fprintf(pf2, ")\r\n");
                i = this->encrypt->getEncryptStandardU()->snprint(lBuf, LG_LBUF, true);
                fprintf(pf2, "/U (");
                fwrite(lBuf, 1, i, pf2);
                fprintf(pf2, ")\r\n");
                fprintf(pf2, "/P %i\r\n", this->encrypt->getEncryptStandardP());
                fprintf(pf2, ">>\r\nendobj\r\n");
                xrefTable->setEncrypt(np);
                fclose(this->pf);
                this->pf = pf2;
            }
         }
      }

      //écriture du trailer
      this->xrefTable->flushTable(pf);
   }
   if ( pf ) {
      fclose(pf);
      pf = NULL;
   }

   if ( random != 0 ) {
       remove(this->nom);
       snprintf(lBuf, LG_LBUF, TEMPORARY_CYPHER_NAME);
       rename(lBuf, this->nom);
       remove(lBuf);
   }

   closed = true;

   return true;
}

#undef LG_LBUF

//*************************************************

int C_pdfFile::merge(char *nomOrg) {
C_pdfFile *org;
int i;

   if ( this->closed ) {
      mergeError = PDF_MERGE_CANT_MERGE_A_CLOSED_FILE;
      return -1;
   }

   if ( this->mode != write ) {
      mergeError = PDF_MERGE_CANT_MERGE_READ_ONLY_FILE;
      return -1;
   }

   org = new C_pdfFile(nomOrg, read);
   if ( !org || !org->getStatus() ) {
      mergeError = PDF_MERGE_CANT_OPEN_ORG_FILE;
      if ( org ) {
         delete org;
      }
      return -1;
   }

   i = this->merge(org);

   delete org;

   return i;
}

//*************************************************

int C_pdfFile::merge(C_pdfFile *org) {
//C_pdfFile *org;
int np, cpt, i;

   if ( this->closed ) {
      mergeError = PDF_MERGE_CANT_MERGE_A_CLOSED_FILE;
      return -1;
   }

   if ( this->mode != write ) {
      mergeError = PDF_MERGE_CANT_MERGE_READ_ONLY_FILE;
      return -1;
   }

   if ( !org || !org->getStatus() ) {
      mergeError = PDF_MERGE_CANT_OPEN_ORG_FILE;
      return -1;
   }

   if ( (org->getXrefTable())->getEncryptObj() != 0 ) {
      if ( !this->encrypt ) {
         this->encrypt = new pdfEncrypt(this->xrefTable);
      }
      if ( org->encrypt->isDecryptPossible() ) {
         this->encrypt->setEncryptStandardP(mostRestrictive(this->encrypt->getEncryptStandardP(), org->encrypt->getEncryptStandardP()));
      }
      else {
         mergeError = PDF_MERGE_CANT_HANDLE_ENCRYPT_VERSION;
         return -1;
      }
   }

   if ( org->getRootAcroForm() != 0 ) {
       if ( org->getNbAcroForm() != 0 ) {
           if ( this->rootAcroForm == 0 ) {
               this->rootAcroForm = this->xrefTable->addObj(0, 0, 'n') - 1; //attention, il faut mettre à jour l'offset à la validation du fichier
           }
       }
   }

   np = org->getFirstPage();
   cpt = 0;
   while ( np ) {
      ++cpt;
      insertPage(org, cpt, rootPages);
      np = org->getNextPage();
   }

   // tranfert du formulaire vers this
   if ( org->getRootAcroForm() != 0 ) {
       if ( org->getNbAcroForm() != 0 ) {
           i = org->getFirstAcroFormObj();
           do {
              if ( this->pAfList == this->lgAfList) {
              int *pi;
                 this->lgAfList *= 2;
                 pi = new int[this->lgAfList];
                 memcpy(pi, this->afList, sizeof(int) * this->pAfList);
                 delete this->afList;
                 this->afList = pi;
              }
              this->afList[this->pAfList++] = this->insertObj(org, i);
           } while ( (i = org->getNextAcroFormObj()) != 0 );
           this->afList[this->pAfList] = 0;

           org->getXrefTable()->setInserted(i, this->rootAcroForm);
           if ( org->acroDA.getLength() != 0 ) {
              this->acroDA.copy(&(org->acroDA));
           }
           //pour acroDR, on ne retient que le premier.
           if ( (org->acroDR != 0) && (this->acroDR == 0) ) {
              this->acroDR = this->insertObj(org, org->acroDR);
           }
       }
   }

   if ( this->keepOutlines ) {
      this->loadOutlines(org);
   }

   if ( org->rootNamesObj != 0 ) {
      this->mergeNames(org);
   }

   return cpt;
}

int C_pdfFile::fullMerge(char *nomOrg) {
C_pdfFile *org;
int i;
char fBuf[30];

   if ( this->closed ) {
      mergeError = PDF_MERGE_CANT_MERGE_A_CLOSED_FILE;
      return -1;
   }

   if ( this->mode != write ) {
      mergeError = PDF_MERGE_CANT_MERGE_READ_ONLY_FILE;
      return -1;
   }

   org = new C_pdfFile(nomOrg, read);
   if ( !org || !org->getStatus() ) {
      mergeError = PDF_MERGE_CANT_OPEN_ORG_FILE;
      if ( org ) {
         delete org;
      }
      return -1;
   }

   if ( (org->getXrefTable())->getEncryptObj() != 0 ) {
      if ( !this->encrypt ) {
         this->encrypt = new pdfEncrypt(this->xrefTable);
      }
      if ( org->encrypt->isDecryptPossible() ) {
         this->encrypt->setEncryptStandardP(mostRestrictive(this->encrypt->getEncryptStandardP(), org->encrypt->getEncryptStandardP()));
      }
      else {
         mergeError = PDF_MERGE_CANT_HANDLE_ENCRYPT_VERSION;
         delete org;
         return -1;
      }
   }

   this->fullMergeFlag = true;
   rewind(this->pf);
   i = snprintf(fBuf,30, "%%PDF-%s\r\n%éàèù\r\n", org->xrefTable->getPdfVersion());
   this->xrefTable->setPdfVersion(fBuf[5] - '0', fBuf[7] - '0');
   fwrite(fBuf, 1, i, this->pf);
   this->xrefTable->removeLastObj();
   i = org->xrefTable->getRootObj();
   i = this->insertObj(org, i);
   this->xrefTable->setNumRoot(i);
   this->close();

   delete org;

   return 0;
}

//*************************************************

bool C_pdfFile::getStatus() {
   return status;
}

//*************************************************

int C_pdfFile::getFirstPage() {

   if ( !getStatus() || (mode != read) )
      return 0;

   if ( !pagesList ) {
      pPage = 0;
      pagesList = new T_pPTN[nbPages + 1];
      buildPagesTree(rootPages, &this->rootTreeNode, 0, 0);
      pagesList[pPage] = 0;
   }

   pPage = 1;

   return pagesList[0]->nObj;
}

int C_pdfFile::getObjAsPageNumber(int nObj) {
T_pageTreeNode **pi;
int i;

    if ( !pagesList )
        this->getFirstPage();

    pi = this->pagesList;
    i = 0;    
    while ( pi[i] && (pi[i]->nObj != nObj) )
       ++i;

    if ( pi[i] )
       return i + 1;

    return 0;
}

int C_pdfFile::getPageAsObjNumber(int nPage) {
T_pageTreeNode **pi;

    if ( nPage > this->nbPages )
        nPage = this->nbPages;

	if ( !this->pagesList )
        this->getFirstPage();

    return this->pagesList[nPage - 1]->nObj;
}

bool C_pdfFile::isObjAPage(int nObj) {

	if ( this->getObjAsPageNumber(nObj) != 0 )
       return true;

    return false;
}

//*************************************************

int C_pdfFile::getNextPage() {

   if ( !getStatus() || (mode != read) )
      return 0;

   if ( !pagesList )
      return getFirstPage();

   if ( pagesList[pPage] != 0 )
      return pagesList[pPage++]->nObj;

   return 0;
}

int C_pdfFile::getPageN(int n) {
   if ( !getStatus() || (mode != read) )
      return 0;

   if ( !pagesList )
      getFirstPage();

   if ( n <= nbPages )
      return pagesList[n - 1]->nObj;

   return 0;
}

T_pageTreeNode *C_pdfFile::getPagePTN(int nPage) {
   if ( nPage <= this->nbPages )
      return this->pagesList[nPage - 1];
   return 0;
}

/* ********************************************** */
#pragma warning(disable : 4100)

int C_pdfFile::makeAttrib(C_pdfFile *org, char *buf, int lgOrgBuf, char **pcRes, char *add, int lgAdd) {
char *pc;
int lgRes, lgBuf, j, nObj, r;

   *pcRes = new char[lgOrgBuf + LG_ATTRIB];
   lgRes = lgBuf = 0;
   pc = *pcRes;

   while ( buf[lgBuf] <= ' ')
      ++lgBuf;
   while (lgBuf < lgOrgBuf ) {
      j = findWinBuf(" R", buf + lgBuf, lgOrgBuf - lgBuf);
      if ( j == -1 )
         break;
      r = rewindRef(buf + lgBuf + j);
      if ( r != 0 ) {
         j -= r;
         nObj = atoi(buf + lgBuf + j);
         nObj = insertObj(org, nObj);
         while ( j ) {
            pc[lgRes++] = buf[lgBuf++];
            --j;
         }
         sprintf(pc + lgRes, "%u 0 R", nObj);
         while ( pc[lgRes] )
            ++lgRes;
         while ( buf[lgBuf++] != 'R' );
      }
      else {
          lgBuf += 1;
      }
   }
   if ( j == - 1 ) {
      while (lgBuf < lgOrgBuf ) {
         pc[lgRes++] = buf[lgBuf++];
      }
   }

   /*if ( (lgAdd != 0) && (add != 0) ) {
      switch ( (int)add ) {
      case _Resources:

         break;
      default:
         memcpy(pc + lgRes, add + sizeof(int), lgAdd);
         lgRes += lgAdd;
      }
   }*/

   pc[lgRes] = 0;

   return lgRes;
}
#pragma warning(default : 4100)

char *C_pdfFile::getValueFromParent(const char * key, C_pdfFile *org, int nPage) {
T_pageTreeNode * pTptn;
T_attrib *pTa;

   if ( nPage > org->nbPages )
      return 0;

   pTptn = org->pagesList[nPage - 1]->parent;

   while ( pTptn != 0) {
      pTa = pTptn->listAttrib;
      while ( pTa != 0 ) {
         if ( strcmp(key, pTa->nom) == 0 )
            return pTa->valeur;
         pTa = pTa->next;
      }

      pTptn = pTptn->parent;
   }

   return 0;
}

/*************************************************
org: pointeur sur un fichier pdf
nPage: numéro de la page à insérer
desParent: numéro du parent dans le fichier dans lequel on insère la page
*/
int C_pdfFile::insertPage(C_pdfFile *org, int nPage, int destParent) {
FILE *orgF;
C_pdfXrefTable *orgXT;
char *fBuf;
int sizeof_fBuf;
int i, j, lgObj, pObj, numPage, idKey, nObj;
char *pc;
C_pdfObject *pPdfObj;

//gestion de options
T_attrib *listAttrib, *pta, *pta2, **ppta;
int listIdAttrib[50]; //liste des identifiants d'attribut trouvé pour la page
                      //y compris les attributs hérités
int pelia; //pointeur en écriture dans le tableau listIdAttrib
T_pageTreeNode *pptn; //gestion de l'héritage.
   listAttrib = 0;
   listIdAttrib[0] = 0;
   pelia = 0;

   if ( closed )
      return PDF_FILE_CLOSED;

   nObj = org->getPageN(nPage);

   if ( nObj == 0 )
      return PDF_FILE_CANT_INSERT_PAGE_0;

   if ( !org->getStatus() || !status )
      return PDF_FILE_CANT_INSERT_STATUS_FALSE;
   orgF = org->getFile();
   orgXT = org->getXrefTable();
   i = orgXT->getOffSetObj(nObj);

   sizeof_fBuf = LG_FBUF;
   fBuf = new char[sizeof_fBuf];
   
   if ( i == 0 ) {
       i = orgXT->getCompObj(nObj);
       if ( i == 0 ) {
           fprintf(stderr, "offset null 1, object : %i !!\r\n", nObj);
           delete fBuf;
           return PDF_FILE_CANT_INSERT_PAGE_OFFSET_IS_NULL;
       }
       i = orgXT->getOffSetObj(i);
       fseek(orgF, i, SEEK_SET);
       fread(fBuf, 1, sizeof_fBuf, orgF);

       pPdfObj = new C_pdfObject(fBuf, sizeof_fBuf, compressedKeyWords, true);
       fBuf = pPdfObj->getObjectFromCompObj(nObj, fBuf, &sizeof_fBuf, &i);

       delete pPdfObj;
   }
   else {
       fseek(orgF, i, SEEK_SET);
       fread(fBuf, 1, sizeof_fBuf, orgF);
   }

   /* ********************************************************************
                        GESTION DES ATTRIBUTS REQUIRED
   */

   // gestion du type REQUIRED
   i = findWinPdfObj("/Type", fBuf, LG_FBUF);
   if ( i == -1 ) {
       delete fBuf;
       return PDF_FILE_CANT_INSERT_PAGE;
   }
   j = i + 5;
   while (fBuf[j] <= ' ' )
      j++;
   pc = "/Page";
   for (i=0; i < 5; i++) {
      if ( fBuf[j + i] != *(pc++) )
         break;
   }
   if ( (i != 5) || (fBuf[j + i] == 's') ) {
       delete fBuf;
       return PDF_FILE_CANT_INSERT_PAGE;
   }

   listIdAttrib[pelia++] = _Type;

   //on réserve un numéro d'objet pour éviter les dead lock
   if ( (numPage = orgXT->getInserted(nObj)) != 0 ) { // gestion des pages dont on avait besoin du numéro avant l'insertion
      xrefTable->setStatus(numPage, 'n');
   }
   else {
      numPage = xrefTable->addObj(0, 0, 'n') - 1;
      orgXT->setInserted(nObj, numPage);
   }

   /* ********************************************************************
                        GESTION DES ATTRIBUTS
   */
   ppta = &listAttrib;

   pc = fBuf;
   while ( *pc != '<' )
      ++pc;
   pc += 2;
   lgObj = tailleObj(pc) - 2; // -2 pour le >>.
   pObj = 0;
#pragma warning (disable: 4127)
   while ( 1 ) {
      while ( (pc[pObj] != '/') && (pObj < lgObj) )
         pObj++;
      if ( pObj >= lgObj )
         break;
      idKey = isWaKey(pc + pObj, pageKeyWords);
      switch ( idKey ) {
      case _Resources:
      case _Rotate:
      case _Contents:
      case _CropBox:
      case _Annots:
      case _BCLPrivAnnots:
      case _MediaBox:
      case _Metadata:
         if ( (idKey == _Rotate) && (this->pageRotation != 0) )
            goto notAType1;
         pta = new T_attrib;
         pObj += keyLen(idKey, pageKeyWords);
         j = tailleAttrib(pc + pObj);
         //*********************************************************************************
         //gestion de l'insertion de texte
         char *pc2, *pc3;
         pc3 = 0;
		 if ( (this->bdpStr || this->insertPageNumber)
			  && (idKey == _Resources)
			  && !X_X_R.doesStrMatchMask(pc + pObj + 1) ) {
         int k, l;
            //il faut gérer /ProcSet et /Font
            pc2 = pc + pObj;
            k = 0;
            while ( pc2[k] == ' ' )
               ++k;
            if ( pc2[k] != '<' ) {
               i = atoi(pc2 + k);
               i = orgXT->getOffSetObj(i);
               fseek(orgF, i, SEEK_SET);
               pc3 = new char[LG_FBUF];
               fread(pc3, 1, LG_FBUF, orgF);
               pc2 = pc3;
               while ( *pc2 >= ' ' )
                  ++pc2;
               while ( *pc2 < ' ' )
                  ++pc2;
               l = j;
               j = tailleAttrib(pc2);
            }

            //en l'état du codage, il est important de traiter /Font avant /ProcSet
            i = findWinBuf("/Font", pc2, j);
            if ( i != -1 ) {
               i += 5;
               while (pc2[i] == ' ')
                  ++i;
               if ( pc2[i] != '<' ) {
               char lBuf[60];
                  snprintf(lBuf, 60, "%s %s", this->bdpFontStr, this->pageNumberFontStr);
                  this->insertAndAppendObj(org, atoi(pc2 + i), lBuf, strlen(lBuf));
               }
            }
            else {
               //il faut ajouter un attribut /Font au dictionnaire /Resources
               //cela est réalisé par mergeDict
            }
            
			char *pcl = (char *)0;
            i = findWinBuf("/ProcSet", pc2, j);
			if ( i != -1 ) {
               i += 8;
               k = i;
               while ( pc2[k] == ' ') {
                  k++;
               }
               if ( *(pc2 + k) != '[' ) {
                  pcl = new char[2 * j];
                  k = i + tailleAttrib(pc2 + i);
                  memcpy(pcl, pc2, i);
                  i += snprintf(pcl + i, 2 * j, " [/PDF /Text /ImageB /ImageC /ImageI]");
                  memcpy(pcl + i, pc2 + k, j - k);
                  pta->lgValeur = makeAttrib(org, pcl, i + j - k, &(pta->valeur), 0, 0);
                  delete pcl;
               }
               else {
                  pta->lgValeur = makeAttrib(org, pc2, j, &(pta->valeur), 0, 0);
               }
            }
            else {
fprintf(stdout, "Malformed PDF, /ProcSet attribut missing in a /Page object (obj = %i)\r\n", nObj);
            }

            if ( pc3 != 0 ) {
               delete pc3;
               j = l;
            }
         }
         //fin de la gestion de l'insertion de texte
         //*********************************************************************************
         else {
            pta->lgValeur = makeAttrib(org, pc + pObj, j, &(pta->valeur), 0, 0);
         }
         pta->nom = nomAttrib(idKey, pageKeyWords);
         pta->next = 0;
         if ( ppta == &listAttrib ) {
            listAttrib = pta;
            ppta = &(listAttrib->next);
         }
         else {
            *ppta = pta;
            ppta = &(pta->next);
         }
         listIdAttrib[pelia++] = idKey;
         pObj += j;
         break;
notAType1:
      case _notAType: // *****************************************************************
      default:        // on saute l'attribut
         ++pObj;
         while ( (pc[pObj] != ' ') && (pc[pObj] != '/') && (pc[pObj] != '[') && (pc[pObj] != '<') && (pObj < lgObj) )
            ++pObj;
         if ( pObj < lgObj )
            pObj += tailleAttrib(pc + pObj);
         break;
      }
   }
#pragma warning (default: 4127)

   //recherche des attributs hérités.
   if ( (org->getPagePTN(nPage))->nbInheritAttrib != 0 ) {
      pptn = (org->getPagePTN(nPage))->parent;
      while ( pptn ) {
         pta2 = pptn->listAttrib;
         while ( pta2 ) {
            idKey = isWaKey((char *)pta2->nom, pageKeyWords);
            i = 0;
            while ( idKey != listIdAttrib[i]) {
               ++i;
               if ( i == pelia )
                  break;
            }
            if ( i == pelia ) {
               pta = new T_attrib;
               pta->lgValeur = makeAttrib(org, pta2->valeur, pta2->lgValeur, &(pta->valeur), 0, 0);
               pta->nom = nomAttrib(idKey, pageKeyWords);
               pta->next = 0;
               if ( ppta == &listAttrib ) {
                  listAttrib = pta;
                  ppta = &(listAttrib->next);
               }
               else {
                  *ppta = pta;
                  ppta = &(pta->next);
               }
               listIdAttrib[pelia++] = idKey;
            }
            pta2 = pta2->next;
         }
         pptn = pptn->parent;
      }
   }


   //**************************************************************************************************
   // gestion de l'ajout d'information
   if ( this->bdpStr || this->insertPageNumber ) {
      pta = listAttrib;
      while ( pta ) {
         if ( strcmp(pta->nom, "/Contents") == 0 ) {
            break;
         }
         pta = pta->next;
      }
      if ( pta ) {
         i = 0;
         j = 0;
         while ( pta->valeur[j] == ' ' )
            ++j;
         if ( pta->valeur[j] == '[' ) {
            while ( pta->valeur[i] != ']' ) {
               fBuf[i] = pta->valeur[i];
               ++i;
            }
         }
         else {
            fBuf[i++] = '[';
            while ( pta->valeur[j] != 'R' ) {
               fBuf[i++] = pta->valeur[j++];
            }
            fBuf[i++] = 'R';
         }
         j = i;
         if ( this->insertPageNumber ) {
         int nbPagesObj;
         int sizeof_lBuf;
         int ii;
         char * lBuf;
         C_pdfObject *pdfObj;

            sizeof_lBuf = LG_FBUF;
            lBuf = new char[sizeof_lBuf];
            pdfObj = new C_pdfObject();
            ii = snprintf(lBuf, sizeof_lBuf, "BT\r\n  /F255 %i Tf\r\n  %.1f %.1f %.1f rg\r\n  %.1f %.1f %.1f %.1f %i %i Tm\r\n  0 Tr\r\n  (%i) Tj\r\nET", this->pageNumberSize, this->pageNumberRgb, this->pageNumberrGb, this->pageNumberrgB, cos(this->pageNumberOrientation), sin(this->pageNumberOrientation), -sin(this->pageNumberOrientation), cos(this->pageNumberOrientation), this->pageNumberX, this->pageNumberY, this->nbPages + this->pageNumberFirstNum);
            pdfObj->appendStream(lBuf, ii);
            nbPagesObj = this->insertObj(pdfObj);
            delete pdfObj;
            delete lBuf;

            i += sprintf(fBuf + i, " %i 0 R", nbPagesObj);
         }
         if ( this->bdpStr != 0 ) {
            i += sprintf(fBuf + i, " %i 0 R", this->bdpObj);
         }
         if ( i > j ) {
            i += sprintf(fBuf + i, "]");
            delete pta->valeur;
            pta->valeur = new char[i + 1];
            sprintf(pta->valeur, "%s", fBuf);
            pta->lgValeur = i;
         }
      }

      pta = listAttrib;
      while ( pta ) {
         if ( strcmp(pta->nom, "/Resources") == 0 ) {
            break;
         }
         pta = pta->next;
      }
      if ( pta ) {
         if ( (this->bdpStr != 0) || this->insertPageNumber ) {
            sprintf(fBuf, "<</ProcSet [/Text] /Font <</F255 %i 0 R /F256 %i 0 R>>>>", this->pageNumberFontObj, this->bdpFontObj);
            i = 0;
            mergeDict(resourcesKeyWords, this, pta->valeur, pta->lgValeur, fBuf, strlen(fBuf), &pc, &i);
            delete pta->valeur;
            pta->valeur = pc;
            pta->lgValeur = i;
         }
      }
   }
   //**********************************************************************************************

   xrefTable->setOffset(numPage, ftell(pf));
   xrefTable->setStatus(numPage, 'n');
   fprintf(pf, "%u 0 obj\r\n<<\r\n/Type /Page\r\n/Parent %u 0 R\r\n", numPage, destParent);

   //ecriture des options
   pta = listAttrib;
   while ( pta ) {
      fprintf(pf, "%s ", pta->nom);
      fwrite(pta->valeur, 1, pta->lgValeur, pf);
      fprintf(pf, "\r\n");
      pta = pta->next;
   }

   fprintf(pf, ">>\r\nendobj\r\n");

   addChild(numPage);
   ++nbPages;

   //destruction de la mémoire allouée
   if ( listAttrib ) {
      pta = listAttrib;
      while ( pta ) {
         if ( pta->valeur )
            delete pta->valeur;
         pta = pta->next;
         delete listAttrib;
         listAttrib = pta;
      }
   }

   delete fBuf;

   return 0;
}

//**************************************************************
// insère dans le fichier pdf l'objet nObj du fichier org
// renvoi le numéro de l'objet inséré

int C_pdfFile::insertObj(C_pdfFile *org, int nObj) {
FILE *orgF;
C_pdfXrefTable *orgXT;
char *fBuf, *pc;
int i, j, k, l, res, pt, iDebut, iFin, sizeof_fBuf, posInFile, lgObj, r;
int *iBuf, sizeof_iBuf;

int numObjInserted;

   if ( !org->getStatus() || !status )
      return 0;
      
   orgF = org->getFile();
   orgXT = org->getXrefTable();

   if ( (i = orgXT->getInserted(nObj)) != 0 )
      return i; // l'objet a déjà été inséré.   
   
   if ( !orgXT->getOffSetObj(nObj) ) {
	   typeXref *ptxr = orgXT->getObjTXR(nObj);
	   if ( ptxr->compObj != 0 && ptxr->indexCompObj != 0) {
		   sizeof_fBuf = LG_FBUF * 2;
		   fBuf = new char[sizeof_fBuf];
		   if ( fBuf == 0 ) {
			  return 0;
		   }

		   i = orgXT->getOffSetObj(ptxr->compObj);
           fseek(orgF, i, SEEK_SET);
           fread(fBuf, 1, sizeof_fBuf, orgF);
		   C_pdfObject* pPdfObj = new C_pdfObject(fBuf, sizeof_fBuf, compressedKeyWords, true);
		   pPdfObj->getObjectFromCompObj(nObj, fBuf, &sizeof_fBuf, &i);
		   
		   delete pPdfObj;		   

		   iDebut = 0;
           iFin = i;

	   } else {	   
	       fprintf(stderr, "offset null 3, object : %i !!\r\n", nObj);
           return 0;      
	   }
   } else {

	   if ( !this->fullMergeFlag && org->isObjAPage(nObj) ) {
		   //on réserve éventuellement la page.
		   if ( (k = orgXT->getInserted(nObj)) == 0 ) {
			   k = xrefTable->addObj(0, 0, 'f') - 1; // f car on n'est pas sur que la page sera insérée dans le cas d'un script
			   orgXT->setInserted(nObj, k);
		   }
		   return k;
	   }

	   // thank you to Sam
	   // mais on a un petit probleme quand même la....
	   if ( orgXT->getStatus(nObj) == 'f' )  {
		  //on réserve un numéro d'objet pour éviter les dead lock
		  numObjInserted = xrefTable->addObj(0, 0, 'n') - 1;
		  orgXT->setInserted(nObj, numObjInserted);

		  this->xrefTable->setFreeObject(numObjInserted);
		  return numObjInserted;
	   }

	   i = orgXT->getOffSetObj(nObj);
	   if ( i == 0 ) {
		  fprintf(stderr, "offset null 3, object : %i !!\r\n", nObj);
		  return 0;
	   }

	   fseek(orgF, i, SEEK_SET);
	   // chargement de l'objet
	   sizeof_fBuf = LG_FBUF * 2;
	   fBuf = new char[sizeof_fBuf];
	   if ( fBuf == 0 ) {
		  return 0;
	   }
	   posInFile = ftell(orgF);
	   fread(fBuf, 1, sizeof_fBuf, orgF);
	   j = findWinBuf("endobj", fBuf, sizeof_fBuf);
	   while ( j == - 1 ) {
		  pc = new char[sizeof_fBuf * 2];
		  memcpy(pc, fBuf, sizeof_fBuf);
		  fread(&pc[sizeof_fBuf], 1, sizeof_fBuf, orgF);
		  sizeof_fBuf *= 2;
		  delete fBuf;
		  fBuf = pc;
		  j = findWinBuf("endobj", fBuf, sizeof_fBuf);
	   }
	   //lgObj = j + 6; // 6 = endobj
	   if ( org->encrypt->isDecryptPossible() ) {
		  i = -1;
		  while ( i == -1 ) {
			 i = org->encrypt->encryptObj(fBuf, lgObj, sizeof_fBuf, false);
			 if ( i == -1 ) {
				delete fBuf;
				sizeof_fBuf += 2 * LG_FBUF;
				fBuf = new char[sizeof_fBuf];
				fseek(orgF, posInFile, SEEK_SET);
				fread(fBuf, 1, sizeof_fBuf, orgF);
			 }
		  }
		  //lgObj = i;
	   }
	   
	   // fin du chargement
	   iDebut = findWinBuf("obj", fBuf, sizeof_fBuf) + 3;
       iFin = findWinBuf("endobj", fBuf, sizeof_fBuf);
   }   

   //détermination du type de l'objet
   i = findWinBuf("/Type", fBuf + iDebut, iFin - iDebut);
   C_pdfObject *ppo = 0;
   if ( i != -1 ) {
	   i += 5;
	   while ( *(fBuf + iDebut + i ) != '/' )
		   ++i;
	   switch ( isWaKey(fBuf + iDebut + i, typeKeyWordsValues) ) {
		   /*case _Font:
		       ppo = new C_pdfObject(fBuf, lgObj, fontKeyWords, false);
			   if ( this->fontList == 0 ) {
				   this->fontList = ppo;
			   }
			   else {
				   i = this->fontList->compareThisToSingleObject(ppo, true);
				   if ( i != -1 ) {
					   delete ppo;
					   delete fBuf;
					   return i;
				   }
				   else {
					   this->fontList->chainObjectToThis(ppo);
				   }
			   }
			   break;*/
		   case _notAType:
			   break;
	   }
       
   }
   //fin de la détermination du type de l'objet.

   //on réserve un numéro d'objet pour éviter les dead lock
   numObjInserted = xrefTable->addObj(0, 0, 'n') - 1;
   orgXT->setInserted(nObj, numObjInserted);
   if ( ppo != 0 )
	   ppo->setNumObj(numObjInserted);
   
   //recherche des objets liés
   pt = 0;
   k = findWinBuf("stream", fBuf, iFin);
   if ( k == -1) {
      k = iFin;
   }

   i = findWinBuf(" R", fBuf, k);
   l = 0;
   sizeof_iBuf = LG_TBUF;
   iBuf = new int[sizeof_iBuf];
   while ( i != -1 ) {
      l += i + 2;
      i = l - 2;
      r = rewindRef(fBuf + i);
      if ( r != 0 ) {
         i -= r;
         i = atoi(fBuf + i);
         if ( pt >= sizeof_iBuf - 2 ) {
         int *pi;
            sizeof_iBuf += LG_TBUF;
            pi = new int[sizeof_iBuf];
            memcpy(pi, iBuf, (sizeof_iBuf - LG_TBUF) * sizeof(int) );
            delete iBuf;
            iBuf = pi;
         }		 
         iBuf[pt++] = insertObj(org, i);		 
      }
      else {
          //debug
                       if ( iBuf[pt - 1] == 0 ) {
                       char c;
                           c = *(fBuf + l + 1);
                           *(fBuf + l + 1) = 0;
                           fprintf(stderr, "mauvaise détection %s\r\n", fBuf + l - 5);
                           *(fBuf + l + 1) = c;
                       }
      }

      i = findWinBuf(" R", fBuf + l, k - l);
   }
   // fin de la recherche des objets liés
   // insertion de l'objet dans le fichier
   this->xrefTable->setOffset(numObjInserted, ftell(this->pf));
   fprintf(pf, "%u 0 obj", numObjInserted);
   res = numObjInserted; // numéro de l'objet inséré
   pc = fBuf + iDebut;
   for (i = 0; i < pt; i++) {
      // substitution de nouveaux numéros d'objets aux objets liés
      j = findWinBuf(" R", pc, sizeof_fBuf);
      r = rewindRef(pc + j);
      if ( r != 0 ) {
         k = j - r;
         fwrite(pc ,1, k, pf);
         fprintf(pf, "%u 0 R", iBuf[i]);
      }
      else {
         fwrite(pc ,1, j + 2, pf);
         --i;
      }
      pc += j + 2;
   }

   j = (iFin - iDebut) - (pc - fBuf - iDebut);
   fwrite(pc , 1, j, pf);
   fprintf(pf,"endobj\r\n");

   delete fBuf;
   delete iBuf;

   if ( res <= 0 ) {
      return 0;
   }

   return res;
}


//**************************************************************
// insère dans le fichier pdf l'objet nObj du fichier org
// renvoi le numéro de l'objet inséré

int C_pdfFile::insertAndAppendObj(C_pdfFile *org, int nObj, const char *append, int lgAppend) {
FILE *orgF;
C_pdfXrefTable *orgXT;
char *fBuf, *pc;
int i, j, k, l, res, pt, iDebut, iFin, sizeof_fBuf, posInFile, lgObj, r;
int *iBuf, sizeof_iBuf;

int numObjInserted;

   if ( !org->getStatus() || !status )
      return 0;
      
   orgF = org->getFile();
   orgXT = org->getXrefTable();
   
   if ( !orgXT->getOffSetObj(nObj) )
      return 0;
   
   if ( (i = orgXT->getInserted(nObj)) != 0 )
      return i; // l'objet a déjà été inséré.

   //on réserve un numéro d'objet pour éviter les dead lock
   if ( (numObjInserted = orgXT->getInserted(nObj)) != 0 ) { // gestion des objets dont on avait besoin du numéro avant l'insertion
      return numObjInserted;
   }
   else {
      numObjInserted = xrefTable->addObj(0, 0, 'n') - 1;
      orgXT->setInserted(nObj, numObjInserted);
   }

   // thank you to Sam
   // mais on a un petit probleme quand même la....
   if ( orgXT->getStatus(nObj) == 'f' )  {
      this->xrefTable->setFreeObject(numObjInserted);
      return numObjInserted;
   }

   i = orgXT->getOffSetObj(nObj);
   if ( i == 0 ) {
      fprintf(stderr, "offset null 3, object : %i !!\r\n", nObj);
      return 0;
   }

   ++lgAppend;
   
   //***********************************************************
   // chargement de l'objet
   fseek(orgF, i, SEEK_SET);
   sizeof_fBuf = LG_FBUF * 2;
   fBuf = new char[sizeof_fBuf + lgAppend];
   if ( fBuf == 0 ) {
      return 0;
   }
   posInFile = ftell(orgF);
   fread(fBuf, 1, sizeof_fBuf, orgF);
   j = findWinBuf("endobj", fBuf, sizeof_fBuf);
   while ( j == - 1 ) {
      pc = new char[sizeof_fBuf + 2 * LG_FBUF + lgAppend];
      memcpy(pc, fBuf, sizeof_fBuf);
      fread(&pc[sizeof_fBuf], 1, 2 * LG_FBUF, orgF);
      sizeof_fBuf += 2 * LG_FBUF;
      delete fBuf;
      fBuf = pc;
      j = findWinBuf("endobj", fBuf, sizeof_fBuf);
   }
   lgObj = j + 6; // 6 = endobj
   
   if ( org->encrypt->isDecryptPossible() ) {
      i = -1;
      while ( i == -1 ) {
         i = org->encrypt->encryptObj(fBuf, lgObj, sizeof_fBuf, false);
         if ( i == -1 ) {
            delete fBuf;
            sizeof_fBuf += 2 * LG_FBUF;
            fBuf = new char[sizeof_fBuf];
            fseek(orgF, posInFile, SEEK_SET);
            fread(fBuf, 1, sizeof_fBuf, orgF);
         }
      }
      lgObj = i;
   }
   iDebut = findWinBuf("obj", fBuf, LG_FBUF) + 3;
   iFin = lgObj - 6;
   // fin du chargement
   //***********************************************************
   
   //***********************************************************
   //recherche des objets liés
   pt = 0;
   k = findWinBuf("stream", fBuf, iFin);
   if ( k == -1) {
      k = iFin;
   }

   i = findWinBuf(" R", fBuf, k);
   l = 0;
   sizeof_iBuf = LG_TBUF;
   iBuf = new int[sizeof_iBuf];
   while ( i != -1 ) {
      l += i + 2;
      i = l - 2;
      r = rewindRef(fBuf + i);
      if ( r != 0 ) {
         i -= r;
         i = atoi(fBuf + i);
         if ( pt >= sizeof_iBuf - 2 ) {
         int *pi;
            sizeof_iBuf += LG_TBUF;
            pi = new int[sizeof_iBuf];
            memcpy(pi, iBuf, (sizeof_iBuf - LG_TBUF) * sizeof(int) );
            delete iBuf;
            iBuf = pi;
         }
         iBuf[pt++] = insertObj(org, i);
      }
      else {
          //debug
                       if ( iBuf[pt - 1] == 0 ) {
                       char c;
                           c = *(fBuf + l + 1);
                           *(fBuf + l + 1) = 0;
                           fprintf(stderr, "mauvaise détection %s\r\n", fBuf + l - 5);
                           *(fBuf + l + 1) = c;
                       }
      }

      i = findWinBuf(" R", fBuf + l, k - l);
   }
   // fin de la recherche des objets liés
   //***********************************************************

   //***********************************************************
   //modification de l'objet
   int ii, jj;
   int t;

   ii = 0;
   while ( fBuf[ii] >= ' ' )
      ++ii;
   while ( fBuf[ii] < ' ' )
      ++ii;
   t = tailleAttrib(fBuf + ii);
   switch ( fBuf[ii]) {
   case '<':
      for (jj = lgObj; jj >= (ii + t - 2) ; jj--) {
         fBuf[jj + lgAppend] = fBuf[jj];
      }
      fBuf[ii + t - 2] = ' ';
      memcpy(fBuf + ii + t - 1, append, lgAppend - 1);
      break;
   case '[':
      break;
   }

   lgObj += lgAppend;
   iFin = lgObj - 6;
   //fin modification de l'objet
   //***********************************************************

   //***********************************************************
   // insertion de l'objet dans le fichier
   this->xrefTable->setOffset(numObjInserted, ftell(this->pf));
   fprintf(pf, "%u 0 obj", numObjInserted);
   res = numObjInserted; // numéro de l'objet inséré
   pc = fBuf + iDebut;
   for (i = 0; i < pt; i++) {
      // substitution de nouveaux numéros d'objets aux objets liés
      j = findWinBuf(" R", pc, sizeof_fBuf);
      r = rewindRef(pc + j);
      if ( r != 0 ) {
         k = j - r;
         fwrite(pc ,1, k, pf);
         fprintf(pf, "%u 0 R", iBuf[i]);
      }
      else {
         fwrite(pc ,1, j + 2, pf);
         --i;
      }
      pc += j + 2;
   }

   j = (iFin - iDebut) - (pc - fBuf - iDebut);
   fwrite(pc , 1, j, pf);
   fprintf(pf,"endobj\r\n");
   //fprintf(pf,"\r\n");

   delete fBuf;
   delete iBuf;

   if ( res <= 0 ) {
      return 0;
   }

   return res;
}

//*******************************************************

int C_pdfFile::addChild(int i) {
int *pc;

   if ( pChilds == sizeof_childs ) {
      //on agrandit la table des enfants
      pc = childs;
      sizeof_childs += NB_CHILDS;
      childs = new int[sizeof_childs];
      memcpy(childs, pc, pChilds * sizeof(int));
      delete pc;
   }

   childs[pChilds++] = i;

   return pChilds;
}


//*******************************************************

#define PTN (*currentPTN)
int C_pdfFile::buildPagesTree(int nObj, T_pageTreeNode **currentPTN, T_pageTreeNode *parent, int cptInheritAttr) {
int pt, i, type, lgObj;
char *fBuf;
int sizeof_fBuf;
int *tBuf;
char *pc;
C_pdfObject *pPdfObj;


   PTN = 0;
   pPdfObj = 0;
   if ( !getStatus() )
      return 0;
   sizeof_fBuf = LG_FBUF;
   fBuf = new char[sizeof_fBuf];

   fBuf = this->getObj(nObj, fBuf, &sizeof_fBuf, &lgObj, &pPdfObj, true);

   if ( lgObj == 0 ) {        
        fprintf(stderr, "offset null 4 !!\r\n");
        delete fBuf;
        if ( pPdfObj )
            delete pPdfObj;
        return 0;
    }

   i = findWinBuf("/Type", fBuf, lgObj);
   if ( i == -1 ) {
       if ( pPdfObj )
           delete pPdfObj;
      delete fBuf;
      return 0;
   }
   while ( fBuf[++i] != '/');
   type = 0;
   pc = "/Pages";
   while ( fBuf[i++] == pc[type++]);
   if ( type < 6 ) {
       if ( pPdfObj )
           delete pPdfObj;
       delete fBuf;
       return 0;
   }
   
   PTN = new T_pageTreeNode;
   PTN->nObj = nObj;
   PTN->parent = (struct S_pageTreeNode *)parent;
   PTN->listAttrib = 0;
   PTN->nbInheritAttrib = 0;
   switch ( type ) {
   case 6:
      PTN->type = PTN_TYPE_PAGE;
      PTN->nbKids = 0;
      PTN->kids = 0;
      PTN->nbInheritAttrib = cptInheritAttr;
      pagesList[pPage++] = PTN;
      if ( pPdfObj )
           delete pPdfObj;
      delete fBuf;
      return 0;

   case 7:
      PTN->type = PTN_TYPE_PAGES;

      //construction de la liste des attributs.

      pc   = fBuf;
      int pObj;
      pObj = 0;
      int idKey, j;
      T_attrib **ppta;
      ppta = &(PTN->listAttrib);
      T_attrib *pta;

#pragma warning (disable : 4127)
      while ( 1 ) {
#pragma warning (default : 4127)
         while ( (pc[pObj] != '/') && (pObj < lgObj) )
            pObj++;
         if ( pObj >= lgObj )
            break;
         idKey = isWaKey(pc + pObj, pageKeyWords);
         switch ( idKey ) {
         case _Rotate: 
         case _CropBox:
         case _MediaBox:
         case _Resources:
            if ( (idKey == _Rotate) && (this->pageRotation != 0) )
                goto notAType2;
            ++cptInheritAttr;
            pObj += keyLen(idKey, pageKeyWords);
            j = tailleAttrib(pc + pObj);
            pta = new T_attrib;
            pta->nom = nomAttrib(idKey, pageKeyWords);
            pta->valeur = new char[j + 1];
            memcpy(pta->valeur, pc + pObj, j);
            pta->lgValeur = j;
            pta->next = 0;
            *ppta = pta;
            ppta = &(pta->next);
            pObj += j;
            break;
notAType2:
         case _notAType:
         default: // on saute l'attribut
            ++pObj;
            while ( (pc[pObj] != ' ') && (pc[pObj] != '/') && (pc[pObj] != '[') && (pc[pObj] != '<') && (pObj < lgObj) )
               ++pObj;
            if ( pObj < lgObj )
               pObj += tailleAttrib(pc + pObj);
            break;
         }
      }

      break;

   default:
       if ( pPdfObj )
           delete pPdfObj;
       delete fBuf;
       return 0;
   }

   i = findWinBuf("/Count", fBuf, lgObj);
   if ( i == -1 ) {
       if ( pPdfObj )
           delete pPdfObj;
       delete fBuf;
       return 0;
   }
   pt = atoi(fBuf + i + 7);
   i = findWinBuf("/Kids", fBuf, lgObj);
   if ( i == -1 ) {
       if ( pPdfObj )
           delete pPdfObj;
       delete fBuf;
       return 0;
   }

   tBuf = new int[pt + 1];
   i += 5;

   while ( fBuf[i] <= ' ' )
      i++;
   switch ( fBuf[i] ) {
   case '[': //c'est un tableau
      pt = 1;
      break;
   case '0':
   case '1':
   case '2':
   case '3':
   case '4':
   case '5':
   case '6':
   case '7':
   case '8':
   case '9': // c'est une référence
      i = atoi(fBuf + i);
      i = xrefTable->getOffSetObj(i);
      if ( i ) {
         fseek(pf, i, SEEK_SET);
         fread(fBuf, 1, LG_FBUF, pf);
         i = 0;
         while ( fBuf[i] >= ' ')
            ++i;
         while ( fBuf[i] <= ' ')
            ++i;
         if ( fBuf[i] != '[' ) { // ce n'est pas un tableau
            //fichier invalide
            pt = 0;
            break;
         }
         pt = 1;
      }
      break;
   default:  // c'est autre chose
      pt = 0;
      break;
   }

   if ( pt ) {
      ++i;
      pt = 0;
#pragma warning(disable : 4127)
      while ( 1 ) {
#pragma warning(default: 4127)
         while ( (fBuf[i] < '0') || (fBuf[i] > '9') && (fBuf[i] != ']') )
            ++i;
         if ( fBuf[i] == ']' ) break;
         tBuf[pt++] = atoi(fBuf + i);
         while ( (fBuf[i] != 'R') && (fBuf[i] != ']')) i++;
      }
      tBuf[pt] = 0;
      PTN->kids = new T_pPTN[pt];
      memset(PTN->kids, 0, pt * sizeof(T_pPTN));
      PTN->nbKids = pt;
      pt = 0;
      while ( tBuf[pt] != 0 ) {
         //
         buildPagesTree(tBuf[pt], &(PTN->kids[pt]), PTN, cptInheritAttr);
         //
         ++pt;
      }
   }

   if ( pPdfObj )
       delete pPdfObj;

   if ( tBuf ) {
      delete tBuf;
   }

   delete fBuf;
   return pt;
}

//**************************************************************************************************
//**************************************************************************************************
//**************************************************************************************************

/*idee de base
on charge la liste des fields du dictionnaire acroform
on vérifie à la fin du merge du ficheir que tous les champ ont bien été mergé
on tranfert la liste de champs de formulaire vers la liste du fichier destination

  et en théorie le tour est joué.

  */

//pAF point sur le dictionnaire correspondant à l'entrée, c'est à dire sur le <<
int C_pdfFile::buildAcroFormTree(char *pAF) {
char *pc;
int lgDic;
int pos;

    if ( (*pAF != '<') || (*(pAF + 1) != '<' ) )
        return -1;

    pc = pAF;
    pos = 0;
    do  {
       if ( (*pc == '<') && (*(pc + 1) == '<') ) {
           ++pos;
           ++pc;
       }
       if ( (*pc == '>') && (*(pc + 1) == '>') ) {
           --pos;
           ++pc;
       }
        ++pc;
    } while ( pos != 0 );
    lgDic = pc - pAF - 2;

    pos = findWinBuf("/Fields", pAF, lgDic);
    if ( pos != -1 ) {
        pc = pAF + pos + 7;
#pragma warning(disable : 4127)
        while ( 1 ) {
            while ( ((*pc < '0') || (*pc > '9')) && (*pc != ']') ) {
                ++pc;
            }
            if ( *pc == ']' )
                break;

            if ( this->pAfList == this->lgAfList) {
            int *pi;
               this->lgAfList *= 2;
               pi = new int[this->lgAfList];
               memcpy(pi, this->afList, sizeof(int) * this->pAfList);
               delete this->afList;
               this->afList = pi;
            }
            this->afList[pAfList++] = atoi(pc);
            while (*pc != 'R' )
                ++pc;
        }
        this->afList[pAfList] = 0;
#pragma warning(default : 4127)
    }

    pos = findWinBuf("/DA", pAF, lgDic);
    if ( pos != -1 ) {
        pc = pAF + pos + 3;
        while ( *pc == ' ')
           ++pc;
        this->acroDA.copy(pc);
    }
    pos = findWinBuf("/DR", pAF, lgDic);
    if ( pos != -1 ) {
       pc = pAF + pos + 3;
       while ( *pc == ' ')
          ++pc;
       this->acroDR = atoi(pc);
    }

    return 0;
}

int C_pdfFile::getRootAcroForm() {

    return this->rootAcroForm;
}

int C_pdfFile::getNbAcroForm() {
int i;

    if ( afList == 0 )
       return 0;

    i = 0;
    while ( afList[i] != 0 ) {
        ++i;
    }

    return i;
}

int C_pdfFile::getFirstAcroFormObj() {
    this->pAfList = 0;

    if ( this->afList[0] != 0 ) {
        return this->afList[this->pAfList++];
    }

    return 0;
}
int C_pdfFile::getNextAcroFormObj() {
    if ( this->afList[0] != 0 ) {
        return this->afList[this->pAfList++];
    }

    return 0;
}

bool C_pdfFile::setForceNoEncrypt(bool b) {
    this->forceNoEncrypt = b;

    return b;
}

C_pdfFile *C_pdfFile::setOptions(char *mdpU, char *mdpO, bool masterModeNoEncrypt, char *restrict, int lgEncryptKey, int pMetadataStr, char **metadataStr) {

   if ( mdpU || mdpO || restrict || lgEncryptKey ) {
      if ( this->encrypt == 0 ) {
         this->encrypt = new pdfEncrypt(this->xrefTable);
      }


      if ( mdpU != 0 ) {
         this->encrypt->setMdpU(mdpU);
      }
      if ( mdpO != 0 ) {
         this->encrypt->setMdpO(mdpO);
      }

      if ( restrict != 0 ) {
      int li;
         li = ENCRYPT_FULL_RIGHT;
         while ( *restrict ) {
            switch (*restrict) {
            case 'a':
               li &= ~ENCRYPT_ALLOW_FILL;
               break;
            case 'm':
               li &= ~ENCRYPT_ALLOW_MODIFY;
               break;
            case 'p':
               li &= ~ENCRYPT_ALLOW_PRINT;
               break;
            case 's':
               li &= ~ENCRYPT_ALLOW_EXTRACT;
               break;
            }
            ++restrict;
         }
         this->encrypt->setEncryptRestriction(li);
      }

      this->encrypt->setEncryptKeyLength(lgEncryptKey);
   }

   if ( masterModeNoEncrypt ) {
      this->setForceNoEncrypt(true);
   }


   if ( pMetadataStr != 0 ) {
   int i;
      i = 0;
      while ( metadataStr[i] != 0 ) {
         this->setMetadataStr(metadataStr[i], metadataStr[i + 1], strlen(metadataStr[i + 1]));
         i += 2;
      }
   }

   return this;
}

int C_pdfFile::setEncryptStandardP(int P) {
   if ( this->encrypt == 0 ) {
         this->encrypt = new pdfEncrypt(this->xrefTable);
   }

   this->encrypt->setEncryptStandardP(P);

   return P;
}

int C_pdfFile::getEncryptStandardP() {
   if ( this->encrypt == 0 ) {
         this->encrypt = new pdfEncrypt(this->xrefTable);
   }

   return this->encrypt->getEncryptStandardP();
}

//*********************************************************************************************************************************
//*********************************************************************************************************************************

const char *C_pdfFile::setBdpStr(const char *str) {
char *fBuf, *pc;
int sizeof_fBuf;
int i;
C_pdfObject *pdfObj;

   if ( !this->getStatus() )
      return 0;

   if ( str != 0 ) {
      if ( str[0] == 0 ) {
         str = 0;
      }
   }
   
   this->bdpStr = str;

   if ( this->bdpStr != 0 ) {
      if ( this->bdpFontObj == 0 ) {
         pdfObj = new C_pdfObject();
         pc = "/Font";
         pdfObj->insertAttrib("/Type", pc, strlen(pc), 0);
         pc = (char *)fontNames[this->bdpFontName];
         pdfObj->insertAttrib("/BaseFont", pc, strlen(pc), 0);
         pc = "/Type1";
         pdfObj->insertAttrib("/Subtype", pc, strlen(pc), 0);
         pc = "/WinAnsiEncoding";
         pdfObj->insertAttrib("/Encoding", pc, strlen(pc), 0);
         this->bdpFontObj = this->insertObj(pdfObj);
         delete pdfObj;

         sprintf(this->bdpFontStr, "/F256 %i 0 R", this->bdpFontObj);
      }
      sizeof_fBuf = LG_FBUF;
      fBuf = new char[sizeof_fBuf];

      pdfObj = new C_pdfObject();
      i = snprintf(fBuf, sizeof_fBuf, "BT\r\n  /F256 %i Tf\r\n  %.1f %.1f %.1f rg\r\n  %.1f %.1f %.1f %.1f %i %i Tm\r\n  0 Tr\r\n  (%s) Tj\r\nET", this->bdpSize, this->bdpRgb, this->bdprGb, this->bdprgB, cos(this->bdpOrientation), sin(this->bdpOrientation), -sin(this->bdpOrientation), cos(this->bdpOrientation), this->bdpX, this->bdpY, this->bdpStr);
      pdfObj->appendStream(fBuf, i);
      this->bdpObj = this->insertObj(pdfObj);
      delete pdfObj;

      delete fBuf;
   }

   return str;
}

int C_pdfFile::insertObj(C_pdfObject *pObj) {
int numObjInserted;

   numObjInserted = xrefTable->addObj(ftell(this->pf), 0, 'n') - 1;
   pObj->flush(this->pf, numObjInserted);

   return numObjInserted;
}

int C_pdfFile::insertPage(C_pdfObject *pObj) {
int numPage;

   numPage = this->insertObj(pObj);
   
   this->addChild(numPage);
   ++this->nbPages;

   return numPage;
}

//*********************************************************************************************************************************
//permet de d'assembler deux dictionnaires

#define LG_KEY_TABLE 20

typedef struct {
   const char *nom;
   int lgNom;
   const char *attrib;
   int lgAttrib;
} T_keyLoc;

int parseDict(const char *dict, int lgDict, T_keyLoc *res, int lgRes) {
int pDict, pRes, i;

   pDict = 0;
   pRes = 0;
   while ( 1 ) {
      while ( (dict[pDict] != '/') && (pDict < lgDict) )
         pDict++;
      if ( (pDict >= lgDict) || (pRes >= lgRes) )
         break;
      
      res[pRes].nom = dict + pDict;
      i = pDict;
      ++pDict;
      while ( (dict[pDict] != ' ') && (dict[pDict] != '/') && (dict[pDict] != '[') && (dict[pDict] != '<') && (pDict < lgDict) )
         ++pDict;
      res[pRes].lgNom = pDict - i;
      i = tailleAttrib(dict + pDict);
      res[pRes].attrib = dict + pDict;
      res[pRes].lgAttrib = i;
      pDict += i;
      ++pRes;
   }

   return pRes;
}


//C_pdfFile::mergeDict
// org1 un pointeur vers le fichier pdf qui contient dict1, utile si dict 1 est une indirection vers un objet
#define LG_LBUF 256
char *C_pdfFile::mergeDict(T_keyWord *ptkw, C_pdfFile *org1, const char *dict1, int lgDict1, const char *dict2, int lgDict2, char **pcRes, int *lgRes) {
//int lgAttrib1, lgAttrib2;
char *pc, *pc1, *pc2;
int lgPc, sizeof_res, nbAttrib1, nbAttrib2, i1, i2, i, j;
//gestion des dictionnaires indirects
char *newDict1 = 0;
int lgNewDict1;

char lBuf[LG_LBUF];
int k, l;

T_keyLoc keyTable1[LG_KEY_TABLE];
T_keyLoc keyTable2[LG_KEY_TABLE];

   nbAttrib1 = parseDict(dict1, lgDict1, keyTable1, LG_KEY_TABLE);
   if ( nbAttrib1 == 0 ) {
	   if ( X_X_R.doesStrMatchMask(dict1) ) {
		   if ( org1 ) {
			   lgNewDict1 = LG_LBUF;
			   newDict1 = new char[lgNewDict1];
			   i = atoi(dict1);
			   newDict1 = org1->getObj(i, newDict1, &lgNewDict1, &lgDict1, NULL,false);
			   if (org1 == this )
				   fseek(this->pf, 0, SEEK_END);
			   i = findWinBuf("<<", newDict1, lgNewDict1);
			   memcpy(newDict1, newDict1 + i, lgNewDict1 - i);
			   dict1 = newDict1;
			   lgDict1 = lgNewDict1 - i;
			   nbAttrib1 = parseDict(dict1, lgDict1, keyTable1, LG_KEY_TABLE);
		   }
	   }
   }
   nbAttrib2 = parseDict(dict2, lgDict2, keyTable2, LG_KEY_TABLE);

   if ( *lgRes == 0 ) {
      sizeof_res = LG_FBUF;
      *pcRes = new char[sizeof_res];
   }
   else {
      sizeof_res = *lgRes;
   }
   pc = *pcRes;
   memcpy(pc, "<<\r\n", 4);
   lgPc = 4;

   for ( i = 0; i < nbAttrib1; i++) {
      memcpy(pc + lgPc, keyTable1[i].nom, keyTable1[i].lgNom);
      lgPc += keyTable1[i].lgNom;
      pc[lgPc++] = ' ';
      //on cherche si l'attrib courant 1 est dans 2
      for ( j = 0; j < nbAttrib2; j++ ) {
         if ( keyTable1[i].lgNom == keyTable2[j].lgNom ) {
            if ( (keyTable2[j].nom != 0) && (memcmp((char *)keyTable1[i].nom, (char *)keyTable2[j].nom, keyTable1[i].lgNom) == 0) )
               break;
         }
      }
      if ( j != nbAttrib2 ) {
         // on mélange les attribs 1 et 2
         keyTable2[j].nom = 0;
         pc1 = (char *)keyTable1[i].attrib;
         i1 = 0;
         while ( pc1[i1] == ' ' )
            ++i1;
         switch ( pc1[i1] ) {
         case '<':
            pc[lgPc++] = '<';
            pc[lgPc++] = '<';
            memcpy(pc + lgPc, keyTable1[i].attrib + i1 + 2, keyTable1[i].lgAttrib - i1 - 4);
            lgPc += keyTable1[i].lgAttrib - i1 - 4;
            pc[lgPc++] = ' ';
            pc2 = (char *)keyTable2[j].attrib;
            i2 = 0;
            while ( pc2[i2] == ' ' )
               ++i2;
            memcpy(pc + lgPc, keyTable2[j].attrib + i2 + 2, keyTable2[j].lgAttrib - i2 - 4);
            lgPc += keyTable2[j].lgAttrib - i2 - 4;
            memcpy(pc + lgPc, ">>", 2);
            lgPc += 2;
            break;
         case '[':
            pc[lgPc++] = '[';
            memcpy(pc + lgPc, keyTable1[i].attrib + i1 + 1, keyTable1[i].lgAttrib - i1 - 2);
            lgPc += keyTable1[i].lgAttrib - i1 - 2;
            pc[lgPc++] = ' ';
            pc2 = (char *)keyTable2[j].attrib;
            i2 = 0;
            while ( pc2[i2] != '[' )
               ++i2;
            ++i2;
            while ( i2 < keyTable2[j].lgAttrib ) {
               k = tailleAttrib(pc2 + i2);
               memcpy(lBuf, pc2 + i2, k);
               lBuf[k] = 0;
               l = findWinBuf(lBuf, (char *)keyTable1[i].attrib, keyTable1[i].lgAttrib);
               if ( l == -1 ) {
                  memcpy(pc + lgPc, lBuf, k);
                  pc[lgPc++] = ' ';
               }
               i2 += k;
               while ( pc2[i2] == ' ' )
                  ++i2;
            }
            //memcpy(pc + lgPc, keyTable2[j].attrib + i2 + 1, keyTable2[j].lgAttrib - i2 - 2);
            //lgPc += keyTable2[j].lgAttrib - i2 - 2;
            pc[lgPc++] = ']';
            break;
         default:
            memcpy(pc + lgPc, keyTable1[i].attrib + i1, keyTable1[i].lgAttrib - i1);
            lgPc += keyTable1[i].lgAttrib - i1;
            break;
         }
      }
      else {
         //on insère l'attrib 1
         memcpy(pc + lgPc, keyTable1[i].attrib, keyTable1[i].lgAttrib);
         lgPc += keyTable1[i].lgAttrib;
      }
      pc[lgPc++] = '\r';
      pc[lgPc++] = '\n';
   }
   //on traite les attribs de 2 qui ne sont pas dans 1
   for (i = 0; i < nbAttrib2; i++) {
      if ( keyTable2[i].nom != 0 ) {
         //on insère l'attrib
         memcpy(pc + lgPc, keyTable2[i].nom, keyTable2[i].lgNom);
         lgPc += keyTable2[i].lgNom;
         pc[lgPc++] = ' ';
         memcpy(pc + lgPc, keyTable2[i].attrib, keyTable2[i].lgAttrib);
         lgPc += keyTable2[i].lgAttrib;
         pc[lgPc++] = '\r';
         pc[lgPc++] = '\n';
      }
   }

   memcpy(pc + lgPc, ">>", 2);
   lgPc += 2;

   if ( newDict1 )
	   delete newDict1;

   *lgRes = lgPc;
   return *pcRes;   
}

#undef LG_LBUF

//*********************************************************************************************************************************
//*********************************************************************************************************************************
// formatage du texte de bas de page.

float C_pdfFile::setBdpOrientation(float o) {
double pi = 3.1415926535;

   if ( (o > 90) || (o < 0) )
      o = 0;

   o = (float)((o * pi) / 180);

   return this->bdpOrientation = o;
}

int C_pdfFile::setBdpX(int x) {
   return this->bdpX = x;
}

int C_pdfFile::setBdpY(int y) {
   return this->bdpY = y;
}

int C_pdfFile::setBdpSize(int s) {
   return this->bdpSize = s;
}

void C_pdfFile::setBdpRGB(float r, float g, float b) {
   this->bdpRgb = r;
   this->bdprGb = g;
   this->bdprgB = b;
}


#define COLOR_SEPARATOR ','
#define COLOR_DYNAMIC 255

void C_pdfFile::setBdpRGB(char *str) {
float f;
int i;
float *tRGBfloat[] = {
   &this->bdpRgb,
   &this->bdprGb,
   &this->bdprgB
};

   i = 0;
   while ( i < 3 ) {
      f = (float)atof(str);
	  if ( f > COLOR_DYNAMIC )
		   f = COLOR_DYNAMIC;
      if ( f >= 0 ) {
	      *tRGBfloat[i++] = f / COLOR_DYNAMIC;
      }
      while ( (*str != COLOR_SEPARATOR) && (*str != 0) )
         ++str;
      if ( *str == 0 ) {
         break;
      }
      ++str;
   }
}
#undef COLOR_SEPARATOR
#undef COLOR_DYNAMIC

int C_pdfFile::setBdpFontName(int i) {
   if ( (i < 0) || (i > ZAPFDINGBATS_FONT) ) {
      i = HELVETICA_FONT;
   }

   this->bdpFontName = i;

   return i;
}

bool C_pdfFile::insertPageNumberInPage(bool b) {
C_pdfObject *pdfObj;
char *pc;

   if ( this->pageNumberFontObj == 0 ) {
      pdfObj = new C_pdfObject();
      pc = "/Font";
      pdfObj->insertAttrib("/Type", pc, strlen(pc), 0);
      pc = (char *)fontNames[this->pageNumberFontName];
      pdfObj->insertAttrib("/BaseFont", pc, strlen(pc), 0);
      pc = "/Type1";
      pdfObj->insertAttrib("/Subtype", pc, strlen(pc), 0);
      pc = "/WinAnsiEncoding";
      pdfObj->insertAttrib("/Encoding", pc, strlen(pc), 0);
      this->pageNumberFontObj = this->insertObj(pdfObj);
      delete pdfObj;

      sprintf(this->pageNumberFontStr, "/F255 %i 0 R", this->pageNumberFontObj);
   }

   this->insertPageNumber = b;

   return b;
}

int C_pdfFile::setPageNumberX(int x) {
   return this->pageNumberX = x;
}

int C_pdfFile::setPageNumberY(int y) {
   return this->pageNumberY = y;
}

int C_pdfFile::setPageNumberSize(int s) {
   return this->pageNumberSize = s;
}

int C_pdfFile::setPageNumberFontName(int i) {
   if ( (i < 0) || (i > ZAPFDINGBATS_FONT) ) {
      i = HELVETICA_FONT;
   }

   this->pageNumberFontName = i;

   return i;
}

void C_pdfFile::setPageNumberRGB(float r, float g, float b) {
   this->pageNumberRgb = r;
   this->pageNumberrGb = g;
   this->pageNumberrgB = b;
}


#define COLOR_SEPARATOR ','
#define COLOR_DYNAMIC 255

void C_pdfFile::setPageNumberRGB(char *str) {
float f;
int i;
float *tRGBfloat[] = {
   &this->pageNumberRgb,
   &this->pageNumberrGb,
   &this->pageNumberrgB
};

   i = 0;
   while ( i < 3 ) {
      f = (float)atof(str);
	  if ( f > COLOR_DYNAMIC )
		   f = COLOR_DYNAMIC;
      if ( f >= 0 )
	      *tRGBfloat[i++] = f / COLOR_DYNAMIC;
      while ( (*str != COLOR_SEPARATOR) && (*str != 0) )
         ++str;
      if ( *str == 0 ) {
         break;
      }
      ++str;
   }
}
#undef COLOR_SEPARATOR
#undef COLOR_DYNAMIC

int C_pdfFile::setPageNumberFirstNum(int num) {
   return this->pageNumberFirstNum = num;
}

float C_pdfFile::setPageNumberOrientation(float o) {
double pi = 3.1415926535;

   if ( (o > 90) || (o < 0) )
      o = 0;

   o = (float)((o * pi) / 180);

   return this->pageNumberOrientation = o;
}

//*********************************************************************************************************************************
//*********************************************************************************************************************************

int C_pdfFile::setPageRotation(int pr) {
   if ( (pr != 90) && (pr != 180) && (pr != 270) )
       return this->pageRotation;
   return this->pageRotation = pr;
}

//*********************************************************************************************************************************
//*********************************************************************************************************************************

char *C_pdfFile::getObj(int idObj, char *destBuf, int *lgDestBuf, int *lgObj, C_pdfObject **pParam, bool getAllocatedPdfObj) {
int i;
C_pdfObject *p, *pPdfCompObj;
bool allocate;

    p = NULL;
	pPdfCompObj = 0;
	if ( pParam )
        pPdfCompObj = *pParam;
    allocate = false;
    i = this->xrefTable->getOffSetObj(idObj);
    if ( i == 0 ) {
        // c'est peut être un objet compressé et il faut aller le chercher
        i = this->xrefTable->getCompObj(idObj);
        if ( i != 0 ) {
        int first;

            if ( pPdfCompObj != NULL ) {
                if ( pPdfCompObj->getNumObj () == i )
                    p = pPdfCompObj;
                else {
                    allocate = true;
                }
            }
            else {
                allocate = true;
            }
            if ( allocate ) {
                i = this->xrefTable->getOffSetObj(i);
                fseek(this->pf, i, SEEK_SET);
                fread(destBuf, 1, *lgDestBuf, this->pf);
                p = new C_pdfObject(destBuf, *lgDestBuf, compressedKeyWords, true);
            }

            destBuf = p->getObjectFromCompObj(idObj, destBuf, lgDestBuf, lgObj);
        }
        else {
            fprintf(stderr, "offset null, objet %i\r\n", idObj);
        }
    }
    else {
        fseek(this->pf, i, SEEK_SET);
        fread(destBuf, 1, *lgDestBuf, this->pf);
        *lgObj = findWinBuf("endobj", destBuf, *lgDestBuf);
        while ( *lgObj == -1 ) {
            delete destBuf;
            *lgDestBuf *= 2;
            destBuf = new char[*lgDestBuf];
            fseek(this->pf, i, SEEK_SET);
            fread(destBuf, 1, *lgDestBuf, this->pf);
			*lgObj = findWinBuf("endobj", destBuf, *lgDestBuf);
        }
    }

    if ( allocate )
        if ( getAllocatedPdfObj )
            *pParam = p;
        else
            delete p;

    return destBuf;
}
