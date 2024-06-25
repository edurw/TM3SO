#include "support.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

/* Manipulate the path to lead com name, extensions and special characters */
char* padding(char *filename) {
    static char output[12]; // Usar static para garantir que a memória permaneça acessível
    char *strptr = filename;
    char *dot = strchr(filename, '.');

    int i;
    for (i = 0; i < 8; i++) {
        if (strptr == dot || *strptr == '\0') {
            break;
        }
        output[i] = toupper(*strptr);
        strptr++;
    }
    while (i < 8) {
        output[i++] = ' ';
    }

    if (dot != NULL) {
        strptr = dot + 1;
        for (i = 8; i < 11; i++) {
            if (*strptr == '\0') {
                break;
            }
            output[i] = toupper(*strptr);
            strptr++;
        }
    }
    while (i < 11) {
        output[i++] = ' ';
    }

    output[11] = '\0';
    return output;
}
