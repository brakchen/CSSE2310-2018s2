#ifndef _PLAYER_H_
#define _PLAYER_H_

#include <protocol.h>

/* The information that describes a game, as seen by the player.
 */
struct Game {
    // The total number of players
    int playerCount;
    // The state of each player within the game
    struct Player* players;
    // The ID of this individual player
    int selfId;
    // The number of available tokens of each type (make space for wild tokens,
    // but never use them)
    int tokenCount[TOKEN_MAX];
    // The number of cards currently on the board - guaranteed to be in the
    // range [0. BOARD_SIZE)
    int boardSize;
    // The cards currently on the board
    struct Card board[BOARD_SIZE];
};

/* Makes a move within the game. Does not change any internal state, instead
 * the internal game state should be updated by the hub's response to that move
 * broadcasting the change in state.
 */
void make_move(const struct Game* game);

/* Returns the name of this process, for error message purposes.
 */
char* process_name(void);

#endif
