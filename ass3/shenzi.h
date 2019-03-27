#ifndef SHENZI
#define SHENZI

#include "error.h"
#include "player.h"
#include "tools.h"
#include "card.h"

void shenzi_game(Player**, Desk*, int);

void do_what(Desk*, Player*, char*);

int can_take_token(int[4]);

int* take_token(int[4]);

int buy_which_card(Cards*, Player*);

int can_buy_a_card(Cards*, Player*);

int* tokens_need(Cards*, Player*);

int cal_sum(int[4]);

int* ordered_worth(Cards*);

void discount_reduce(int [4], Player*);

int* find_most_worth(Cards*, Player*);

int* find_cheapest(Cards*, Player*, int*);

#endif

