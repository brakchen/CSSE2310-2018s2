#include "util.h"
#include <stdlib.h>

#define INITIAL_BUFFER_SIZE 80

int read_line(FILE* input, char** output, int offset) {
    int bytesRead = offset, capacity = INITIAL_BUFFER_SIZE, nextByte;

    // resize buffer if needed, to ensure already read bytes can fit
    while (bytesRead + 1 >= capacity) {
        capacity *= 2;
    }

    char* ret = malloc(sizeof(char) * capacity);

    while (nextByte = fgetc(input), nextByte != EOF && nextByte != '\n') {
        ret[bytesRead] = nextByte;
        bytesRead += 1;
        if (bytesRead + 1 >= capacity) {
            capacity *= 2; // double each time, for amortized linear cost
            ret = realloc(ret, capacity);
        }
    }

    if (nextByte == EOF && ferror(input)) {
        // an IO error occured
        *output = ret;
        return -1 * bytesRead;
    } else if (bytesRead == 0 && nextByte == EOF) {
        // reading failed
        free(ret);
        *output = NULL;
        return 0;
    } else {
        // reading succeeded
        ret[bytesRead] = '\0';
        *output = ret;
        return bytesRead;
    }
}

long parse_int(const char* input, char** output) {
    if (*input == ' ') {
        if (output != NULL) {
            *output = (char*) input;
            return 0;
        }
    }

    return strtol(input, output, 10);
}

int max(int val1, int val2) {
    return val1 > val2 ? val1 : val2;
}
