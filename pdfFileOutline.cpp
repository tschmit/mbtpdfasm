#include "pdfFile.hpp"

const char *outlineFitChars[] = {
    "Fit",
    "FitH",
    "FitV"
};

#define LG_LBUF 250

int C_pdfFile::pOutlinesGrowth(int growth) {
T_outlines *pto;
int i;

    if ( growth == 0 )
       growth = NB_OUTLINES;

    if ( this->pOutlines == 0 ) {
       this->sizeof_pOutlines = growth;
       this->pOutlines = new T_outlines[this->sizeof_pOutlines];
       memset((char *)this->pOutlines, 0, this->sizeof_pOutlines * sizeof(T_outlines));
       for (i=0; i < this->sizeof_pOutlines; i++) {
          pOutlines[i].titre = new pdfString();
       }
       return this->sizeof_pOutlines;
    }

    growth += this->sizeof_pOutlines;

    pto = new T_outlines[growth];
    memset((char *)pto, 0, growth * sizeof(T_outlines));
    memcpy(pto, this->pOutlines, this->sizeof_pOutlines * sizeof(T_outlines));
    for (i = this->sizeof_pOutlines; i < growth; i++ ) {
       pto[i].titre = new pdfString();
    }
    if ( this->pOutlines )
       delete this->pOutlines;
    this->sizeof_pOutlines = growth;
    this->pOutlines = pto;

    return this->sizeof_pOutlines;
}

//* *****************************************************************************************

int C_pdfFile::readOutlines(int idOutline) {
C_pdfFile *org;

   org = this;

   if ( !org->getStatus() || (org->mode != read) )
      return 0;

char *lBuf;
int sizeof_lBuf;
int i, lgObj;
FILE *orgPf;
int pWrite;

   if ( this->nbOutlines >= this->sizeof_pOutlines ) {
      this->pOutlinesGrowth(this->nbOutlines * 2);
   }

   orgPf = org->getFile();

   sizeof_lBuf = LG_LBUF;
   lgObj = -1;
   lBuf = 0;
   i = org->xrefTable->getOffSetObj(idOutline);
   while ( lgObj == -1 ) {
      sizeof_lBuf *= 2;
      if ( lBuf )
         delete lBuf;
      lBuf = new char[sizeof_lBuf];
      fseek(orgPf, i, SEEK_SET);
      fread(lBuf, 1, sizeof_lBuf, orgPf);
      lgObj = findWinBuf("endobj", lBuf, sizeof_lBuf);
   }
   if ( org->xrefTable->getEncryptObj() != 0 ) {
      org->encrypt->encryptObj(lBuf, lgObj + 6, sizeof_lBuf, false);
   }

   pWrite = this->nbOutlines++;
   if ( (i = findWinBuf("/Title", lBuf, lgObj)) != -1 ) {
      i += 6;
      while ( (lBuf[i] != '(') && (i < sizeof_lBuf) )
         ++i;
      if ( lBuf[i] == '(' ) {
      pdfString title(lBuf + i);

         pOutlines[pWrite].titre->copy(&title);
      }
   }

   if ( (i = findWinBuf("/Parent", lBuf, lgObj)) != -1 ) {
      pOutlines[pWrite].parent = atoi(lBuf + i + 8);
   }
   if ( (i = findWinBuf("/Prev", lBuf, lgObj)) != -1 ) {
      pOutlines[pWrite].prev = atoi(lBuf + i + 6);
   }
   if ( (i = findWinBuf("/Next", lBuf, lgObj)) != -1 ) {
      pOutlines[pWrite].next = atoi(lBuf + i + 6);
   }
   if ( (i = findWinBuf("/First", lBuf, lgObj)) != -1 ) {
      pOutlines[pWrite].first = atoi(lBuf + i + 7);
   }
   if ( (i = findWinBuf("/Last", lBuf, lgObj)) != -1 ) {
      pOutlines[pWrite].last = atoi(lBuf + i + 6);
   }
   if ( (i = findWinBuf("/Count", lBuf, lgObj)) != -1 ) {
      pOutlines[pWrite].count = atoi(lBuf + i + 7);
   }
   if ( (i = findWinBuf("/Dest", lBuf, lgObj)) != -1 ) {
   int li;

      li = i;
      while ( (lBuf[li] != 'R') && (li < lgObj) ) {
         ++li;
      }
      --li;
      li -= rewindRef(lBuf + li);
      pOutlines[pWrite].dest = atoi(lBuf + li);
   }
   else {
      if ( (i = findWinBuf("/A", lBuf, lgObj)) != -1 ) {
         i += 2;
         while ( lBuf[i] == ' ' )
            ++i;
		 char *pc; C_pdfObject *action; strMatcher *regExp;
		 switch ( lBuf[i] ) {
			 //indirection vers un dictionnaire (action)
		 case '0': case '1': case '2': case '3': case '4': case '5':
		 case '6': case '7': case '8': case '9':
            i = atoi(lBuf + i);
			pOutlines[pWrite].a = i;
			this->getObj(i, lBuf, &sizeof_lBuf, &i, (C_pdfObject **)0, false);
			action = new C_pdfObject(lBuf, sizeof_lBuf, actionKeyWords, false);
			pc = (char *)action->getAttrib(_D);
			if ( pc && f_X_X_R.doesStrMatchMask((const char *)pc) ) {
				pOutlines[pWrite].dest = atoi(pc + f_X_X_R.getOffsetOfMatchingStr());
			}
			else {
				pc = (char *)action->getAttrib(_JS);
				if ( pc ) {
                    regExp = new strMatcher("pageNum *= *([0-9]*)", true, 0);
					if ( regExp->doesStrMatchMask(pc) ) {
                        pOutlines[pWrite].dest = this->getPageAsObjNumber(atoi(pc + regExp->getOffsetOfCaptured(1)) + 1);
					}
					delete regExp;
				}
			}
			delete action;
			break;
			//dictionnaire direct
		 case '<':
			 if ( f_X_X_R.doesStrMatchMask((const char *)(lBuf + i)) ) {
				pOutlines[pWrite].dest = atoi(lBuf + i + f_X_X_R.getOffsetOfMatchingStr());
			}
			 break;
		 default:
            fprintf(stdout, "the destination is an action... sorry I can't manage that (yet :-) )\r\n");
		 }
      }
   }

   pOutlines[pWrite].nObj = idOutline;

   if ( pOutlines[pWrite].first != 0 ) {
      readOutlines(pOutlines[pWrite].first);
   }

   if ( pOutlines[pWrite].next != 0 ) {
      readOutlines(pOutlines[pWrite].next);
   }

   if ( lBuf )
      delete lBuf;

   return this->nbOutlines;
}
#undef LG_LBUF

//* *****************************************************************************************
// org: fichier pdf dans lequel on doit lire les signets

bool C_pdfFile::loadOutlines(C_pdfFile *org) {

   if ( org->nbOutlines == 0 ) {
      return true;
   }
   
   if ( this->sizeof_pOutlines < (this->nbOutlines + org->nbOutlines) )  {
      this->pOutlinesGrowth(this->nbOutlines + org->nbOutlines + 1);
   }

   if ( this->outlines == 0 ) {
      this->outlines = this->xrefTable->addObj(0, 0, 'n') - 1;
   }

int i;
   for ( i = this->nbOutlines; i < this->nbOutlines + org->nbOutlines; i++) {
      org->xrefTable->setInserted(org->pOutlines[i - this->nbOutlines].nObj, this->xrefTable->addObj(0, 0, 'n') - 1);
   }

   for (i = this->nbOutlines; i < this->nbOutlines + org->nbOutlines; i++) {
      this->pOutlines[i].nObj = org->xrefTable->getInserted(org->pOutlines[i - this->nbOutlines].nObj);
      this->pOutlines[i].titre->copy(org->pOutlines[i - this->nbOutlines].titre);
      if ( org->pOutlines[i - this->nbOutlines].parent == org->outlines ) {
         this->pOutlines[i - this->nbOutlines].parent = this->outlines;
         if ( this->firstOutlines == 0 ) {
            this->firstOutlines = this->pOutlines[i].nObj;
            this->lastOutlines  = this->pOutlines[i].nObj;
         }
         else {
            for (int j = 0; j < this->nbOutlines + org->nbOutlines; j++) {
               if ( this->pOutlines[j].nObj == this->lastOutlines ) {
                  this->pOutlines[j].next = this->pOutlines[i].nObj;
                  this->pOutlines[i].prev = this->pOutlines[j].nObj;
                  break;
               }
            }
            this->lastOutlines = this->pOutlines[i].nObj;
         }
         if ( org->pOutlines[i - this->nbOutlines].first )
            this->pOutlines[i].first = org->xrefTable->getInserted(org->pOutlines[i - this->nbOutlines].first);
         if ( org->pOutlines[i - this->nbOutlines].last )
            this->pOutlines[i].last = org->xrefTable->getInserted(org->pOutlines[i - this->nbOutlines].last);
      }
      else {
         if ( org->pOutlines[i - this->nbOutlines].parent )
            this->pOutlines[i].parent = org->xrefTable->getInserted(org->pOutlines[i - this->nbOutlines].parent);
         if ( org->pOutlines[i - this->nbOutlines].next )
            this->pOutlines[i].next = org->xrefTable->getInserted(org->pOutlines[i - this->nbOutlines].next);
         if ( org->pOutlines[i - this->nbOutlines].prev )
            this->pOutlines[i].prev = org->xrefTable->getInserted(org->pOutlines[i - this->nbOutlines].prev);
         if ( org->pOutlines[i - this->nbOutlines].first )
            this->pOutlines[i].first = org->xrefTable->getInserted(org->pOutlines[i - this->nbOutlines].first);
         if ( org->pOutlines[i - this->nbOutlines].last )
            this->pOutlines[i].last = org->xrefTable->getInserted(org->pOutlines[i - this->nbOutlines].last);
      }
      if ( org->pOutlines[i - this->nbOutlines].dest != 0 ) {
         this->pOutlines[i].dest = org->xrefTable->getInserted(org->pOutlines[i - this->nbOutlines].dest);
      }
      else {
         if ( org->pOutlines[i - this->nbOutlines].a != 0 )
            this->pOutlines[i].a = insertObj(org, org->pOutlines[i - this->nbOutlines].a);
      }
      this->pOutlines[i].count = org->pOutlines[i - this->nbOutlines].count;
   }

   this->nbOutlines = i;
   outlinesLoaded = true;

   return true;
}

//* *****************************************************************************************

//fileName: fichier de définition des signets
bool C_pdfFile::loadOutlines(char *fileName) {
FILE *org;
char *fBuf, *pc;
int sizeof_fBuf, sf, i, j, k, l, val, memNbOutlines;
int *maxChild;
int maxOut; // id max pour un outline
int fit;

   fit = OUTLINE_FIT;

   if ( mode != write )
      return false;

   if ( *fileName == 0 )
      return false;

   if ( (org = fopen(fileName, "rb")) == 0 )
      return false;

   maxOut = 0;

   if ( this->pOutlines == 0 ) {
      this->pOutlinesGrowth(0);
   }

   // chargement du fichier de description en mémoire
   fseek(org, 0, SEEK_END);
   sizeof_fBuf = (sf = ftell(org)) + 1;
   fBuf = new char[sizeof_fBuf];
   fseek(org, 0, SEEK_SET);
   fread(fBuf, 1, sizeof_fBuf, org);
   fBuf[sizeof_fBuf - 1] = 0;

   pc = fBuf;
   i = this->nbOutlines;
   memNbOutlines = i;
   if ( this->outlines == 0 )
      this->outlines = this->xrefTable->addObj(0, 0, 'n') - 1;
   while ((unsigned char)*pc < ' ' )
      pc++;
   while ( (pc - fBuf) < (sf - 9) ) { //9 car taille minimum d'une ligne valide pour le fichier de définition des outlines
      if ( *pc == '/') {
          ++pc;
          if ( (*pc == 'F') && (*(pc + 1) == 'i') && (*(pc + 2) == 't') ) {
              pc += 3;
              switch ( *pc ) {
                 case 'H':
                    fit = 1;
                    break;
                 case 'V':
                    fit = 2;
                    break;
                 default:
                    fit = 0;
              }
          }
          while ((unsigned char)*pc >= ' ' )
              pc++;
      }
      else {
          ++this->nbOutlines;
          if ( this->nbOutlines == this->sizeof_pOutlines ) {
             this->pOutlinesGrowth(0);
          }
          this->pOutlines[i].id = atoi(pc);
          if ( pOutlines[i].id > maxOut )
             maxOut = pOutlines[i].id;
          while ( *(pc++) != ' ' );
          pOutlines[i].idP = atoi(pc);
          while ( *(pc++) != ' ' );
          pOutlines[i].rg = atoi(pc);
          while ( *(pc++) != ' ' );
          val = atoi(pc);
          if ( val <= nbPages )
             pOutlines[i].dest = childs[val - 1];
          else
             pOutlines[i].dest = 0;
          pOutlines[i].nObj = xrefTable->addObj(0, 0, 'n') - 1;
          while ( *(pc++) != ' ');
          j = 0;
          while ( ((unsigned char)pc[j] >= ' ') && (j < (LG_TITRE_OUTLINE - 1)) )
             ++j;
          pOutlines[i].titre->copy(pc, j, _carType);
          pOutlines[i].fit = fit;
          i++;
          pc += j;
      }
      while ((unsigned char)*pc < ' ' )
         pc++;
   }
   // recherche du nombre maximum d'enfant par parent
   maxChild = new int[maxOut + 1];
   memset(maxChild, 0, (maxOut + 1) * sizeof(int));
   for (i = memNbOutlines; i < this->nbOutlines; i++) {
      if ( maxChild[this->pOutlines[i].idP] < this->pOutlines[i].rg )
         maxChild[this->pOutlines[i].idP] = this->pOutlines[i].rg;
   }
   countOutlines = maxChild[0];
   for (i = memNbOutlines; i < this->nbOutlines; i++) {
      this->pOutlines[i].count = -maxChild[this->pOutlines[i].id];
      k = pOutlines[i].idP;
      l = pOutlines[i].rg;
      if ( k == 0 ) { // cas de la racine des outlines
         this->pOutlines[i].parent = this->outlines;
         if ( l == 1 ) {
            if ( this->firstOutlines == 0 ) {
               this->firstOutlines = this->pOutlines[i].nObj;
            }
         }
         else {
            if ( l == maxChild[0] )
               this->lastOutlines = this->pOutlines[i].nObj;
         }
      }
      else { // cas général
         // **** renseignement de parent *****
         for (j= memNbOutlines; j < this->nbOutlines; j++) {
            if ( this->pOutlines[j].id == this->pOutlines[i].idP )
               break;
         }
         this->pOutlines[i].parent = this->pOutlines[j].nObj;
      }
      // ***** renseignement de prev *****
      if ( l == 1 ) {
         if ( k == 0 ) {
            for (j = 0; j < this->nbOutlines; j++) {
               if ( this->pOutlines[j].nObj == this->lastOutlines ) {
                  this->pOutlines[j].next = this->pOutlines[i].nObj;
                  break;
               }
            }
            this->pOutlines[i].prev = this->lastOutlines;
            this->lastOutlines = this->pOutlines[i].nObj;
         }
         else
            this->pOutlines[i].prev = 0;
      }
      else {
         for (j = 0; j < this->nbOutlines; j++ ) {
            if ( (this->pOutlines[j].idP == this->pOutlines[i].idP) && (this->pOutlines[j].rg == (l - 1)) )
               break;
         }
         this->pOutlines[i].prev = this->pOutlines[j].nObj;
      }
      // ***** renseignement de next ****
      if ( l == maxChild[this->pOutlines[i].idP] )
         this->pOutlines[i].next = 0;
      else {
         for (j = memNbOutlines; j < this->nbOutlines; j++ ) {
            if ( (this->pOutlines[j].idP == this->pOutlines[i].idP) && (this->pOutlines[j].rg == (l + 1)) )
               break;
         }
         this->pOutlines[i].next = this->pOutlines[j].nObj;
      }
      this->pOutlines[i].first = 0;
      this->pOutlines[i].last = 0;
      if ( maxChild[this->pOutlines[i].id] > 0 ) {
         // ***** renseignement de first *****
         for (j = memNbOutlines; j < this->nbOutlines; j++ ) {
            if ( (this->pOutlines[j].idP == this->pOutlines[i].id) && (this->pOutlines[j].rg == 1) ) {
               this->pOutlines[i].first = this->pOutlines[j].nObj;
               break;
            }
         }
         // ***** renseignement de last *****
         for (j = memNbOutlines; j < this->nbOutlines; j++ ) {
            if ( (this->pOutlines[j].idP == this->pOutlines[i].id) && (this->pOutlines[j].rg == maxChild[this->pOutlines[i].id]) ) {
               this->pOutlines[i].last = this->pOutlines[j].nObj;
               break;
            }
         }
      }
   }

   delete maxChild;
   delete fBuf;
   fclose(org);

   this->outlinesLoaded = true;

   return true;
}

//* *****************************************************************************************

#define LG_LBUF 1000
bool C_pdfFile::flushOutlines() {
int i, j;
char lBuf[LG_LBUF];

   if ( !(mode == write) || !outlinesLoaded || closed )
      return false;

   xrefTable->setOffset(outlines, ftell(pf));
   fprintf(pf, "%u 0 obj\r\n<<\r\n/Type /Outlines\r\n/Count %u\r\n/First %u 0 R\r\n/Last %u 0 R\r\n>>\r\nendobj\r\n", outlines, countOutlines, firstOutlines, lastOutlines);

   sprintf(lBuf, "/Title (");

   for (i = 0; i < nbOutlines; i++) {
      xrefTable->setOffset(pOutlines[i].nObj, ftell(pf));
      fprintf(pf, "%u 0 obj\r\n<<\r\n", pOutlines[i].nObj);
      j = pOutlines[i].titre->snprint(lBuf + 8, LG_LBUF, true);
      lBuf[8 + j] = ')';
      lBuf[9 + j] = '\r';
      lBuf[10 + j] = '\n';
      fwrite(lBuf, 1, 10 + j + 1, pf);
      fprintf(pf, "/Parent %u 0 R\r\n", pOutlines[i].parent);
      if ( pOutlines[i].count )
         fprintf(pf, "/Count %i\r\n", pOutlines[i].count);
      if ( pOutlines[i].prev )
         fprintf(pf, "/Prev %u 0 R\r\n", pOutlines[i].prev);
      if ( pOutlines[i].next )
         fprintf(pf, "/Next %u 0 R\r\n", pOutlines[i].next);
      if ( pOutlines[i].first )
         fprintf(pf, "/First %u 0 R\r\n", pOutlines[i].first);
      if ( pOutlines[i].last )
         fprintf(pf, "/Last %u 0 R\r\n", pOutlines[i].last);
      if ( pOutlines[i].dest )
         fprintf(pf, "/Dest [%u 0 R /%s]\r\n", pOutlines[i].dest, outlineFitChars[pOutlines[i].fit]);
      if ( pOutlines[i].a )
         fprintf(pf, "/A %u 0 R\r\n", pOutlines[i].a);
      fprintf(pf, ">>\r\nendobj\r\n");
   }

   outlinesFlushed = true;
   return true;
}
#undef LG_LBUF


FILE *C_pdfFile::flushFormatedOutlinesToDest(int format, FILE *dest) {
int i, j;

    for (i = 0; i < nbOutlines; i++) {
		this->pOutlines[i].rg = 1;
		this->pOutlines[i].idP = 0;
        
			for (j = 0; j < this->nbOutlines; j++) {
				if ( this->pOutlines[j].nObj == this->pOutlines[i].parent)
					this->pOutlines[i].idP = j + 1;
				if ( this->pOutlines[j].next == this->pOutlines[i].nObj)
					this->pOutlines[i].rg = this->pOutlines[j].rg + 1;
			}

		fprintf(dest, "%i %i %i %i ",i + 1, this->pOutlines[i].idP, this->pOutlines[i].rg, this->getObjAsPageNumber(this->pOutlines[i].dest));
		this->pOutlines[i].titre->echo(dest);
		fprintf(dest, "\r\n");
	}
	return dest;
}
