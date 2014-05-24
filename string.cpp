#include "mpaMain.hpp"

const char *strVersion = "1.5.1.1";
const char *strMPAURL = "https://mbtpdfasm.codeplex.com/";

/* *********************************************************************************************** */


const char *strHLAuthor = "Author";
const char *strHLKW = "keywords";
const char *strHLSubject = "Subject";
const char *strHLTitle = "Title";


#ifdef LANG_FR

const char *strDebTrait = "debut du traitement de %s...\r\n";

const char *strAjout = "%s ajoute. (%u page(s))\r\n";

const char *strResImp = "impossible d'initialiser %s <--------------\r\n";

const char *strNoMFile = "pas de fichiers correspondant au masque <--------------\r\n";

const char *strErrSignet = "impossible d'intégrer les signets <--------------\r\n";

const char *strErrFile = "erreur avec le fichier %s <--------------\r\n";

const char *strCantOpenScript = "impossible d'ouvrir le fichier de script %s <--------------\r\n";

const char *strPageInserted = "page %u du fichier %s inseree...\r\n";

const char *strCantInsertPage = "impossible d'inserer page %u du fichier %s <--------------\r\n";

const char *strFinAsm = "\nfin de l'assemblage, %u page(s) assemblee(s)\n";

const char *strErrEncrypt = "impossible d'assembler un fichier comportant des restrictions d'acces <-----\r\n";
const char *strErrEncryptVer = "impossible de decrypter ce fichier <-----\r\n";

const char *strHLFileName = "nom du fichier";
const char *strHLNumberOfPages = "nombre de pages";
const char *strHLEFilter = "pilote de chiffrement";
const char *strHLEKeyLength = "longueur de la clé";
const char *strHLUserMdp = "mdp utilisateur";

const char *strUpdateRes = "fichier %s mis à jour\r\n";
const char *strUpdateResErr = "mise a jour de %s impossible <---------\r\n";

/* *********************************************************************************************** */
#elif defined(LANG_EN)

const char *strDebTrait = "begining of insertion of %s...\r\n";

const char *strAjout = "%s inserted (%u page(s)).\r\n";

const char *strResImp = "can't initialize <--------------%s\r\n";

const char *strNoMFile = "no matching file <--------------\r\n";

const char *strErrSignet = "outlines error <--------------\r\n";

const char *strErrFile = "error with file %s <--------------\r\n";

const char *strCantOpenScript = "can't open script file %s <--------------\r\n";

const char *strPageInserted ="page %u of file %s inserted...\r\n";

const char *strCantInsertPage ="can't insert page %u of file %s <--------------\r\n";

const char *strFinAsm = "\nend of process, %u page(s) assembled\n";

const char *strErrEncrypt = "can't insert a file having access restriction <---------\r\n";
const char *strErrEncryptVer = "can't handle encryption specifications <-----\r\n";

const char *strHLFileName = "file name";
const char *strHLNumberOfPages = "number of pages";
const char *strHLEFilter = "security handler";
const char *strHLEKeyLength = "key length";
const char *strHLUserMdp = "user password";

const char *strUpdateRes = "file %s updated\r\n";
const char *strUpdateResErr = "update of %s impossible <---------\r\n";

#endif
