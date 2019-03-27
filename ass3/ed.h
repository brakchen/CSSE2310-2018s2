#ifndef ED
#define ED

#include "error.h"
#include "player.h"
#include "tools.h"
#include "card.h"

void pipe_message_handle(char**);

void print_debug_message(Player* p);

void ed_game(Player**, Desk*, int);

void do_what(Desk*, Player**, char*, int);

int can_take_token(int[4]);

int* take_token(int[4]);

int buy_which_card(int**, Desk*, Player**, int);

int can_buy_a_card(Player*, Cards*);

int* tokens_need(Cards*, Player*);

int cal_sum(int[4]);

void ordered_worth(int**, int**, int);

int is_yellow_can_take(int [4]);

int is_brown_can_take(int [4]);

int is_purple_can_take(int [4]);

int is_red_can_take(int [4]);

int check_repeat(int*, int);

void sort_list(int*, int);

void no_card_can_take(Desk*, int);

int only_one_card_can_take(Desk* desk, Player**, int);

int most_worth_in_list(int**, int);

int multiple_most_worth(int, Desk*, int**, int**, int);

int* take_tokens_to_buy_card(Desk*, Player*, Cards*);

void buy_a_card(int, Player*, Desk*);

int at_least_one_card_can_buy(Desk*, Player**, char*, int);

void fill_data(int*, int, int);

int check_desk_tokens_empty(int [4]);

int can_buy_after_take(Desk*, Player*, Cards*);

#endif
