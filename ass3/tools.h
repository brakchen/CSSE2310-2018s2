#ifndef TOOLS
#define TOOLS

#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdarg.h>
#include "error.h"
#include "card.h"

#define MAX_PLAYER 26
#define PLAYER_MAX_ARGS_COUNT 3
#define READ 0
#define WRITE 1
#define STDIN_FD 0
#define STDOUT_FD 1
#define STDERR_FD 2
#define BUFFER_SIZE 1024
typedef enum HubMessage {
    HUB_EOG = 1,
    HUB_DOWHAT = 2,
    HUB_TOKENS = 3,
    HUB_NEWCARD = 4,
    HUB_PURCHASED = 5,
    HUB_TOOK = 6,
    HUB_WILD = 7,

} HubMessage;
typedef enum PlayerMessage {
    PLAYER_PURCHASE = 1,
    PLAYER_TAKE = 2,
    PLAYER_WILD = 3,
} PlayerMessage;



int is_number(const char);

int is_capital_letter(const char);

int is_lowercase_letter(const char);

int is_letter(const char);

int is_all_numbers(char*);


int is_comma_or_colon(char);

int is_PBYR(char);

int is_comma(char);

int is_colon(char);

int* count_comma_and_colon(char* str);
#endif