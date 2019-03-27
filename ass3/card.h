#ifndef CARD_HEAD
#define CARD_HEAD

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "error.h"
#include "tools.h"

typedef struct Cards {
    char color;
    int No;
    int worth;
    int price[4];
    struct Cards* next;
} Cards;

void read_cards(Cards**, const char*);

int get_card_amount(Cards*);

//Deck draw_card(Deck**);
void handle_comma(Cards**, int [10], int);

void handle_colon(Cards**, int [10], int);

void handle_newline(Cards**, int [10], Cards**);

int get_amount_cards(char*);

char** split_cards(char*);

int is_card_valid(char* card);

void split(int**, char*, int);

void add_element(Cards**, Cards*);

void del_element(Cards**, int);

void insert_element(Cards*, int, Cards*);

void free_list(Cards*);

Cards* find_element(Cards*, int);

void refresh_id(Cards*);

Cards* pop_last_one(Cards*);

Cards* pop_first_one(Cards**);

#endif