#include "support.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

char* padding(char *filename){
    char* output = malloc(12); // Allocate memory for the formatted name
    if (!output) {
        return NULL; // Return NULL if memory allocation fails
    }

    memset(output, ' ', 11); // Fill with spaces
    output[11] = '\0'; // Null-terminate the string

    char* strptr = filename;
    char* dot = strchr(filename, '.');

    int i;
    for (i = 0; strptr != dot && *strptr != '\0'; strptr++, i++) {
        if (i == 8) {
            break;
        }
        output[i] = toupper(*strptr); // Convert to uppercase and copy
    }

    if (dot) {
        strptr = dot + 1;
        for (i = 8; i < 11 && *strptr != '\0'; strptr++, i++) {
            output[i] = toupper(*strptr); // Convert to uppercase and copy
        }
    }
    return output;
}
