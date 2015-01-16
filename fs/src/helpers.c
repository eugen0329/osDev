#include "helpers.h"

void sendNotification(const char * name, const char * text)
{
    static char notifyIcon[] = "dialog-information";
    notify_init (name);
    NotifyNotification * notif = notify_notification_new (name, text, notifyIcon);
    notify_notification_show (notif, NULL);
    g_object_unref(G_OBJECT(notif));
    notify_uninit();
}

char * getBasename(const char * path)
{
    int start = rgetCharIndex(path, '/') + 1;
    int len = strlen(path) - start + 1;

    if(len == 0) return NULL;
    char * base = (char *) malloc(len);
    strncpy(base, path + start, len);
    base[len - 1] = '\0';
    return base; 
}

char * getDirname(const char * path)
{
    int len;
    len  = rgetCharIndex(path, '/') + 2;
    //printf("len%d\n", len);
    char * dir = (char *) malloc(strlen(path));
    strcpy(dir, path);
    dirname(dir);
    return dir;     
}

int getCharIndex(const char * str, char target)
{
    const char *ptr = strchr(str, target);
    if(ptr) return ptr - str;
    return -1;
}

int rgetCharIndex(const char * str, char target)
{
    const char *ptr = strrchr(str, target);
    if(ptr) return ptr - str;
    return -1;
}
