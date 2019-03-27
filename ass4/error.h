#include <stdio.h>
#include <stdlib.h>

#ifndef ERROR_ENUM
#define ERROR_ENUM
/*S denote to scores*/
enum ScoresErr {
    S_BAD_ARGS_CNT = 1,
    S_BAD_CNCT = 3,
    S_BAD_SERVER = 4
};


/* P denote to player,
 * OP denote to other player
 * */
enum PlayerErr {
    P_NOTHING = 0,
    P_BAD_ARGS_CNT = 1,/*incorrect number of args*/
    P_BAD_FILE = 2,/*bad key file*/
    P_BAD_NAME = 3,/*bad name*/
    P_BAD_CNCT = 5,/*Failed to connect*/
    P_BAD_AUTH = 6,/*bad auth*/
    P_BAD_ID = 7,/*bad reconnect id*/
    P_BAD_COMM = 8,/*communication error*/
    OP_DISCNCT = 9,/*player ? dissonnected*/
    OP_BAD_MSG = 10/*? sent invalid message*/
};

enum ServerErr {
    BAD_ARGS_CNT = 1,/*incorrect number of args*/
    BAD_KEY_FILE = 2,/*bad keyfile*/
    BAD_DECK_FILE = 3,/*bad deskfile*/
    BAD_STAT_FILE = 4,/*bad statfile*/
    BAD_TIMEOUT = 5,/*bad timeout*/
    BAD_LISTEN = 6,/*failed listen*/
    SYS_ERROR = 10/*system error*/
};
#endif

/*DEC denote to declaration*/
void server_err(int code);

void player_err(int code);

void other_player_err(int code, char letter);

void scores_err(int code);
