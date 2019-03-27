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
    return "ed";
}

/* Objective function used to prioritise cards to purchase. Better cards are
 * those worth the most points. Takes as arguments the card, and a null pointer
 * (only used for the objective function), and returns the number of points the
 * card is worth.
 */
int card_points(struct Card card, const void* null) {
    assert(null == NULL);
    return card.points;
}

/* Objective function used to prioritise cards to purchase. The best card is
 * the one which can be afforded by a player whos turn is soon after my turn.
 * Takes as arguments the card in question, and a pointer to the game struct.
 * Returns a heuristic value equal in the range [-1, playerCount] that
 * decreases by one for each player in the turn sequence after self that is not
 * able to afford the card.
 */
int turn_priority(struct Card card, const void* arg) {
    const struct Game* game = arg;

    // give a maximum score of the number of players, and deduct a point for
    // every turn that will go by in which this card cannot be purchased
    int score = game->playerCount;

    for (int i = 0; i < game->playerCount; ++i) {
        int playerId = (i + game->selfId) % game->playerCount;
        int tokens[TOKEN_MAX] = {0};
        struct Player copy = game->players[playerId];
        if (buy_card(tokens, &copy, card) == 0) {
            return score;
        } else {
            score -= 1;
        }
    }

    return -1;
}

/* Find the ideal card for ed to make decisions around. Takes as inputs the
 * game struct, and a pointer to where to store this card if its found. Returns
 * 0 if no card can be found which meets all the criteria, and returns 1 if a
 * card was found.
 */
int find_ideal_purchase(const struct Game* game, struct Card* output) {
    // first find all cards which _someone_ can afford
    struct Card board[BOARD_SIZE];
    int boardSize = 0;

    for (int i = 0; i < game->boardSize; ++i) {
        int canAfford = 0;
        for (int j = 0; j < game->playerCount; ++j) {
            if (j == game->selfId) {
                continue;
            }

            int tokens[TOKEN_MAX] = {0};
            struct Player copy = game->players[j];
            if (buy_card(tokens, &copy, game->board[i]) == 0) {
                canAfford = 1;
            }
        }

        if (canAfford) {
            board[boardSize++] = game->board[i];
        }
    }

    if (boardSize == 0) {
        return 0;
    }

    // filter out by points
    boardSize = find_best_purchases(board, boardSize, card_points, NULL);

    // filter out by who can afford the card
    boardSize = find_best_purchases(board, boardSize, turn_priority, game);

    // the best card is the oldest card - which is the one with the lowest ID
    *output = board[0];

    return 1;
}

/* Negotaties with the hub the purchase of a given card. Takes as arguments the
 * game state, and the card that is going to be purchased.
 */
void purchase_card(const struct Game* game, struct Card card) {
    struct PurchaseMessage msg;
    memset(&msg, 0, sizeof(struct PurchaseMessage));

    struct Player self = game->players[game->selfId];

    buy_card(msg.costSpent, &self, card);

    for (int i = 0; i < game->boardSize; ++i) {
        if (memcmp(&game->board[i], &card, sizeof(struct Card)) == 0) {
            msg.cardNumber = i;
            break;
        }
    }

    char* printed = print_purchase_message(msg);
    fputs(printed, stdout);
    fflush(stdout);
    free(printed);
}

/* Negotaties with the hub the aquiring of some tokens. Takes as arguments the
 * game state, and optionally also the "ideal" card that should be used in
 * prioritising token colours.
 */
void take_tokens(const struct Game* game, int idealExists, struct Card ideal) {
    struct Player self = game->players[game->selfId];

    struct TakeMessage msg;
    memset(&msg, 0, sizeof(struct TakeMessage));

    if (idealExists) {
        int canAfford[TOKEN_MAX - 1] = {0};
        for (int i = 0; i < TOKEN_MAX - 1; ++i) {
            if (self.discounts[i] + self.tokens[i] >= ideal.cost[i]) {
                canAfford[i] = 1;
            }
        }

        if (!canAfford[TOKEN_YELLOW]) {
            take_if_possible(msg.tokens, game->tokenCount, TOKEN_YELLOW);
        }

        if (!canAfford[TOKEN_RED]) {
            take_if_possible(msg.tokens, game->tokenCount, TOKEN_RED);
        }

        if (!canAfford[TOKEN_BROWN]) {
            take_if_possible(msg.tokens, game->tokenCount, TOKEN_BROWN);
        }

        if (!canAfford[TOKEN_PURPLE]) {
            take_if_possible(msg.tokens, game->tokenCount, TOKEN_PURPLE);
        }
    }

    take_if_possible(msg.tokens, game->tokenCount, TOKEN_YELLOW);
    take_if_possible(msg.tokens, game->tokenCount, TOKEN_RED);
    take_if_possible(msg.tokens, game->tokenCount, TOKEN_BROWN);
    take_if_possible(msg.tokens, game->tokenCount, TOKEN_PURPLE);

    char* printed = print_take_message(msg);
    fputs(printed, stdout);
    fflush(stdout);
    free(printed);
}

void make_move(const struct Game* game) {
    // the ideal card to make decisions off
    struct Card ideal;

    int idealExists = find_ideal_purchase(game, &ideal);
    int canPurchaseIdeal = 0;

    // first attempt to buy the ideal card
    if (idealExists) {
        struct Player copy = game->players[game->selfId];
        int tokenCopy[TOKEN_MAX] = {0};
        if (buy_card(tokenCopy, &copy, ideal) == 0) {
            // buy the card
            canPurchaseIdeal = 1;
        }
    }

    if (canPurchaseIdeal) {
        purchase_card(game, ideal);
    } else if (distinct_tokens_available(game->tokenCount, TOKEN_MAX - 1)
            >= TAKE_NUMBER) {
        take_tokens(game, idealExists, ideal);
    } else {
        fputs("wild\n", stdout);
        fflush(stdout);
    }
}
