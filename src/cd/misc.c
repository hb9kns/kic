#include <stdio.h>
#include <ctype.h>

/* Some library functions that might not be around. */


char *
to_lower_case(char *string)
{
    char *s = string;

    if (s == NULL) return NULL;
    while (*s) {
        if (isupper(*s)) *s = tolower(*s);
        s++;
    }
    return string;
}


#ifndef NOTUSED
/* library does not have the stricmp(), strnicmp() functions */

int
stricmp(char *s1, char *s2)
{
    char c, d;

    while (*s2) {
        c = *s1;
        d = *s2;
        if (c == '\0') return -1;
        if (isupper(c)) c = tolower(c);
        if (isupper(d)) d = tolower(d);
        if (c != d) return c-d;
        s1++;
        s2++;
    }
    return 0;
}

int
strnicmp(char *s1, char *s2, int n)
{
    char c, d;

    while (n--) {
        c = *s1;
        d = *s2;
        if (c == '\0') return -1;
        if (d == '\0') return 1;
        if (isupper(c)) c = tolower(c);
        if (isupper(d)) d = tolower(d);
        if (c != d) return c-d;
        s1++;
        s2++;
    }
    return 0;
}

#endif
