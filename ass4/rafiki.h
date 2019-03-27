

#ifndef _TOOLBOX_H_
#define _TOOLBOX_H_

#include "error.h"
#include "tools.h"

#endif
#define SIGNAL_NORMAL 0
#define SIGNAL_INT 1
#define SIGNAL_TERM 2

#define PLAYER_COUNT 0
#define GAME_COUNT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/socket.h>
#include <time.h>

#include <arpa/inet.h>
#include <util.h>
#include <server.h>
#include <game.h>
#include <errno.h>
#include <deck.h>
#include <player.h>

#ifndef _RAFIKI_H_
#define _RAFIKI_H_
typedef struct StatFile {
    int port;
    int tokens;
    int points;
    int players;
} StatFile;
typedef struct SocketPool {
    int tokens;
    int points;
    int players;
    int port;
    int sock;
} SocketPool;
typedef struct SigData {
    SocketPool* sp;
    int size;
} SigData;
typedef struct GameToPass {
    struct Game* game;
    int timeout;
    char* deckFileName;
} GameToPass;
typedef struct GameCounter {
    int currentPlayer;
    int gameTurns;
} GameCounter;
typedef struct Middleware {
    char connectionMode[TXT_SIZE];
    char rid[TXT_SIZE];/*reconnection use only*/
    char gameName[TXT_SIZE];
    struct GamePlayer gamePlayer;
    char key[TXT_SIZE];
    pthread_mutex_t mutex;
    pthread_mutex_t resource;
    int spPos;
    SocketPool* sp;
    int fd;
} Middleware;
typedef struct Score {
    char player[100];
    int tokens;
    int points;
} Score;


void swap_stat_to_socket_pool(SocketPool* sp, StatFile* stats, int lines);

void init_middleware(Middleware* m, char* keyFile);

void server(int args, char** argv);

int build_listen(SocketPool* sp);

void args_validation(int args, char** argv);

void msg_handle(char* msg);

int lines_count(const char* fileName);

int read_stat(StatFile** stat, const char* fileName);

int parse_stat_file(char tmp[4][1024], char* input);

int equal(StatFile* stat, int port, int size);

void sig_handler(int signo);

void output_port(const SocketPool* sp, int size);

void handle_play_connection(Middleware* m, struct Game** game,
        SocketPool* sp, int lines, int* gameCount);

void init_game_struct(struct Game* game);

int find_game_index(const struct Game* game,
        const char* gameName, int gameCount);

void resolve_start_new_game(struct Game* game, int gameCounti,
        int timeout, char* deckFileName);

void* pthread_play_new_game(void* args);

void wipe_middleware(Middleware* m);

void* pthread_wait_connection(void* args);

void* pthread_handle_signal(void* args);

void sort_player(struct Game* game);

int handle_reconnection(struct Game** game, int gameCount, Middleware* m);

void display_invalid_message(struct Game game, struct GamePlayer this);

void display_disco_message(struct Game game, struct GamePlayer this);

void read_stat_exit_with_err(FILE* fp, char* output, StatFile* stat);

void pthread_func_init_game_stats(GameToPass* gargs, int gameTurns);

void pthread_func_send_eog(struct Game game);

void pthread_func_main_game_logic(struct Game* game, int timeout);

enum ErrorCode pthread_func_msg_from_player(char* msg,
        struct GamePlayer this, struct Game* game);

int pthread_func_catch_signal(Middleware* m);

int pthread_func_handshake(int fd, Middleware* m, char buffer[TXT_SIZE]);

void pthread_func_setup_new_player(FILE* fp[2], int fd, Middleware* m,
        char buffer[TXT_SIZE]);

void create_new_game_obj(Middleware* m, struct Game** game,
        SocketPool* sp, int lines, int* gameCount);

void realloc_new_game_obj(Middleware* m, struct Game** game,
        SocketPool* sp, int lines, int* gameCount);

void reconnection_send_init_game_state(struct Game* game, FILE* toPlayer);

void handle_score_client(struct Game* game, int gameSize, int fd);

int is_exist_in_scoreBoard(const Score* scoreBoard,
        const char* name, int size);

void display_score_board(Score* scoreBoard, int size, FILE* toScore);

void sort_socre_board(Score* scoreBoard, int size);

int rid_validation(char* buffer);

int check_key(char* buffer, int offset, FILE* fp[2], char* key);

void pthread_func_setup_reconnect_player(FILE* fp[2], int fd, Middleware* m,
        char buffer[TXT_SIZE]);

void server_setup_socketPool(Middleware* m, char* fileName);

int server_handle_sig(Middleware* m, char* fileName, int gameCount,
        struct Game* game);

void sig_catch_free(Middleware m, struct Game* game, int gameCount);

void free_game(struct Game* game);

void check_file_content(FILE* fp);

#endif
