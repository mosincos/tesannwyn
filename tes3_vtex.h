#ifndef TES3_IMPORT_H
#define TES3_IMPORT_H

int GetFormIDFromTEXNum(unsigned short int, char *);
int GetFormIDForFilename(char *, char *, char *, char *);
int ReadLTEX3(char *);
int StringToReverseFormID(char *, char *);

#endif /* TES3_IMPORT_H */