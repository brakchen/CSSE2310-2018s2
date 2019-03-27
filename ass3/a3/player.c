#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <game.h>
#include <protocol.h>
#include <token.h>
#include <util.h>

#include "player.h"

/* The exit codes for a player.
 */
enum ExitCode {
    NORMAL_EXIT = 0,
    ARGUMENT_COUNT = 1,
    PLAYER_NUMBER = 2,
    PLAYER_ID = 3,
    COMMUNICATION_ERROR = 6,
};

/* Parse the command line arguments. Returns 0 if the arguments are valid, and
 * populates the output struct. If the arguments are not valid, returns the
 * relevant exit code. Only fields directly related to command line arguments
 * within the game struct are populated by this function. Not populated is
 * tokenCount, boardSize or players.
 */
enum ExitCode parse_args(struct Game* output, int argc, char** argv) {
    --argc;
    ++argv;

    char* arg;
    char* end;

    if (argc-- <= 0) {
        return ARGUMENT_COUNT;
    } else {
        arg = *argv++;
        output->playerCount = parse_int(arg, &end);
        if (*end || arg == end || output->playerCount < MIN_PLAYERS ||
                output->playerCount > MAX_PLAYERS) {
            return PLAYER_NUMBER;
        }
    }

    if (argc-- <= 0) {
        return ARGUMENT_COUNT;
    } else {
        arg = *argv++;
        output->selfId = parse_int(arg, &end);
        if (*end || arg == end || output->selfId < 0 ||
                output->selfId >= output->playerCount) {
            return PLAYER_ID;
        }
    }

    if (argc == 0) {
        return 0;
    } else {
        return ARGUMENT_COUNT;
    }
}

/* Prints to stderr the information relating to winners at the end of the game.
 * Takes as an argument the game struct full of information to be printed.
 */
void display_eog_info(const struct Game* game) {
    int numWinners = 0;
    int winningScore = INT_MIN;

    for (int i = 0; i < game->playerCount; ++i) {
        int score = game->players[i].score;
        if (score > winningScore) {
            numWinners = 0;
        }

        if (score >= winningScore) {
            game->players[numWinners++] = game->players[i];
            winningScore = score;
        }
    }

    fprintf(stderr, "Game over. Winners are ");
    for (int i = 0; i < numWinners; ++i) {
        fprintf(stderr, "%c%c", 'A' + game->players[i].playerId,
                i + 1 == numWinners ? '\n' : ',');
    }
}

/* Prints to stderr the information relating to game state that needs to be
 * printed after each message that isn't eog or dowhat.
 */
void display_turn_info(const struct Game* game) {
    for (int i = 0; i < game->boardSize; ++i) {
        struct Card card = game->board[i];
        fprintf(stderr, "Card %d:%c/%d/%d,%d,%d,%d\n", i,
                print_token(card.discount), card.points,
                card.cost[TOKEN_PURPLE], card.cost[TOKEN_BROWN],
                card.cost[TOKEN_YELLOW], card.cost[TOKEN_RED]);
    }
    for (int i = 0; i < game->playerCount; ++i) {
        struct Player player = game->players[i];
        fprintf(stderr,
                "Player %c:%d:Discounts=%d,%d,%d,%d:Tokens=%d,%d,%d,%d,%d\n",
                'A' + player.playerId, player.score,
                player.discounts[TOKEN_PURPLE], player.discounts[TOKEN_BROWN],
                player.discounts[TOKEN_YELLOW], player.discounts[TOKEN_RED],
                player.tokens[TOKEN_PURPLE], player.tokens[TOKEN_BROWN],
                player.tokens[TOKEN_YELLOW], player.tokens[TOKEN_RED],
                player.tokens[TOKEN_WILD]);
    }
}

/* Updates internal game state in response to a "PURCHASED" message from the
 * hub. Will return 0 if the message is valid (syntactically, semantically and
 * contextually), and the relevant exit code if it is not. Takes as arguments
 * the game state (to be updated) and the message (to be parsed).
 */
enum ExitCode handle_purchased_message(struct Game* game, const char* line) {
    struct PurchaseMessage body;
    int playerId;
    if (parse_purchased_message(&body, &playerId, line) != 0) {
        return COMMUNICATION_ERROR;
    }

    // find the player involved
    if (playerId < 0 || playerId >= game->playerCount) {
        return COMMUNICATION_ERROR;
    }
    struct Player* affected = &game->players[playerId];

    // find the card involved
    if (body.cardNumber < 0 || body.cardNumber >= game->boardSize) {
        return COMMUNICATION_ERROR;
    }
    struct Card purchased = game->board[body.cardNumber];

    // check that the costs line up
    if (validate_costs(*affected, purchased, body.costSpent) != 0) {
        return COMMUNICATION_ERROR;
    }

    // buy the card, and remove it from the board
    buy_card(game->tokenCount, affected, purchased);
    game->boardSize -= 1;
    memmove(&game->board[body.cardNumber], &game->board[body.cardNumber + 1],
            sizeof(struct Card) * (game->boardSize - body.cardNumber));

    return 0;
}

/* Updates internal game state in response to a "TOOK" message from the hub.
 * Will return 0 if the message is valid (syntactically, semantically and
 * contextually), and the relevant exit code if it is not. Takes as arguments
 * the game state (to be updated) and the message (to be parsed).
 */
enum ExitCode handle_took_message(struct Game* game, const char* line) {
    struct TakeMessage body;
    int playerId;
    if (parse_took_message(&body, &playerId, line) != 0) {
        return COMMUNICATION_ERROR;
    }

    if (playerId < 0 || playerId >= game->playerCount) {
        return COMMUNICATION_ERROR;
    }

    struct Player* affected = &game->players[playerId];
    if (process_take_tokens(game->tokenCount, affected, body) < 0) {
        return COMMUNICATION_ERROR;
    }

    return 0;
}

/* Updates internal game state in response to a "WILD" message from the hub.
 * Will return 0 if the message is valid (syntactically, semantically and
 * contextually), and the relevant exit code if it is not. Takes as arguments
 * the game state (to be updated) and the message (to be parsed).
 */
enum ExitCode handle_took_wild_message(struct Game* game, const char* line) {
    int playerId;
    if (parse_took_wild_message(&playerId, line) != 0) {
        return COMMUNICATION_ERROR;
    }

    if (playerId < 0 || playerId >= game->playerCount) {
        return COMMUNICATION_ERROR;
    }
    struct Player* affected = &game->players[playerId];
    affected->tokens[TOKEN_WILD] += 1;

    return 0;
}

/* Updates internal game state in response to a "NEW CARD" message from the
 * hub. Will return 0 if the message is valid (syntactically, semantically and
 * contextually), and the relevant exit code if it is not. Takes as arguments
 * the game state (to be updated) and the message (to be parsed).
 */
enum ExitCode handle_new_card_message(struct Game* game, const char* line) {
    struct Card card;
    if (parse_new_card_message(&card, line) != 0) {
        return COMMUNICATION_ERROR;
    }

    if (game->boardSize >= BOARD_SIZE) {
        // the card doesn't fit
        return COMMUNICATION_ERROR;
    }

    game->board[game->boardSize++] = card;

    return 0;
}

/* Reads the intial message from the hub, describing initial tokens size, and
 * then initialises all fields within the game struct not set up by the
 * argument parsing code. Returns 0 on success, or the relevant error code on
 * failure.
 */
enum ExitCode setup_game(struct Game* game) {
    char* line;
    int readBytes = read_line(stdin, &line, 0);
    if (readBytes <= 0) {
        return COMMUNICATION_ERROR;
    }

    int tokens;
    if (parse_tokens_message(&tokens, line) != 0) {
        free(line);
        return COMMUNICATION_ERROR;
    }
    free(line);
    for (int i = 0; i < TOKEN_MAX - 1; ++i) {
        game->tokenCount[i] = tokens;
    }


    game->players = malloc(sizeof(struct Player) * game->playerCount);
    for (int i = 0; i < game->playerCount; ++i) {
        initialize_player(&game->players[i], i);
    }

    game->boardSize = 0;

    display_turn_info(game);

    return 0;
}

/* Play the game, starting at the first round. Will return at the end of the
 * game, either with 0 or with the relevant exit code.
 */
enum ExitCode play_game(struct Game* game) {
    enum ExitCode err = 0;
    while (1) {
        char* line;
        int readBytes = read_line(stdin, &line, 0);
        if (readBytes <= 0) {
            return COMMUNICATION_ERROR;
        }
        enum MessageFromHub type = classify_from_hub(line);
        switch (type) {
            case END_OF_GAME:
                free(line);
                display_eog_info(game);
                return NORMAL_EXIT;
            case DO_WHAT:
                fprintf(stderr, "Received dowhat\n");
                make_move(game);
                break;
            case PURCHASED:
                err = handle_purchased_message(game, line);
                break;
            case TOOK:
                err = handle_took_message(game, line);
                break;
            case TOOK_WILD:
                err = handle_took_wild_message(game, line);
                break;
            case NEW_CARD:
                err = handle_new_card_message(game, line);
                break;
            default:
                free(line);
                return COMMUNICATION_ERROR;
        }
        free(line);
        if (err) {
            return err;
        } else if (type != DO_WHAT) {
            display_turn_info(game);
        }
    }
}

/* Exit the program, and print the relevant game over message.
 */
void game_over(enum ExitCode code) {
    switch (code) {
        case NORMAL_EXIT:
            break;
        case ARGUMENT_COUNT:
            fprintf(stderr, "Usage: %s pcount myid\n", process_name());
            break;
        case PLAYER_NUMBER:
            fprintf(stderr, "Invalid player count\n");
            break;
        case PLAYER_ID:
            fprintf(stderr, "Invalid player ID\n");
            break;
        case COMMUNICATION_ERROR:
            fprintf(stderr, "Communication Error\n");
            break;
    }

    exit(code);
}

int main(int argc, char** argv) {
    enum ExitCode err;
    struct Game game;

    err = parse_args(&game, argc, argv);
    if (err) {
        game_over(err);
    }

    err = setup_game(&game);
    if (err) {
        game_over(err);
    }

    err = play_game(&game);

    free(game.players);

    game_over(err);
}
