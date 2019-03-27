#ifndef _TOOLBOX_H_
#define _TOOLBOX_H_

#include "error.h"
#include "tools.h"

#endif

#include <player.h>
#include <protocol.h>

#ifndef _ZAZU_H_
#define _ZAZU_H_
typedef struct Reconnection {
    char* gameName;
    int gameCounter;
    int playerId;

} Reconnection;
enum ServerMsg {
    S_YES = 0,
    S_NO = 1,
    S_RID = 2,
    S_PLAYINFO = 3,
    S_PLAYER = 4,
    S_DISCO = 5,
    S_INVALID = 6
};
typedef struct PlayerMessage {
    char letter;
    int playerCount;
    int points;
    int discount[TOKEN_MAX - 1];
    int tokens[TOKEN_MAX];
} PlayerMessage;

void free_game_state(struct GameState* gstate);

void game(int args, char** argv);

void build_connection(const char* port, int* conn);

enum PlayerErr auth(FILE* fp[2], char** argv);

void check_args(int args, char** argv);

int first_comma_pos(const char* value);

enum ErrorCode process_hub_msg(const char* msg,
        struct GameState* gstate, FILE* fp[2]);

int handle_play_info(struct GameState* gstate, char* msg);

int handle_player_reconnect(struct GameState* gstate, char* msg);

int parse_rid(const char* msg, Reconnection* output);

int parse_play_info(const char* msg, int output[2]);

void player_info_validation(const char* msg, struct GameState* gstate);

int parse_reconnect_info(const char* msg, PlayerMessage* pMsg);

void output_game_state(struct GameState gstate);

void free_game_state_err(struct GameState* gstate);

char* prompt(struct GameState* gstate);

void talkok(char buffer[BUF_SIZE], char* action);

void reconnect_mode(char** argv, FILE* fp[2]);

void normal_play_mode(int args, char** argv, FILE* fp[2]);

void read_key_file(char* output, char* fileName);


void prompt_purchase_dialog(struct PurchaseMessage* purchaseMsg,
        const struct GameState gstate);

void exit_with_free(enum PlayerErr err, struct GameState* gstate, FILE* fp[2]);

void handshake(char* msg, struct GameState* gstate, FILE* fp[2]);

void start_game_logic(char* msg, struct GameState* gstate, FILE* fp[2]);

void init_game_state(struct GameState* gstate, char** argv);

void player_name_check(char* name);

void prompt_purchase_token(struct PurchaseMessage* purchaseMsg,
        const struct GameState gstate);

#endif
