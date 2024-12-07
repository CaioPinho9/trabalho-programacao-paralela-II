#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "file_controller.h"

int get_abs_path(char *file_path, char **abs_path, int verbose)
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

    verbose_printf(verbose, "Full path: %s\n", *abs_path);

    free(cwd); // Free the allocated memory for cwd
    return 0;
}

int get_part_file_path(char *file_path, char **file_path_with_part)
{
    size_t required_size = strlen(file_path) + strlen(".part") + 1; // +1 for '\0'

    *file_path_with_part = malloc(required_size);
    if (*file_path_with_part == NULL)
    {
        perror("malloc failed");
        free(file_path_with_part);
        return -1;
    }
    snprintf(*file_path_with_part, required_size, "%s.part", file_path);
    return 0;
}

int handle_write_part_file(char *buffer, int valread, message_t *message, int verbose)
{
    char *file_path_with_part;
    get_part_file_path(message->file_path, &file_path_with_part);

    FILE *file = fopen(file_path_with_part, "a");
    if (file == NULL)
    {
        perror(INVALID_FILE_PATH_EXCEPTION);
        return -1;
    }

    fprintf(file, "%s", buffer);
    fclose(file);

    // Check if the last char of buffer is end of file rename the file removing .part
    if (buffer[valread - 1] == EOF_MARKER)
    {
        // Remove EOF from the file
        FILE *file = fopen(file_path_with_part, "r+");
        fseek(file, -1, SEEK_END);
        ftruncate(fileno(file), ftell(file));
        fclose(file);

        verbose_printf(verbose, "Renaming %s to %s...\n", file_path_with_part, message->file_path);
        rename(file_path_with_part, message->file_path);
        printf("File %s received successfully\n", message->file_path);
        free(file_path_with_part);
        return 1;
    }
    free(file_path_with_part);
    return 0;
}

int file_exists(char *file_path)
{
    FILE *file = fopen(file_path, "r");
    if (file == NULL)
    {
        return 0;
    }
    fclose(file);
    return 1;
}

long get_size_file(char *file_path, int verbose)
{
    FILE *file = fopen(file_path, "r");
    if (file == NULL)
    {
        return 0;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fclose(file);
    verbose_printf(verbose, "File %s: %ld bytes\n", file_path, size);
    return size;
}
