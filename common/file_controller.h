#ifndef FILE_CONTROLLER_H
#define FILE_CONTROLLER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EOF_MARKER EOF

int get_abs_path(char *file_path, char **abs_path);

#endif // FILE_CONTROLLER_H