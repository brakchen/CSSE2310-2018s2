

#ifndef ERROR_ENUM
#define ERROR_ENUM

#include <stdio.h>
#include <stdlib.h>

typedef enum {
    OK = 0,                 //no error
    ARGS_COUNT = 1,         //argument count incorrect
    BAD_PLAYER_COUNT = 2,       //invalid player count
    BAD_ID = 3,             //invalid player ID
    BAD_PIPE = 4,           /*pipe recive invalid message or 
                            end game eailer*/
} ErrorPlayer;

typedef enum {
    BAD_ARGS_COUNT = 1,     //argument count incorrct
    BAD_ARGS = 2,           //incalid input argument
    BAD_FILE = 3,           //incorrect file
    BAD_CONTENT = 4,        //incorrect file content
    BAD_START = 5,          //bad start
    BAD_CONNECT = 6,        //client disconnected
    BAD_PROTOCOL = 7,       //portocol error byclient
    BAD_SIGINT = 10,        //SIGINT caught

} ErrorHub;

void player_error(ErrorPlayer, char*);

void hub_error(ErrorHub);

#endif
