#ifndef FILE_CONTROLLER_H
#define FILE_CONTROLLER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../common/socket/socket.h"

#define EOF_MARKER EOF
#define FILE_EXISTS_EXCEPTION "File already exists. [0000]\n"
#define FILE_NOT_FOUND_EXCEPTION "File not found. [0001]\n"
#define INVALID_FILE_PATH_EXCEPTION "Invalid file path. [0002]\n"

int get_abs_path(char *file_path, char **abs_path);

int get_part_file_path(char *file_path, char **file_path_with_part);

int handle_write_part_file(char *buffer, int valread, message_t *client);

int get_size_file(char *file_path, char *size_str);

int file_exists(char *file_path);

#endif // FILE_CONTROLLER_H