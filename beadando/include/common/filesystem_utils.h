#ifndef FILESYSTEM_UTILS_H
#define FILESYSTEM_UTILS_H

/**
 * Create all directories in `path` (similar to POSIX `mkdir -p`).
 * Supports both forward (`/`) and backslash (`\`) separators on Windows.
 * `path` is expected to be a file path; the function strips the last component
 * (the file name) and creates the directory tree leading to it.
 * Returns 0 on success, -1 on error.
 */
int create_output_directories(const char* file_path);

#endif