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
    return "banzai";
}

/* Objective function used to prioritise cards to purchase.  More expensive
 * cards (high number of tokens) are better. Takes as arguments the card
 * itself, and a pointer to self. Returns the number of tokens that would be
 * involved in a purchase of this card.
 */
int most_expensive_card(struct Card card, const void* arg) {
    struct Player self = *(struct Player*) arg;
    int tokens[TOKEN_MAX] = {0};

    // buy the card, and then figure out how many wild tokens were used
    // afterward
    buy_card(tokens, &self, card);

    int output = 0;

    for (int i = 0; i < TOKEN_MAX; ++i) {
        output += tokens[i];
    }

    return output;
}

/* Figure out which card would take the most wild tokens to purchase. This is
 * an objective function, used to figure out which card to purchase. Returns a
 * number indicating how many wild tokens would be necessary to purchase this
 * card. Takes as arguments the card, and a pointer to self.
 */
int most_wilds(struct Card card, const void* arg) {
    struct Player self = *(struct Player*) arg;
    int tokens[TOKEN_MAX] = {0};

    // buy the card, and then figure out how many wild tokens were used
    // afterward
    buy_card(tokens, &self, card);

    return tokens[TOKEN_WILD];
}

/* Decides on which tokens to take, and negotiates the taking of those tokens
 * with the hub. Assumes that it is possible to take 3 non-wild tokens.
 */
void take_tokens(const struct Game* game) {
    struct TakeMessage msg;
    memset(&msg, 0, sizeof(struct TakeMessage));

    take_if_possible(msg.tokens, game->tokenCount, TOKEN_YELLOW);
    take_if_possible(msg.tokens, game->tokenCount, TOKEN_BROWN);
    take_if_possible(msg.tokens, game->tokenCount, TOKEN_PURPLE);
    take_if_possible(msg.tokens, game->tokenCount, TOKEN_RED);

    char* printed = print_take_message(msg);
    fputs(printed, stdout);
    fflush(stdout);
    free(printed);
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
            find_best_purchases(purchaseable, canPurchase,
                    most_expensive_card, self.discounts);
    canPurchase =
            find_best_purchases(purchaseable, canPurchase, most_wilds, &self);

    // oldest card is the one at the end of the array of shortlisted cards
    struct Card purchased = purchaseable[0];
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

void make_move(const struct Game* game) {
    struct Player self = game->players[game->selfId];

    int myTokens = 0;
    for (int i = 0; i < TOKEN_MAX; ++i) {
        myTokens += self.tokens[i];
    }
    int availableTokens =
            distinct_tokens_available(game->tokenCount, TOKEN_MAX - 1);

    struct Card purchaseable[BOARD_SIZE];
    int canPurchase =
            get_purchaseable(game->board, game->boardSize, purchaseable, self);
    int nonZero = 0;
    for (int i = 0; i < canPurchase; ++i) {
        if (purchaseable[i].points > 0) {
            nonZero += 1;
        }
    }

    // figure out if we need to get tokens (and if we can)
    if (myTokens < TAKE_NUMBER && availableTokens >= TAKE_NUMBER) {
        take_tokens(game);
    } else if (nonZero > 0) {
        purchase_card(game);
    } else {
        fputs("wild\n", stdout);
        fflush(stdout);
    }
}
