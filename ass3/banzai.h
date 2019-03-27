#ifndef BANZAI
#define BANZAI

#include "error.h"
#include "player.h"
#include "tools.h"
#include "card.h"

void banzai_game(Player**, Desk*, int);

void do_what(Desk*, Player*);

int can_take_token(int[4]);

int is_yellow_can_take(int [4]);

int is_brown_can_take(int [4]);

int is_purple_can_take(int [4]);

int is_red_can_take(int [4]);

int* take_token(int[4]);

int buy_which_card(Cards*, Player*);

int can_buy_a_card(Cards*, Player*);

int* tokens_need(Cards*, Player*);

int cal_sum(int[4]);

int* ordered_worth(Cards*);

int is_need_tokens(Player*);

#endif