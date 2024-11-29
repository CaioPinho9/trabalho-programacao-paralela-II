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

int get_part_file_path(char *file_path, char *file_path_with_part)
{
    snprintf(file_path_with_part, sizeof(file_path_with_part), "%s.part", file_path);
    return 0;
}

int handle_write_part_file(char *buffer, int valread, message_t *client)
{
    char file_path_with_part[512];
    get_part_file_path(client->file_path, file_path_with_part);

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

        printf("Renomeando %s to %s...\n", file_path_with_part, client->file_path);
        rename(file_path_with_part, client->file_path);
        printf("Arquivo %s recebido com sucesso\n", client->file_path);
        return 1;
    }
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

int get_size_file(char *file_path, char *size_str)
{
    FILE *file = fopen(file_path, "r");
    if (file == NULL)
    {
        snprintf(size_str, sizeof(size_str), "%d", 0);
        return -1;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    snprintf(size_str, sizeof(size_str), "%ld", size);
    fclose(file);
    printf("Tamanho do arquivo %s: %s bytes\n", file_path, size_str);
    return 0;
}