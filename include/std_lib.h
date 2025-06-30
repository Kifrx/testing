#ifndef __STD_LIB_H__
#define __STD_LIB_H__

#include "std_type.h"

int div(int a, int b);
int mod(int a, int b);

void memcpy(byte* dst, byte* src, unsigned int size);
unsigned int strlen(char* str);
bool strcmp(char* str1, char* str2);
void strcpy(char* dst, char* src);
void clear(byte* buf, unsigned int size);

/**
 * TODO: Add your general helper function here
 */
void printChar(char c);
void printString(char* str);
void strncpy(char* dst, char* src, unsigned int n);
void memset(byte* buf, byte val, unsigned int size);



#endif // __STD_LIB_H__
