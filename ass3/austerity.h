#ifndef AUSTERITY_HEADER
#define AUSTERITY_HEADER

#include "tools.h"
#include "error.h"
#include "card.h"
#include "player.h"

typedef struct Hub {
    Player** players;
    int token[5];
    Cards* deck;
    Cards* board;
    int totalPlayer;
    int endPoint;
    int** childPipes;
    int** parentPipes;
    pid_t* pids;
} Hub;

void check_hub_args(int, char**);

void game(int, char**);

void child(Hub*, int);

void start_game(Hub*);

void send(int [2], char*, int [2]);

char** get(int [2], int*);

int interpret_message(char*, Player*, Hub*);

int interpret_keyword(char*);

void hub_output(int, char, int [6]);

/*below are the function action to player's message*/
void update_hub_player_tokens(Player*, int*, char*);

void update_hub_tokens(Hub*, int*, char*);

void update_hub_player_discount(Player*, char);

void send_tokens_action(Hub*);

void send_new_cards_action(Hub*);

void send_purchased_action(Hub*, char, int, int*);

void send_wild_action(Hub*, char);

int* interpret_take(char*);

int* interpret_purchase(char*);

Player** player_setup(int, char**);

Cards* construct_cards_board(Cards**);

void refill_a_card_and_send_message(Hub*);

int is_someone_reach_win(Hub*);

void hub_output_eog(Hub*);

void bad_start_check(int, char**);

char* get_file_name(char*);

void send_take_action(Hub*, char, int*);

int is_win(Hub*);

#endif
