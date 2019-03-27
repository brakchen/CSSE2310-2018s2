#ifndef PLAYER
#define PLAYER

#include "tools.h"
#include "card.h"

typedef struct Player {
    int ID;
    int discount[4];
    int tokens[5];
    int totalPoints;
    char indicator;
    char* name;
    int fd[2];
} Player;

typedef struct Desk {
    Cards* board;
    int* tokens;
    int totalPlayer;

} Desk;

/*message parse function*/
int parsing_message(char*);

int parsing_keyword(char*);

int* get_tokens(char*);

int* get_newcard(char*);

int* get_purchased(char*);

int* get_took(char*);

int* get_wild(char*);

/*game init constructer*/
Player** get_player_objects(int);

void get_cards_board();

Desk* get_desk_object(int);

/*update info's help function*/
void update_player_info(Player**, Desk*, char*);

void update_desk_info(Desk*, char*);

/*update player's discount*/
void update_player_tokens_purchase(Player*, int*);

void update_player_discount(Player*, char);

/*update tokens*/
void update_desk_tokens_purchased(Desk*, int*);

void update_desk_tokens(Desk*, int*, char*);

void update_player_tokens(Player*, int*, char*);

/*help function*/
int get_id(char);

void print_chain(Cards*, char);


void add_card_to_board(Cards**, int*);

char** get_read(int*);

void do_write(char*);

Cards* copy(Cards* card);


char* construct_purchase_message(Cards*, int*);

char* construct_take_message(int*);

char* construct_wild_message();

/*output message*/
void output_board_status(Desk*);

void output_player_status(Player**, int, int);

void check_player_args(int, char**, char*);

void message_format_check(char*);

void output_eog(Desk*, Player**, int);

void checker(char*);

#endif
