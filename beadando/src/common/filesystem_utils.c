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

    // Normalize backslashes to forward slashes for consistent handling
    for (char* p = tmp; *p; ++p)
        if (*p == '\\') *p = '/';

    // Extract the directory part (strip the filename)
    char* last_sep = strrchr(tmp, '/');
    if (!last_sep) {
        free(tmp);
        return 0;   // no directory part – nothing to create
    }
    *last_sep = '\0';

    // Walk the path and attempt to create each subdirectory
    char* p = tmp;
    if (*p == '/') p++;   // skip leading slash on POSIX paths
    do {
        char* next = strchr(p, '/');
        if (next) *next = '\0';

        // Create directory – ignore the return value.
        // If it already exists, that's fine. If it truly fails,
        // the subsequent file open will report the error.
        (void)MKDIR(tmp);

        if (next) *next = '/';
        p = next ? next + 1 : NULL;
    } while (p && *p);

    free(tmp);
    return 0;
}