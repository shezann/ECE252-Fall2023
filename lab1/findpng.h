#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "lab_png.h"

/**
 * @brief Find all PNG files under the given directory.
 * 
 * @param directory The directory to find PNG files.
 * @note This function will print content to the standard output.
*/
void find_png_files(const char* directory);
