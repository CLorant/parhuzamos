#include "common/filesystem_utils.h"
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#include <direct.h>
#define MKDIR(p) _mkdir(p)
#else
#include <sys/stat.h>
#define MKDIR(p) mkdir(p, 0755)
#endif

int create_output_directories(const char* file_path)
{
    if (!file_path || !*file_path) return -1;

    char* tmp = (char*)malloc(strlen(file_path) + 1);
    if (!tmp) return -1;
    strcpy(tmp, file_path);

    for (char* p = tmp; *p; ++p)
        if (*p == '\\') *p = '/';

    char* last_sep = strrchr(tmp, '/');
    if (!last_sep) {
        free(tmp);
        return 0;
    }
    *last_sep = '\0';

    char* p = tmp;
    if (*p == '/') p++;
    do {
        char* next = strchr(p, '/');
        if (next) *next = '\0';
        (void)MKDIR(tmp);

        if (next) *next = '/';
        p = next ? next + 1 : NULL;
    } while (p && *p);

    free(tmp);
    return 0;
}