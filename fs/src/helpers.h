#ifndef HELPERS_H_RMFCDDLE
#define HELPERS_H_RMFCDDLE

#include <stdlib.h>
#include <string.h>
#include <libgen.h> /* basename, dirname */
#include <libnotify/notify.h>

int getCharIndex(const char * str, char target);
int rgetCharIndex(const char * str, char target);
char * getBasename(const char * path);
char * getDirname(const char * path);
void sendNotification(const char * name, const char * text);

#endif
