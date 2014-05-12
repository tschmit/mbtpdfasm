#include <stdlib.h>
#include <string.h>

#include "pdfEncrypt.hpp"

#ifdef WIN32
#else
#define _snprintf snprintf
#endif

const char *nullStandardKey = "\x28\xBF\x4E\x5E\x4E\x75\x8A\x41\x64\x00\x4E\x56\xFF\xFA\x01\x08\x2E\x2E\x00\xB6\xD0\x68\x3E\x80\x2F\x0C\xA9\xFE\x64\x53\x69\x7A";

const char *filterStandard = "/Standard";
//*************************************************************************************************
//**************************************************************************************************
//**************************************************************************************************

pdfEncrypt::pdfEncrypt(C_pdfXrefTable *xT) {
/*int i, j;
int lgObj;
char fBuf[LG_FBUF];*/


   this->xrefTable = xT;
   this->pf = 0;

   this->encryptFilter[0] = 0;
   this->encryptV = 0;
   this->encryptLength = 0;
   this->canDecrypt = false;
   this->encryptStandardR = 0;
   this->encryptStandardP = ENCRYPT_FULL_RIGHT;
   this->encryptKey[0] = 0;
   this->encryptKeyLength = 0;
   this->mdpO[0] = 0;
   this->lgMdpO = 0;
   this->mdpU[0] = 0;
   this->lgMdpU = 0;
   this->newMdpU = true;
}


pdfEncrypt::pdfEncrypt(C_pdfXrefTable *xT, FILE *orgPf) {
int i, j;
int lgObj;
char fBuf[LG_FBUF];


   this->xrefTable = xT;
   this->pf = orgPf;

   this->encryptFilter[0] = 0;
   this->encryptV = 0;
   this->encryptLength = 0;
   this->canDecrypt = false;
   this->encryptStandardR = 0;
   this->encryptStandardP = ENCRYPT_FULL_RIGHT;
   this->encryptKey[0] = 0;
   this->encryptKeyLength = 0;
   this->mdpO[0] = 0;
   this->lgMdpO = 0;
   this->mdpU[0] = 0;
   this->lgMdpU = 0;
   this->newMdpU = true;

   i = this->xrefTable->getEncryptObj();
   
   if ( i != 0 ) {
      i = this->xrefTable->getOffSetObj(i);
      fseek(pf, i, SEEK_SET);
      fread(fBuf, 1, LG_FBUF, pf);
      lgObj = LG_FBUF;
      i = findWinPdfObj("/Filter", fBuf, lgObj);
      if ( i != -1 ) {
         for ( j = 0; j < ENCRYPT_FILTER_LENGTH - 1; j++) {
            this->encryptFilter[j] = (fBuf + i + 8)[j];
            if ( this->encryptFilter[j] < ' ' )
               break;
         }
         this->encryptFilter[j] = 0;
      }
      i = findWinPdfObj("/V", fBuf, lgObj);
      if ( i != -1 ) {
         this->encryptV = atoi(fBuf + i + 3);
      }
      i = findWinPdfObj("/Length ", fBuf, lgObj);
      if ( i != -1 ) {
         this->encryptLength = atoi(fBuf + i + 8);
      }
      switch ( this->encryptV ) {
      case 0:
         this->canDecrypt = false;
         break;
      case 1:
         this->encryptLength = 40;
      case 2:
         this->canDecrypt = true;
         i = findWinPdfObj("/R", fBuf, lgObj);
         if ( i != -1 ) {
            encryptStandardR = atoi(fBuf + i + 3);
         }
         i = findWinPdfObj("/P", fBuf, lgObj);
         if ( i != -1 ) {
            encryptStandardP = atoi(fBuf + i + 3);
         }
         i = findWinPdfObj("/O", fBuf, lgObj);
         if ( i != -1 ) {
            j = i + 2;
            while ( (fBuf[j] != '(') && (fBuf[j] != '<') )
               ++j;
            encryptStandardO.copy(fBuf + j);
         }
         i = findWinPdfObj("/U", fBuf, lgObj);
         if ( i != -1 ) {
            j = i + 2;
            while ( (fBuf[j] != '(') && (fBuf[j] != '<') )
               ++j;
            encryptStandardU.copy(fBuf + j);
         }
          break;
      case 3:
         this->canDecrypt = false;
         break;
      default:
         this->canDecrypt = false;
      }
       this->encryptKeyLength = this->encryptLength / 8;
       if ( this->canDecrypt == true ) {
         //mise à jout de this->canDecrypt
         this->isDecryptPossible();
      }
   }
   
}

pdfEncrypt::~pdfEncrypt() {
}

//**************************************************************************************************
//**************************************************************************************************
//**************************************************************************************************

int pdfEncrypt::makeEncryptKey(char * pass, int lg) {
md5_state_t mst;
char fBuf[255];
int i, j;

   i = 0;
   if ( pass != 0 ) {
      for (;i < lg; i++) {
         fBuf[i] = pass[i];
      }

   }
   for (j = 0; i < 32; i++, j++) {
      fBuf[i] = nullStandardKey[j];
   }

   md5_init(&mst);
   md5_append(&mst, (const unsigned char *)fBuf, 32);
   i = this->encryptStandardO.snprint(fBuf, 255, false);
   md5_append(&mst, (const unsigned char *)fBuf, i);
   md5_append(&mst, (const unsigned char *)&this->encryptStandardP, 4);
   i = xrefTable->getID(0)->snprint(fBuf, 255, false);
   md5_append(&mst, (const unsigned char *)fBuf, i);
   md5_finish(&mst, (unsigned char *)fBuf);
   if ( this->encryptStandardR == 3 ) {
      for (i=0; i < 50; i++) {
         md5_init(&mst);
         md5_append(&mst, (unsigned char *)fBuf, 16);
         md5_finish(&mst, (unsigned char *)fBuf);
      }
   }

   for (i=0; i < 16; i++ ) {
      this->encryptKey[i] = fBuf[i];
   }

   return 0;
}

/***************************************************************/

int pdfEncrypt::getEncryptKey(char *dest, int lg) {
int i;

  if ( (lg < this->encryptKeyLength) || !dest )
     return 0;
  for (i = 0; i < this->encryptKeyLength; i++) {
     dest[i] = encryptKey[i];
  }

  return i;
}

//**************************************************************************************************

int pdfEncrypt::encryptPdf(FILE *pf2) {
md5_state_t mst;
int sizeof_fBuf;
unsigned char *fBuf, *pc;
rc4_key key;
int i, j, lgObj, k;
int off;
char keyBuf[16];
char keyBuf2[16];

   xrefTable->makeID();

   sizeof_fBuf = LG_FBUF;
   fBuf = new unsigned char[sizeof_fBuf];

   sprintf(this->encryptFilter, "/Standard");
   if ( this->encryptKeyLength == 0 )
       this->encryptKeyLength = 5;
   this->encryptV = 1;
   if ( this->encryptKeyLength > 5 )
       this->encryptV = 2;
   this->encryptLength = this->encryptKeyLength * 8;
   this->encryptStandardR = 2;
   if ( this->encryptKeyLength > 5 )
       this->encryptStandardR = 3;
   this->encryptStandardP |= ENCRYPT_REVISION3_RIGHT;

   //*************************************************************
   // création de l'entrée propriétaire
      //step 1 ***************************
   pc = (unsigned char *)&this->mdpO[0];
   k = this->lgMdpO;
   if ( *pc == 0 ) {
      pc = (unsigned char *)&this->mdpU[0];
      k = this->lgMdpU;
   }
   for (i = 0; i < k; i++ ) {
      fBuf[i] = pc[i];
   }
   for (j = 0; i < 32; i++, j++) {
      fBuf[i] = nullStandardKey[j];
   }
      //step 2 ***************************
   md5_init(&mst);
   md5_append(&mst, (const unsigned char *)fBuf, 32);
   md5_finish(&mst, fBuf);
      //step 3 Rev 3 only ***************************
   if ( this->encryptStandardR == 3 ) {
      for (i = 0; i < 50; i++) {
         md5_init(&mst);
         md5_append(&mst, (const unsigned char *)fBuf, 16);
         md5_finish(&mst, fBuf);
      }
      memcpy(keyBuf, fBuf, this->encryptKeyLength);
   }
      //step 4 ***************************
   prepare_key(fBuf, this->encryptKeyLength, &key);
      //step 5 ***************************
   for (i = 0; i < this->lgMdpU; i++ ) {
      fBuf[i] = this->mdpU[i];
   }
   for (j = 0; i < 32; i++, j++) {
      fBuf[i] = nullStandardKey[j];
   }
      //step 6 ***************************
   rc4(fBuf, 32, &key);
      //step 7 Rev 3 only ***************************
   if ( this->encryptStandardR == 3 ) {
      for ( i = 1; i <= 19; i++) {
         for (j = 0; j < this->encryptKeyLength; ++j) {
            keyBuf2[j] = (char)(keyBuf[j] ^ (char)i);
         }
         prepare_key((unsigned char *)keyBuf2, this->encryptKeyLength, &key);
         rc4(fBuf, 32, &key);
      }
   }
      //step 8 ***************************
   this->encryptStandardO.copy((const char *)fBuf, 32, _carType);

   //*************************************************************
   // création de l'entrée utilisateur
   this->makeEncryptKey(this->mdpU, this->lgMdpU);
   switch ( this->encryptStandardR ) {
   case 3:
      memcpy(keyBuf, this->encryptKey, this->encryptKeyLength);
      md5_init(&mst);
      for (i = 0; i < 32; i++) {
         fBuf[i] = nullStandardKey[i];
      }
      md5_append(&mst, (const unsigned char *)fBuf, 32);
      i = xrefTable->getID(0)->snprint((char *)fBuf, LG_FBUF, false);
      md5_append(&mst, (const unsigned char *)fBuf, i);
      md5_finish(&mst, (unsigned char *)fBuf);
      prepare_key((unsigned char *)this->encryptKey, this->encryptKeyLength, &key);
      rc4(fBuf, 16, &key);
      for (i = 1; i <= 19; i++) {
         for (j= 0; j < this->encryptKeyLength; ++j) {
            keyBuf2[j] = (char)(keyBuf[j] ^ (char)i);
         }
         prepare_key((unsigned char *)keyBuf2, this->encryptKeyLength, &key);
         rc4(fBuf, 16, &key);
      }
      break;
   default:
      prepare_key((unsigned char *)this->encryptKey, this->encryptKeyLength, &key);
      for (i = 0; i < 32; i++)
         fBuf[i] = nullStandardKey[i];
      rc4(fBuf, 32, &key);
      break;
   }
   this->encryptStandardU.copy((const char *)fBuf, 32, _carType);

   for ( i = 1; i < xrefTable->getNbObj(); i++ ) {
      off = xrefTable->getOffSetObj(i);
      if ( off != 0 ) {
         fseek(pf, off, SEEK_SET);
         fread(fBuf, 1, sizeof_fBuf, pf);
         j = findWinBuf("endobj", (char *)fBuf, sizeof_fBuf);
         while ( (j == -1) || (j > sizeof_fBuf - 255) ) {
            // on agrandit fBuf
            pc = new unsigned char[sizeof_fBuf + 2 * LG_FBUF];
            memcpy(pc, fBuf, sizeof_fBuf);
            fread(&pc[sizeof_fBuf], 1, 2 * LG_FBUF, pf);
            sizeof_fBuf += 2 * LG_FBUF;
            delete fBuf;
            fBuf = pc;
            j = findWinBuf("endobj", (char *)fBuf, sizeof_fBuf);
         }
         lgObj = encryptObj((char *)fBuf, j + 6, sizeof_fBuf, true); // 6 = endobj
         xrefTable->setOffset(i, ftell(pf2));
         fwrite(fBuf, 1, lgObj, pf2);
         fwrite("\r\n", 1, 2, pf2);
      }
      else {
         if ( (xrefTable->getStatus(i) != 'f') )
            fprintf(stderr, "offset null 6, objet %i !!\r\n", i);
      }
   }

   delete fBuf;

   return 0;
}

//**************************************************************************************************
// on chiffre à partir de pc + k et sur une longueur de lgAChiffrer
#define  LG_LBUF 255

int pdfEncrypt::encryptBuf(unsigned char *buf, int lgBuf, unsigned char *ckey, int lgCKey) {
rc4_key key;

    prepare_key(ckey, lgCKey, &key);
    rc4(buf, lgBuf, &key);

    return lgBuf;
}


int pdfEncrypt::encryptObj(char *pObj, int lgObj, int sizeofBuf, bool wholePdfEncryption) {
int i, j, k, l, m, nObj, lgKey, lgStr;
stringType type;
int streamLg, streamPos, max;
char lBuf[LG_LBUF];
char key[LG_LBUF];
md5_state_t mst;
int sizeof_tBuf = LG_TBUF + 1;
char *tBuf;
pdfString pdfStr;
pdfEncrypt *org;

   org = this;

   nObj = atoi(pObj); //numéro de l'objet

   //ici on assume que le generation number vaut 0
   lgKey = org->getEncryptKey(key, LG_FBUF);
   key[lgKey++] = ((char *)&nObj)[0];
   key[lgKey++] = ((char *)&nObj)[1];
   key[lgKey++] = ((char *)&nObj)[2];
   key[lgKey++] = 0;
   key[lgKey++] = 0;
   md5_init(&mst);
   md5_append(&mst, (const unsigned char *)key, lgKey);
   md5_finish(&mst, (unsigned char *)key);

   if ( lgKey > 16 )
       lgKey = 16;

   streamPos = findWinBuf("stream", pObj, lgObj);
   max = lgObj;
   if ( streamPos != -1 ) {
      streamLg = 0;
      max = streamPos;
      streamPos += 6; // longeur de "stream"
      if ( pObj[streamPos] == 0x0D ) {
         ++streamPos; // CR
      }
      ++streamPos;  // LF

      i = findWinBuf("/Length ", pObj, max);
     
      if ( i != - 1 ) {
         i += 8;
         while ( (pObj[i] < '0') || (pObj[i] > '9') )
            ++i; //on se cale sur le nombre suivant /length
         j = atoi(pObj + i);
         while ( (pObj[i] >= '0') && (pObj[i] <= '9') )
            ++i; // on zappe le nombre suivant /length
         ++i; // on décide qu'il y a un espace
         if ( (pObj[i] >= '0') && (pObj[i] <= '9') ) {
            // c'est une indirection
            //petit test à la poursuite d'un bug
            if ( wholePdfEncryption && (j < nObj) ) {
               //là on a un pb....
               //cela est rendu necessaire à cause de l'utilisation d'un fichier intermédiaire dans lequel les objets sont réordonnés.
               i = this->xrefTable->getOffsetObjBeforeEncrypt(j);
            }
            else {
                i = this->xrefTable->getOffSetObj(j);
            }
            fseek(this->pf, i, SEEK_SET);
            fread(lBuf, 1, LG_LBUF, this->pf);
            i = 0;
            while ( lBuf[i] != 'j' )
               ++i;
            while ( (lBuf[i] < '0') || (lBuf[i] > '9') )
               ++i;
            streamLg = atoi(lBuf + i);
         }
         else {
            streamLg = j;
         }
      }

      if ( streamLg != 0 ) { //y a t il qqch à chiffrer entre pc + k et streamLg
          //this->encryptBuf((unsigned char *)pObj + streamPos, streamLg, (unsigned char *)key, lgKey);
          org->encryptBuf((unsigned char *)pObj + streamPos, streamLg, (unsigned char *)key, lgKey);
      }
   }

   //il faut explorer pc de 0 à max à la recherche de chaînes à (dé)chiffrer
   i = j = 0;
   tBuf = new char[sizeof_tBuf];
   while ( i < max ) {
      while ( (pObj[i] != '(') && (pObj[i] != '<') && (i < max) )
          ++i;
      if ( i == max )
          break;
      j = i;
      switch ( pObj[i] ) {
      case '(':
          k = 1;
          ++j;
          type = _carType;
          while ( k ) {
             if ( pObj[j] == ')' ) {
                l = 1;
                while ( pObj[j - l] == '\\' )
                   ++l;
                if ( (l % 2) == 1 )
                   --k;
             }
             if ( pObj[j] == '(' ) {
                l = 1;
                while ( pObj[j - l] == '\\' )
                   ++l;
                if ( (l % 2) == 1 )
                   ++k;
             }
             ++j;
          }
          --j;
          break;
      case '<':
          type = _hexaType;
          if ( pObj[j + 1] == '<') {
              j += 1;
              break;
          }
          while ( (pObj[j] != '>') ) {
              ++j;
          }
          break;
      }

      lgStr = j - i - 1;
      m = 0;
      if ( lgStr > 0 ) {

         if ( (nObj == 112) || (nObj == 320) ) {
            lgStr = j - i - 1;
         }

          pdfStr.copy((const char *)pObj + i);
          k = pdfStr.snprint(tBuf, sizeof_tBuf, false);
          if ( k > sizeof_tBuf ) {
             delete tBuf;
             sizeof_tBuf = k + 1;
             tBuf = new char[sizeof_tBuf];
             k = pdfStr.snprint(tBuf, sizeof_tBuf, false);
          }
          //this->encryptBuf((unsigned char *)tBuf, k, (unsigned char *)key, lgKey);
          org->encryptBuf((unsigned char *)tBuf, k, (unsigned char *)key, lgKey);
          
          pdfStr.copy((const char *)tBuf, k, type);
          k = pdfStr.snprint(tBuf, sizeof_tBuf, true);
          if ( k > sizeof_tBuf ) {
              delete tBuf;
              sizeof_tBuf = k + 1;
              tBuf = new char[sizeof_tBuf];
              k = pdfStr.snprint(tBuf, sizeof_tBuf, true);
          }
          if ( k > lgStr ) {
              //il faut modifier le buffer d'origine
              m = k - lgStr;
              if ( m + lgObj > sizeofBuf ) {
                 //on ne peut pas effectuer l'opération car le buffer contenant l'objet n'est pas assez grand
fprintf(stderr, "erreur ici - buffer pas assez grand pour chiffrer\r\n");
                 delete tBuf;
                 return -1;
              }
              for ( l = lgObj; l >= j ; l-- ) {
                  pObj[l + m] = pObj[l];
              }
              lgObj += m;
          }
          memcpy(pObj + i + 1, tBuf, k);
          if ( k < lgStr ) {
              m = lgStr - k;
              for (l = i + k + 1; l <= lgObj; l++) {
                  pObj[l] = pObj[l + m];
              }
              lgObj -= m;
              m *= -1;
          }
      }

      i = j + 1 + m;
   }
   if ( tBuf )
       delete tBuf;


   return lgObj;
}

#undef LG_LBUF

//**************************************************************************************************

int pdfEncrypt::setEncryptRestriction(int i) {
   return this->encryptStandardP = ((i | ENCRYPT_MUST_BE_1) & ~ENCRYPT_MUST_BE_0);
}

int pdfEncrypt::setEncryptKeyLength(int i) {
    if ( (i < 5) || (i > 16) )
        i = 5;
    this->encryptLength = i * 8;
    return this->encryptKeyLength = i;
}

int pdfEncrypt::setMdpO(const char *mdp) {
int i;

   if ( mdp == 0 )
      return 0;

   for (i = 0; (i < ENCRYPT_KEY_LENGTH - 1) && (mdp[i] != 0); i++) {
      this->mdpO[i]  = mdp[i];
   }
   this->mdpO[i] = 0;
   this->lgMdpO = i;

   return i;
}

int pdfEncrypt::setMdpU(const char *mdp) {
int i;

   if ( mdp == 0 )
      return 0;

   for (i = 0; (i < ENCRYPT_KEY_LENGTH - 1) && (mdp[i] != 0); i++) {
      this->mdpU[i]  = mdp[i];
   }
   this->mdpU[i] = 0;
   this->lgMdpU = i;

   this->newMdpU= true;

   return i;
}

int pdfEncrypt::setMdpU(const char *mdp, int lg) {
int i;

   if ( mdp == 0 )
      return 0;

   for (i = 0; (i < ENCRYPT_KEY_LENGTH - 1) && (i < lg); i++) {
      this->mdpU[i]  = mdp[i];
   }
   this->mdpU[i] = 0;
   this->lgMdpU = i;

   this->newMdpU= true;

   return i;
}

//**************************************************************************************************

bool pdfEncrypt::isMdpUserMdp(char *mdp, int lg) {
md5_state_t mst;
int i, j;
char fBuf[LG_FBUF];

    /*if ( !this->status )
       return false;*/

    this->makeEncryptKey(mdp, lg);
    i = this->getEncryptKey(fBuf, LG_FBUF);

    switch ( this->encryptStandardR ) {
    case 2:
    rc4_key key;
        prepare_key((unsigned char *)fBuf, i, &key);
        for ( i = 0; i < 32; i++)
            fBuf[100 + i] = nullStandardKey[i];
        rc4((unsigned char *)(fBuf + 100), 32, &key);
        if ( encryptStandardU.compare(fBuf + 100, 32) == false ) {
            // le fichier a été protégé avec un mot de passe User ..... et merde mais c'est la vie.
            return false;
        }
        return true;
    case 3:
    char keyBuf[16];
    char keyBuf2[16];

        memcpy(keyBuf, this->encryptKey, this->encryptKeyLength);
        md5_init(&mst);
        for (i = 0; i < 32; i++) {
           fBuf[i] = nullStandardKey[i];
        }
        md5_append(&mst, (const unsigned char *)fBuf, 32);
        i = this->xrefTable->getID(0)->snprint((char *)fBuf, LG_FBUF, false);
        md5_append(&mst, (const unsigned char *)fBuf, i);
        md5_finish(&mst, (unsigned char *)fBuf);
        prepare_key((unsigned char *)this->encryptKey, this->encryptKeyLength, &key);
        rc4((unsigned char *)fBuf, 16, &key);
        for (i = 1; i <= 19; i++) {
           for (j= 0; j < this->encryptKeyLength; ++j) {
              keyBuf2[j] = (char)(keyBuf[j] ^ (char)i);
           }
           prepare_key((unsigned char *)keyBuf2, this->encryptKeyLength, &key);
           rc4((unsigned char *)fBuf, 16, &key);
        }
        if ( encryptStandardU.compare(fBuf, 16) == false ) {
           // le fichier a été protégé avec un mot de passe User ..... et merde mais c'est la vie.
           return false;
        }
        return true;
    default:
        return false;
    }
}

/* ********************************************************************************* */
/* Attention, l'utilisation correcte de cette fonction requière la connaissance de mdpU,
   ce qui n'est pas toujours le cas.
*/

bool pdfEncrypt::isMdpOwnerMdp(char *mdp) {
md5_state_t mst;
rc4_key key;
unsigned char *pc;
char fBuf[LG_FBUF];
int i, j, k;
char keyBuf[16];
char keyBuf2[16];

   pc = (unsigned char *)mdp;

   if ( *pc == 0 )
      pc = (unsigned char *)&this->mdpU[0];
   for (i = 0; (i < 32) && (pc[i] != 0); i++ ) {
      fBuf[i] = pc[i];
   }
   for (j = 0; i < 32; i++, j++) {
      fBuf[i] = nullStandardKey[j];
   }
      //step 2 ***************************
   md5_init(&mst);
   md5_append(&mst, (const unsigned char *)fBuf, 32);
   md5_finish(&mst, (unsigned char *)fBuf);
      //step 3 Rev 3 only ***************************
   if ( this->encryptStandardR == 3 ) {
      for (i = 0; i < 50; i++) {
         md5_init(&mst);
         md5_append(&mst, (const unsigned char *)fBuf, 16);
         md5_finish(&mst, (unsigned char *)fBuf);
      }
      memcpy(keyBuf, fBuf, this->encryptKeyLength);
   }
      //step 4 ***************************
   switch ( this->encryptStandardR ) {
   case 2:
       prepare_key((unsigned char *)fBuf, this->encryptKeyLength, &key);
       i = this->encryptStandardO.snprint(fBuf, LG_FBUF, false);
       rc4((unsigned char *)fBuf, i, &key);
       break;
   case 3:
       i = this->encryptStandardO.snprint(fBuf, LG_FBUF, false);
       for (j = 19; j >= 0; --j) {
           for (k=0; k < this->encryptKeyLength; k++) {
               keyBuf2[k] = (char)(keyBuf[k] ^ (char)j);
           }
           prepare_key((unsigned char *)keyBuf2, this->encryptKeyLength, &key);
           rc4((unsigned char *)fBuf, i, &key);
       }
       break;
   }

   return isMdpUserMdp(fBuf, 32);
}

bool pdfEncrypt::isDecryptPossible() {

    if ( !this->newMdpU ) {
        return this->canDecrypt;
    }
    this->newMdpU = false;

    this->canDecrypt = isMdpUserMdp(this->mdpU, this->lgMdpU);

    return this->canDecrypt;
}

char *pdfEncrypt::getEncryptFilter(char *c, int lg) {
   if ( c == NULL )
      return 0;

   _snprintf(c, lg, "%s", this->encryptFilter);

   return c;
}

bool pdfEncrypt::userProtect() {
    if ( /*!this->status || */ !this->xrefTable->getEncryptObj())
       return false;

    if ( this->xrefTable->getEncryptObj() == 0 )
        return false;

    return !this->isMdpUserMdp("", 0);
}

bool pdfEncrypt::ownerProtect() {
    if ( /*!this->status ||*/ !this->xrefTable->getEncryptObj())
       return false;

    if ( this->xrefTable->getEncryptObj() == 0 )
        return false;

    return !this->isMdpOwnerMdp("");
}
