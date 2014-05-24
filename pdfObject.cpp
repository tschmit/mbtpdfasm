#include "pdfObject.hpp"
#include "pdfXrefTable.hpp"
#include "diversPdf.hpp"
#include "zlib.h"
#include "pdfUtils.hpp"
#include <stdlib.h>

#ifdef DEBUG_MEM_LEAK
#ifdef WIN32
   #include <crtdbg.h>
   #define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
   #define new DEBUG_NEW
#endif
#endif

int compareAttribToAttrib(T_attrib *attrib1, bool chain1, T_attrib *attrib2, bool chain2, bool skipIndirectRef) {
T_attrib *attribt;
bool same;
int i;
int ref1, ref2;
    
    same = false;
    do {
        ref1 = findWinBuf(" R", attrib1->valeur, attrib1->lgValeur);
        if ( !skipIndirectRef || (ref1 == -1) ) {
            attribt = attrib2;
            same = false;
            do {
                if ( attrib1->id == attribt->id) {
                    if ( attrib1->lgValeur == attribt->lgValeur) {
                        ref2 = findWinBuf(" R", attrib2->valeur, attrib2->lgValeur);
                        if ( !skipIndirectRef || (ref2 == -1) ) {
                            for (i = 0; i < attrib1->lgValeur; i++) {
                                if ( attrib1->valeur[i] != attribt->valeur[i] ) {
                                    break;
                                }
                            }
                            if ( i == attrib1->lgValeur) 
                                same = true;
                        }
                    }
                }
                attribt = attribt->next;
            } while (attribt && chain2 && !same);
        }
        attrib1 = attrib1->next;
    } while (attrib1 && chain1 && same);
    
    if ( !same )
        return -1;
    
    return 0;
}

int C_pdfObject::initVar() {
    this->listAttrib = 0;
    this->stream = 0;
    this->lgStream = 0;
    this->sizeofStream = 0;
    this->next = 0;
    this->numObj = 0;
    this->loadStatus = true;

    return 0;
}

C_pdfObject::C_pdfObject() {
   this->initVar();
}

C_pdfObject::C_pdfObject(char *pBuf, int lgPBuf, T_keyWord *pKWT, bool loadStream) {
int lgBufUtil, streamPos, streamLg, pb, j, idKey;
    
    this->loadStatus = false;    
    this->initVar();
	

	if ( pBuf[0] >= '0' &&  pBuf[0] <= '9') {
        if ( (lgBufUtil = findWinBuf("endobj",pBuf, lgPBuf)) == -1) {
            return;
        }

        numObj = atoi(pBuf);
    
        streamPos = findWinBuf("stream", pBuf, lgBufUtil);
        streamLg = 0;
        if ( streamPos != -1 ) {
            lgBufUtil = streamPos;
            streamPos += 7; // 6+1 est le nombre de caractère dans 'stream' avec un caractère LF
            if ( pBuf[streamPos - 1] == 0x0D ) {
                ++streamPos;
            }
        }
	}
	else {
		//ce n'est pas vraiment un objet, mais peut etre un dictionnaire.
		return;
	}

    pb = 0;
    while ( 1 ) {
        while ( (pBuf[pb] != '/') && (pb < lgBufUtil) )
            pb++;
        if ( pb >= lgBufUtil )
            break; // <=============== c'est ici qu'on sort du while
        idKey = isWaKey(pBuf + pb, pKWT);
        if ( idKey == _notAType ) {
            ++pb;
            while ( (pBuf[pb] != ' ') && (pBuf[pb] != '/') && (pBuf[pb] != '[') && (pBuf[pb] != '<') && (pBuf[pb] != '(') && (pb < lgBufUtil) )
               ++pb;
            if ( pb < lgBufUtil )
               pb += tailleAttrib(pBuf + pb);
        }
        else {
            pb += keyLen(idKey, pKWT);
            while ( (unsigned char)(pBuf[pb]) <= ' ' )
                ++pb;
            j = tailleAttrib(pBuf + pb);
            insertAttrib(nomAttrib(idKey, pKWT), pBuf + pb, j, idKey);
            if ( idKey == _Length ) {
                streamLg = atoi(pBuf + pb);
            }
            pb += j;
        }
        
    }
    if ( loadStream && (streamPos != -1) && streamLg ) {
        appendStream(pBuf + streamPos, streamLg);
    }
}

C_pdfObject::C_pdfObject(C_pdfObject *prev) {
   this->initVar();

   prev->next = this;
}

C_pdfObject::~C_pdfObject() {
T_attrib *pta, *ptaNext;
   if ( this->stream != 0 )
      delete this->stream;
   pta = this->listAttrib;
   while ( pta ) {
      ptaNext = pta->next;
      if ( pta->valeur ) {
         delete pta->valeur;
      }
      delete pta;
      pta = ptaNext;
   }
}

int C_pdfObject::insertAttrib(T_attrib *pta) {
T_attrib *ptaLoc;
   if ( listAttrib == 0 ) {
      listAttrib = pta;

      return 0;
   }

   ptaLoc = this->listAttrib;
   while ( ptaLoc->next != 0 ) {
      ptaLoc = ptaLoc->next;
   }
   ptaLoc->next = pta;

   return 0;
}

int C_pdfObject::insertAttrib(const char *nom, char *valeur, int lgValeur, int id) {
T_attrib *pta;

   pta = new T_attrib;
   pta->nom = nom;
   pta->valeur = new char[lgValeur + 1];
   memcpy(pta->valeur, valeur, lgValeur);
   pta->valeur[lgValeur] = 0;
   pta->lgValeur = lgValeur;
   pta->next = 0;
   pta->id = id;

   this->insertAttrib(pta);

   return 0;
}

int C_pdfObject::appendStream(char *pc, int lgPc) {
char *lBuf;
   if ( sizeofStream < lgStream + lgPc) {
      lBuf = new char[(lgStream + lgPc) * 2];
      memcpy(lBuf, this->stream, lgStream);
      delete this->stream;
      memcpy(lBuf + lgStream, pc, lgPc);
      this->lgStream += lgPc;
      this->stream = lBuf;

      return 0;
   }

   memcpy(this->stream, pc, lgPc);
   this->lgStream += lgPc;

   return 0;
}

int C_pdfObject::flush(FILE *dest, int num) {
T_attrib *pta;

   fprintf(dest, "%i 0 obj\r\n<<\r\n", num);
   pta = this->listAttrib;
   while ( pta ) {
      fprintf(dest, "%s ", pta->nom);
      fwrite(pta->valeur, 1, pta->lgValeur, dest);
      fprintf(dest, "\r\n");
      pta = pta->next;
   }
   if ( this->stream != 0 ) {
      fprintf(dest, "/Length %i\r\n", this->lgStream);
   }
   fprintf(dest, ">>\r\n");
   if ( this->stream != 0 ) {
      fprintf(dest, "stream\r\n");
      fwrite(this->stream, 1, this->lgStream, dest);
      fprintf(dest, "\r\nendstream\r\n");
   }
   fprintf(dest, "endobj\r\n");

   return 0;
}


const char *C_pdfObject::getAttrib(int idAttrib) {
    T_attrib *pta = this->listAttrib;

    if ( pta == NULL ) return NULL;

    while ( (pta) ) {
       if ( pta->id == idAttrib )
           return (const char *)pta->valeur;
       pta = pta->next;
    }

    return NULL;
}

//**************************************************************************************
// ATTENTION....... cette fonction alloue de la mémoire.. il faut la libérer;
char *C_pdfObject::getStream(char *destBuf, int *lgDestBuf, int *lgStream, bool filter) {
const char *pcc;
int idKey, pred, cols, i, j, k;

   *lgStream = this->lgStream;

   if ( *lgStream == 0 )
       return destBuf;
   
   if ( (destBuf == 0) || (*lgDestBuf < *lgStream) ) {
       *lgDestBuf = *lgStream;
       destBuf = new char[*lgDestBuf];
   }

   pcc = getAttrib(__Filter);
   if ( !filter || (pcc == NULL)) {
       memcpy(destBuf, this->stream, this->lgStream);
   }
   else {
       idKey = isWaKey((char *)pcc, filterKeyWords); 
       switch ( idKey ) {
           case _FlateDecode:
           z_stream strm;
           int ret;
               strm.zalloc   = Z_NULL;
               strm.zfree    = Z_NULL;
               strm.opaque   = Z_NULL;
               strm.avail_in = 0;
               strm.next_in  = Z_NULL;
               ret = inflateInit(&strm);
               if (ret != Z_OK) {
                   *lgStream = 0;
                   break;
               }

               strm.avail_in  = this->lgStream;
               strm.next_in   = (Bytef *)this->stream;
               do {
                   strm.avail_out = *lgDestBuf;
                   strm.next_out  = (Bytef *)destBuf;

                   ret = inflate(&strm, Z_NO_FLUSH);

                   switch (ret) {
                   case Z_NEED_DICT:
                       ret = Z_DATA_ERROR;
                   case Z_DATA_ERROR:
                   case Z_MEM_ERROR:
                      (void)inflateEnd(&strm);
                      *lgStream = 0;
                      break;
                   }

                   if ( strm.avail_out <= 0 ) {
					   fprintf(stderr, "erreur de décompression");
                       //il faut agrandir fBuf
                   }
               } while (strm.avail_out == 0);

               *lgStream = *lgDestBuf - strm.avail_out;
               (void)inflateEnd(&strm);
			   if ((pcc = this->getAttrib(__DecodeParms)) != NULL) {
				   pred = pdfGetParmValueAsInt("/Predictor", pcc, strlen(pcc));
				   if (pred > INT_MIN) {
					   cols = pdfGetParmValueAsInt("/Columns", pcc, strlen(pcc));
					   switch (pred) {
					   case 12:						   
						   for (i = 0; i < cols; i++) {
							   destBuf[i] = destBuf[i + 1];
						   }
						   k = cols;
						   i++;
						   while (i < strm.total_out) {
							   //on zappe l'octet de prédiction
							   i++;
							   for (j = 0; j < cols; j++) {
								   destBuf[k] = destBuf[k - cols] + destBuf[i];
								   k++; i++;
							   }
						   }
						   break;
					   }
				   }
			   }

			   break;
           default:
               *lgStream = 0;
               break;
       }
   }

   return destBuf;
}

char *C_pdfObject::getObjectFromCompObj(int k, char *destBuf, int *lgDestBuf, int *lgObj) {
int first, i, j;

    //il faut tester ici qu'il s'agit bien d'un objec compressé, en testant le type par exemple.
    
    this->getStream(destBuf, lgDestBuf, lgObj, true);

    first = atoi(this->getAttrib(_First));
    i = atoi(destBuf);
    j = 0;
    while ( (i != k) && (j < first) ) {
        while ( (destBuf[j] >= '0') && (destBuf[j] <= '9'))
            ++j;
        while ( (destBuf[j] < '0') || (destBuf[j] > '9'))
            ++j;
        while ( (destBuf[j] >= '0') && (destBuf[j] <= '9'))
            ++j;
        while ( (destBuf[j] < '0') || (destBuf[j] > '9'))
            ++j;
        i = atoi(destBuf + j);
    }
    while ( (destBuf[j] >= '0') && (destBuf[j] <= '9'))
        ++j;
    while ( (destBuf[j] < '0') || (destBuf[j] > '9'))
        ++j;
    i = atoi(destBuf + j) + first; //offset de l'objet que l'on cherche dans le stream.
    memcpy(destBuf, destBuf + i, *lgDestBuf - i);
    *lgObj = tailleAttrib(destBuf);
    return destBuf;
}

int C_pdfObject::chainObjectToThis(C_pdfObject *ppo) {
C_pdfObject *ppot;
    
    ppot = this;
    while ( ppot->next != 0 )
        ppot = ppot->next;
    ppot->next = ppo;
   
    return 0;
}

//*********************************************************************************
//retourne 0 si au moins un objet de la chaine this est semblable à ppo
int C_pdfObject::compareThisToSingleObject(C_pdfObject *ppo, bool chainThis) {
C_pdfObject *ppot;
    
    if ( ppo == 0 )
        return -1;
    
    ppot = this;
    do {
        if ( compareAttribToAttrib(ppot->listAttrib, true, ppo->listAttrib, true, true) == 0 ) {
            //il faut ici tester le contenu du stream si il existe
            return ppot->numObj;
        }
        ppot = ppot->next;
    } while ( chainThis && ppot);
    
    return -1;
}
