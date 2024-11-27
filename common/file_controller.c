#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "file_controller.h"

int get_abs_path(char *file_path, char **abs_path)
{
    char *cwd = getcwd(NULL, 0); // Dynamically allocate buffer for current working directory
    if (cwd == NULL)
    {
        perror("getcwd failed");
        return -1;
    }

    // Calculate required buffer size for the full path
    size_t required_size = strlen(cwd) + strlen(file_path) + 2; // +2 for '/' and '\0'

    *abs_path = malloc(required_size);
    if (*abs_path == NULL)
    {
        perror("malloc failed");
        free(cwd);
        return -1;
    }

    // Construct the full path
    snprintf(*abs_path, required_size, "%s/%s", cwd, file_path);

    printf("Full path: %s\n", *abs_path);

    free(cwd); // Free the allocated memory for cwd
    return 0;
}