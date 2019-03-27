#include "error.h"
#include "tools.h"

#ifndef _GOPHER_H_
#define _GOPHER_H_
typedef struct Score {
    char player[100];
    int tokens;
    int points;
} Score;
typedef struct Message {
    char* text;
    int size;
} Message;

void score(int args, char** argv, FILE* fp[2]);

int auth(const char* msg);

void entry_data(Score* score, Message msg);

void print_score(Score* scoreBoard, int size);

void sort(Score* scoreBoard, int size);

int exist(Score* scoreBoard, Message msg);

void addup(Score* scoreBoard, int position, Message msg);

char** parse_message(Message msg);

void free_parse_message(char** parsed);

void check_args(int args, char** argv);

void build_connection(const char* port, int* conn);

#endif
