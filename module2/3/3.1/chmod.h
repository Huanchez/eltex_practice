#ifndef CHMOD_H
#define CHMOD_H

#include <sys/stat.h>

#define ADD         0x001
#define SET         0x002
#define DELETE      0x003

#define USER        0x1C0
#define GROUP       0x038
#define OTHER       0x007
#define ALL         0x1FF

#define READ        0x124
#define WRITE       0x092
#define EXEC        0x049
#define NO          0x000

#define ERROR       0xFFF

void letterToBit(char* c, unsigned int* permission);
void numericToBit(char* c, unsigned int* permission);
void letterPermissions(unsigned int mode, char *permission);
void bitPermissions(unsigned int mode, unsigned int *permissions);
void numericPermissions(unsigned int mode, unsigned int *permission);

unsigned int badchmod(char *c, const char *filename);
unsigned int letterchmod(char *c, unsigned int mode);
unsigned int numericchmod(char* c, unsigned int mode);
unsigned int matchPattern(char *input, const char *pattern);

#endif 