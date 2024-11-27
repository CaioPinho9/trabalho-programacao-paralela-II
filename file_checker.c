#include <stdio.h>
#include <stdlib.h>

void printDifference(int c1, int c2, int position) {
    printf("Difference at position %d:\n", position);
    printf("File1: %s\n", c1 == EOF ? "EOF" : (char[]){(char)c1, '\0'});
    printf("File2: %s\n", c2 == EOF ? "EOF" : (char[]){(char)c2, '\0'});
}

int compareFiles(const char *file1, const char *file2) {
    FILE *fp1 = fopen(file1, "r");
    FILE *fp2 = fopen(file2, "r");
    int ch1, ch2;
    int position = 0;
    int foundDifference = 0;

    if (fp1 == NULL || fp2 == NULL) {
        perror("Error opening file");
        return -1;
    }

    printf("Comparing files...\n");

    do {
        ch1 = fgetc(fp1);
        ch2 = fgetc(fp2);
        position++;

        // If characters differ, print the position and values
        if (ch1 != ch2) {
            printDifference(ch1, ch2, position);
            foundDifference = 1;
        }

    } while (ch1 != EOF && ch2 != EOF);

    // Check if both files reached EOF
    if (ch1 == EOF && ch2 == EOF && !foundDifference) {
        printf("Files are the same.\n");
    } else if (!foundDifference) {
        // Files differ in length
        printDifference(ch1, ch2, position);
    }

    fclose(fp1);
    fclose(fp2);
    return foundDifference ? 0 : 1;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <file1> <file2>\n", argv[0]);
        return 1;
    }

    int result = compareFiles(argv[1], argv[2]);

    if (result == -1) {
        printf("Error opening files.\n");
    }

    return 0;
}
