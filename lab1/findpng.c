#include "findpng.h"

// Recursive function to search for PNG files
void find_png_files(const char *directory) {
    DIR *dir = opendir(directory);
    if (dir == NULL) {
        perror("Error opening directory");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            char file_path[256];
            snprintf(file_path, sizeof(file_path), "%s/%s", directory, entry->d_name);

            if (is_png(file_path)) {
                printf("%s\n", file_path);
            }
        } else if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char subdir_path[256];
            snprintf(subdir_path, sizeof(subdir_path), "%s/%s", directory, entry->d_name);
            find_png_files(subdir_path);
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