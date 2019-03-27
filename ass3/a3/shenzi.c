#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <game.h>
#include <protocol.h>
#include <token.h>
#include <util.h>

#include "player.h"

char* process_name(void) {
    return "shenzi";
}

/* Objective function used to prioritise cards to purchase. Better cards are
 * those worth the most points.
 */
int card_points(struct Card card, const void* null) {
    assert(null == NULL);
    return card.points;
}

/* Objective function used to prioritise cards to purchase. Better cards are
 * those with the smallest number of tokens to purchase them.
 */
int lowest_card_cost(struct Card card, const void* arg) {
    int* discounts = (int*) arg;
    int totalCost = 0;
    totalCost += max(card.cost[0] - discounts[0], 0); 
    totalCost += max(card.cost[1] - discounts[1], 0); 
    totalCost += max(card.cost[2] - discounts[2], 0); 
    totalCost += max(card.cost[3] - discounts[3], 0); 
    return -1 * totalCost;
}

/* Decides on a card to purchase, and negotiates the purchase of that card with
 * the hub. Assumes that at least one card is able to be purchased.
 */
void purchase_card(const struct Game* game) {
    struct Card purchaseable[BOARD_SIZE];
    struct Player self = game->players[game->selfId];
    int canPurchase = get_purchaseable(game->board, game->boardSize,
            purchaseable, self);

    struct PurchaseMessage msg;
    memset(&msg, 0, sizeof(struct PurchaseMessage));

    // narrow down the list using the first two criteria
    canPurchase =
            find_best_purchases(purchaseable, canPurchase, card_points, NULL);
    canPurchase =
            find_best_purchases(purchaseable, canPurchase, lowest_card_cost,
            self.discounts);

    // most recent card is the one at the end of the array of shortlisted cards
    struct Card purchased = purchaseable[canPurchase - 1];
    buy_card(msg.costSpent, &self, purchased);

    // calculate the ID of the card we want to buy within the original list
    for (int i = game->boardSize - 1; i >= 0; --i) {
        if (memcmp(&game->board[i], &purchased, sizeof(struct Card)) == 0) {
            msg.cardNumber = i;
            break;
        }
    }

    char* printed = print_purchase_message(msg);
    fputs(printed, stdout);
    fflush(stdout);
    free(printed);
}

/* Decides on which tokens to take, and negotiates the taking of those tokens
 * with the hub. Assumes that it is possible to take 3 non-wild tokens.
 */
void take_tokens(const struct Game* game) {
    struct TakeMessage msg;
    memset(&msg, 0, sizeof(struct TakeMessage));

    take_if_possible(msg.tokens, game->tokenCount, TOKEN_PURPLE);
    take_if_possible(msg.tokens, game->tokenCount, TOKEN_BROWN);
    take_if_possible(msg.tokens, game->tokenCount, TOKEN_YELLOW);
    take_if_possible(msg.tokens, game->tokenCount, TOKEN_RED);

    char* printed = print_take_message(msg);
    fputs(printed, stdout);
    fflush(stdout);
    free(printed);
}

void make_move(const struct Game* game) {
    // figure out which cards we can purchase
    struct Card purchaseable[BOARD_SIZE];
    struct Player self = game->players[game->selfId];
    int canPurchase =
            get_purchaseable(game->board, game->boardSize, purchaseable, self);

    if (canPurchase != 0) {
        purchase_card(game);
    } else if (distinct_tokens_available(game->tokenCount, TOKEN_MAX - 1)
            >= TAKE_NUMBER) {
        take_tokens(game);
    } else {
        fputs("wild\n", stdout);
        fflush(stdout);
    }
}
