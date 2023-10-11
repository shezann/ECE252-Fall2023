#include "findpng.h"

// Recursive function to search for PNG files
void find_png_files(const char *directory) {
    DIR *dir = opendir(directory);
    if (dir == NULL) {
        perror("Error opening directory.\n");
        return;
    }
    
    struct dirent *entry;
    struct stat file_stat;
    char sub_directory[2048];
    while ((entry = readdir(dir)) != NULL) {
        if (directory[strlen(directory) - 1] == '/') { // Remove the extra '/'
            snprintf(sub_directory, sizeof(sub_directory), "%s%s", directory, entry->d_name);
        } else {
            snprintf(sub_directory, sizeof(sub_directory), "%s/%s", directory, entry->d_name);
        }

        if (stat(sub_directory, &file_stat) == -1) {
            perror("Error getting the file stat.\n");
            continue;
        }

        if (S_ISREG(file_stat.st_mode) && is_png(sub_directory)) { // If is a file and is png file
            printf("%s\n", sub_directory);
        } else if (S_ISDIR(file_stat.st_mode) && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            // If it is a dir, call the function recursively
            find_png_files(sub_directory);
        }
    }

    closedir(dir);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <directory name>\n", argv[0]);
        return 1;
    }

    const char *directory = argv[1];
    find_png_files(directory);

    return 0;
}
