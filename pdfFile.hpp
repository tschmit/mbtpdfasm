#ifndef __PDFFILE__
#define __PDFFILE__

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "diversPdf.hpp"
#include "pdfObject.hpp"
#include "pdfXrefTable.hpp"
#include "diversPdf.hpp"
#include "pdfString.hpp"
#include "md5.h"
#include "rc4.hpp"

#include "pdfEncrypt.hpp"

extern const char *nameOfTrees[];

#pragma warning(disable: 4514)

/*********************************************************************************
structure d'un fichier de définition:
id_titre | id_parent | rang | n_page | titre
si id_parent vaut 0, c'est un titre de niveau 0.
*/
#define LG_TITRE_OUTLINE 250
#define OUTLINE_FIT  0
#define OUTLINE_FITH 1
#define OUTLINE_FITV 2
typedef struct {
   int id;  //identifiant dans le fichier de description > 0
   int idP; // identifiant du parent
   int rg;  //rang parmi les fils du parents > 0
   int nObj; // identifiant d'objet dans le fichier PDF
   pdfString *titre;
   int count;
   int parent;
   int prev;
   int next;
   int first;
   int last;
   int dest;
   int a;
   int fit;
} T_outlines;

#define NB_OUTLINES 100

#define OUTLINES_FLUSH_FORMAT_MBT 1

extern const char *outlineFitChars[];

/*********************************************************************************/

#define NB_ATTRIB 20
#define LG_ATTRIB 1000

#define PDF_MERGE_CANT_MERGE_READ_ONLY_FILE       0x01
#define PDF_MERGE_CANT_OPEN_ORG_FILE              0x02
//#define PDF_FILE_CANT_MERGE_FILE_WITH_COUNT_SUP_1 0x03
#define PDF_FILE_CANT_INSERT_STATUS_FALSE         0x04
#define PDF_FILE_CANT_INSERT_PAGE                 0x05
#define PDF_MERGE_CANT_MERGE_A_CLOSED_FILE        0x06
#define PDF_FILE_CLOSED                           0x07
#define PDF_FILE_CANT_INSERT_PAGE_0               0x08
#define PDF_FILE_CANT_INSERT_PAGE_OFFSET_IS_NULL  0x09
#define PDF_MERGE_CANT_MERGE_ENCRYPTED_FILE       0x0A
#define PDF_MERGE_CANT_HANDLE_ENCRYPT_VERSION     0x0B

enum openMode {_read_, _write_};
#define read _read_
#define write _write_
#define LG_NOM 255
#define NB_CHILDS 50

#define AF_LIST_INCREASE 50

typedef struct S_nameStruct {
   pdfString         str;
   int               nObj;
   int               version;
   struct S_nameStruct *next;
} T_name;

typedef struct {
   const char *name;
   T_name **tree;
   int rootObj;
} T_name2tree;


//gestion des pages *****************************
#define PTN_TYPE_PAGES 1
#define PTN_TYPE_PAGE  2

typedef struct S_pageTreeNode {
   int nObj;
   int type;
   struct S_pageTreeNode *parent;
   int nbKids;
   struct S_pageTreeNode **kids;
   T_attrib *listAttrib;
   int nbInheritAttrib; //utilisé pour l'accélération du traitement des attributs hérités cf ::insertPage, utilisé uniquement quand
                        // type vaut PTN_TYPE_PAGE
} T_pageTreeNode;

typedef T_pageTreeNode *T_pPTN;

//************************************************

class C_pdfFile {
   private:
      static char* debug_pc;

      C_pdfXrefTable *xrefTable;
      openMode mode;
      char nom[LG_NOM]; //nom du fichier
      FILE *pf;
      bool status;    //résultat de l'ouverture du fichier/objet
      int rootPages; //numéro de l'objet de type /Pages

      //gestion des signets **********************************************************
      bool keepOutlines;
      int outlines; // numéro de l'objet racine des signets
      int firstOutlines;
      int lastOutlines;
      int countOutlines;
      int nbOutlines;
      bool outlinesLoaded;
      bool outlinesFlushed;
      T_outlines *pOutlines;
      int sizeof_pOutlines;
      int readOutlines(int idOutline);
      int pOutlinesGrowth(int growth);

      //gestion des pages *************************************************************
      T_pageTreeNode **pagesList;
      int nbPages; //nombre de pages dans le fichier PDF

      int pPage; // utilisé par getFirstPage et getNextPage
                 //si il vaut -1, il y a un pb
      int buildPagesTree(int nObj, T_pageTreeNode **currentPTN, T_pageTreeNode *parent, int cptInheritAttr);
      int *childs;
      int sizeof_childs;
      int pChilds;
      int addChild(int i);
	  int getObjAsPageNumber(int nObj);
	  int getPageAsObjNumber(int nPage);
      bool isObjAPage(int nObj);
      T_pageTreeNode *rootTreeNode;
      char *getValueFromParent(const char *, C_pdfFile *, int);

      //gestion des formulaires *******************************************************
      int rootAcroForm;
      int buildAcroFormTree(char *pAF);
      int *afList;
      int pAfList;
      int lgAfList;
      pdfString acroDA;
      int acroDR;
      int getFirstAcroFormObj();
      int getNextAcroFormObj();

      //int findInheritRes(C_pdfFile *org, char *fBuf, int lg, char **pcRes);
      int makeRes(C_pdfFile *org, char *fBuf, int lg, char **pcRes);
      int makeAttrib(C_pdfFile *org, char *fBuf, int lgOrgBuf, char **pcRes, char *add, int lgAdd);

      bool fast; // version rapide, on ne gère pas les options
      bool closed; //protection plus rien n'est possible aprés l'appel à la fonction close

      int mergeError; // code d'erreur de la fonction merge en cas de retour -1

      int insertObj(C_pdfFile *org, int nObj);
      int insertAndAppendObj(C_pdfFile *org, int nObj, const char *append, int lgAppend);


      // gestion des metadata *********************************************************
      pdfString **pdfMetadataStrValue;

      //gestion de l'encryption *******************************************************
      bool forceNoEncrypt;

      //********* gestion de l'option -u.
      bool fullMergeFlag;

      //*******************************************************************************
      // gestion des names ************************************************************
      T_name *Dests;
      T_name *AP;
      T_name *JavaScript;
      T_name *Pages;
      T_name *Templates;
      T_name *IDS;
      T_name *URLS;
      T_name **currentTreeRoot;
      T_name *currentTreeLeaf;
      T_name2tree *n2t;
      T_name *buildNameTree(int nObj);
      int rootNamesObj;
      int pN2T;
      int mergeNames(C_pdfFile *orgNames);
      int flushNames();
      inline int getRootNamesObj() {return this->rootNamesObj;}
      bool namesLoaded;
      
      //**********************************************************
      // gestion de l'insertion d'information
      const char *bdpStr;
      int bdpFontObj;
      int bdpObj;
      char bdpFontStr[30];
      int bdpFontName;
      float bdpOrientation;
      int bdpX, bdpY, bdpSize;
      float bdpRgb, bdprGb, bdprgB;

      int pageNumberFontObj;
      char pageNumberFontStr[30];
      bool insertPageNumber;
      int pageNumberX, pageNumberY, pageNumberSize;
      float pageNumberOrientation;
      int pageNumberFontName;
      float pageNumberRgb, pageNumberrGb, pageNumberrgB;
      int pageNumberFirstNum;

      //************************************************************
      //rotation des pages
      int pageRotation;

	  //************************************************************
      //gestion des polices de caractères (font)
	  //Abandonné à ce jour
	  C_pdfObject *fontList;

	  //************************************************************

      char *mergeDict(T_keyWord *ptkw, C_pdfFile *org1, const char *res1, int lgRes1, const char *res2, int lgRes2, char **, int *);

	  //******
	  // utilitaires
	  void setIntInDynBuf(int v, int **buf, int *sizeof_buf, int i);

   public:

      inline FILE *getFile() {return pf;}

      static const char *pdfMetadataStrName[];

      C_pdfFile(char *nom, openMode mode);
      ~C_pdfFile();

      bool close();

      //********************************************
      // merge renvoi -1 en cas d'erreur, le code d'erreur est alors disponible via getMergeError
      int merge(char *nomOrg);
      int merge(C_pdfFile *org);
      int fullMerge(char *nomOrg);
      inline int getMergeError() {return mergeError;}
      int getFirstPage();
      int getNextPage();
      int getPageN(int n);
      T_pageTreeNode *getPagePTN(int nPage);
      int insertPage(C_pdfFile *org, int nPage, int parent);
      inline int getNbPages(){ return nbPages; }
      inline int getRootPagesObjNum() { return rootPages; }

      //outlines
      bool loadOutlines(char *fileName);
      bool loadOutlines(C_pdfFile *org);
      bool flushOutlines();
	  FILE *flushFormatedOutlinesToDest(int format, FILE *dest);
      bool setKeepOutlines(bool b) {return this->keepOutlines = b;}

      bool getStatus();

      inline const char *getFileName() {return (const char *)nom; }
      inline C_pdfXrefTable *getXrefTable() { return xrefTable; }

      int getRootAcroForm();
      int getNbAcroForm();

      // gestion des metadata *****************************************************
      int setMetadataStr(const char *, char *, int);
      int getMetadataStr(char *dest, int lgDest, const char *metaName);
      static int getNumberOfMetadata();

      // gestion de l'encryption **************************************************
      bool setForceNoEncrypt(bool);
      pdfEncrypt *encrypt;
      int setEncryptStandardP(int P);
      int getEncryptStandardP();

      C_pdfFile *setOptions(char *mdpU, char *mdpO, bool masterModeNoEncrypt, char *restrict, int lgEncryptKey, int pMetadataStr, char **metadataStr);

      // ***************************************************************************
      // gestion de l'insertion de texte
      const char *setBdpStr(const char *pcc);
      float setBdpOrientation(float o);
      int setBdpX(int x);
      int setBdpY(int y);
      int setBdpSize(int s);
      void setBdpRGB(float r, float g, float b);
      void setBdpRGB(char *str);
      int setBdpFontName(int i);

      // ***************************************************************************
      // gestion de l'insertion de numéro de page
      bool insertPageNumberInPage(bool b);
      int setPageNumberX(int x);
      int setPageNumberY(int y);
      int setPageNumberSize(int s);
      int setPageNumberFontName(int i);
      void setPageNumberRGB(float r, float g, float b);
      void setPageNumberRGB(char *str);
      int setPageNumberFirstNum(int num);
      float setPageNumberOrientation(float o);

      int insertObj(C_pdfObject *pObj);
	     int insertPage(C_pdfObject *pObj);

      int setPageRotation(int pr);

      char *getObj(int idObj, char *destBuf, int *lgDestBuf, int *lgObj, C_pdfObject **pPdfCompObj, bool getAllocatedPdfObj);
      
};

#endif
