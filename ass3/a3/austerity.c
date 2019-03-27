#include <stdbool.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/wait.h>

#include <game.h>
#include <token.h>
#include <util.h>
#include <protocol.h>

// maximum length (in chars) that a player number can take when printed out
// equal to 2 digits (max number is 26) and a null terminator
#define LENGTH_OF_PLAYER_NUMBER 3
#define INITIAL_CARD_BUFFER_SIZE 10

/* The exit codes for the hub.
 */
enum ExitCode {
    NORMAL_EXIT = 0,
    ARGUMENT_COUNT = 1,
    INVALID_ARGUMENTS = 2,
    OPEN_FILE = 3,
    INVALID_FILE = 4,
    SPAWN_FAILURE = 5,
    PLAYER_CLOSED = 6,
    PROTOCOL_ERROR = 7,
    ILLEGAL_MOVE = 8,
    INTERRUPTED = 10,
};

/* The information passed to an austerity program from its command line
 * arguments.
 */
struct Args {
    // The name of the file containing the card deck.
    char* deckFile;
    // The number of starting tokens in each of the (non-wild) piles
    int tokens;
    // The number of points required to trigger the end of the game
    int winScore;
    // The number of players in the game.
    int playerCount;
    // The programs required to launch each player.
    char** playerPrograms;
};

/* The information that describes a currently running child process.
 */
struct Child {
    // The process ID of the child (for killing)
    pid_t pid;
    // The file that can be written to to talk to the child.
    FILE* toChild;
    // The file that can be read from the listen to the child.
    FILE* fromChild;
    // 0 if the child is dead, 1 if the child is alive.
    int isAlive;
    // Undefined if the child is alive, but in the case of a dead child this is
    // the way that the child died - ie the status argument of wait. (exit
    // status of child)
    int meansOfDeath;
};

/* The hub's view of a player, including both the players state and the hub's
 * means of contacting that player.
 */
struct HubPlayer {
    // The player's state within the game
    struct Player playerState;
    // The means of contacting the process that represents that player
    struct Child process;
};

/* The hub's view of the game state.
 */
struct Game {
    // the number of points required to win
    int winScore;

    // The players in the game
    int playerCount;
    struct HubPlayer* players;

    // The cards currently on the board
    int boardSize;
    struct Card board[BOARD_SIZE];

    // The cards still in the deck, and not on the board
    int deckSize;
    struct Card* deck;

    // The number of tokens available for each non-wild token type
    int tokenCount[TOKEN_MAX - 1];
};

/* Does nothing. Used in some signal handlers.
 */
void do_nothing() {
}

/* Creates a handler that ignores a given signal. Does not set to SIG_IGN, but
 * instead just does nothing if the signal occurs. Used so that the signal is
 * able to be used with sigwait. Takes as a parameter the number of the signal,
 * and the flags passed to sigaction for that signal.
 */
void ignore_signal(int sig, int flags) {
    struct sigaction action;
    action.sa_handler = do_nothing;
    sigemptyset(&action.sa_mask);
    action.sa_flags = flags;
    sigaction(sig, &action, NULL);
}

/* Parse the command line arguments. Returns 0 if the arguments are valid, and
 * populates the output struct. If the arguments are not valid, returns the
 * relevant exit code.
 */
enum ExitCode parse_args(struct Args* output, int argc, char** argv) {
    // Within this function, each time I parse an argument I shorten the array.
    // Start by removing the first element from the array (the name of the
    // program), and then use this same pattern through rest of function.
    --argc;
    ++argv;

    // get the number of each token available (excluding wildcards)
    if (argc-- <= 0) {
        return ARGUMENT_COUNT;
    } else {
        char* arg = *argv++;
        char* end;
        output->tokens = parse_int(arg, &end);
        if (*end || arg == end || output->tokens < 0) {
            return INVALID_ARGUMENTS;
        }
    }

    // get the number of points to trigger end of game
    if (argc-- <= 0) {
        return ARGUMENT_COUNT;
    } else {
        char* arg = *argv++;
        char* end;
        output->winScore = parse_int(arg, &end);
        if (*end || arg == end || output->winScore < 0) {
            return INVALID_ARGUMENTS;
        }
    }

    // get the deckfile (and consume the argument)
    if (argc-- <= 0) {
        return ARGUMENT_COUNT;
    } else {
        output->deckFile = *argv++;
    }

    // get the player programs
    if (argc < MIN_PLAYERS || argc > MAX_PLAYERS) {
        return ARGUMENT_COUNT;
    } else {
        output->playerCount = argc;
        output->playerPrograms = argv;
    }

    return 0;
}

/* Open, close, and parse a deck file. Takes as arguments points to an array
 * (with length) of cards, and the file to open. On success, returns 0 and
 * leaves cardCount and cards pointing to an allocated array. On failure,
 * returns the relevant ExitCode, and leaves cardCount and cards pointing to
 * unspecified memory (cards will not need to be freed).
 */
enum ExitCode parse_deck_file(int* cardCount, struct Card** cards,
        const char* filename) {
    FILE* deck = fopen(filename, "r");
    if (deck == NULL) {
        return OPEN_FILE;
    }

    *cardCount = 0;
    int capacity = INITIAL_CARD_BUFFER_SIZE;
    *cards = malloc(sizeof(struct Card) * capacity);

    while (1) {
        char* line;
        if (read_line(deck, &line, 0) <= 0) {
            if (feof(deck)) {
                // EOF, no more cards
                if (*cardCount == 0) {
                    return INVALID_FILE;
                }
                break;
            } else {
                // empty line, or other IO error
                free(*cards);
                fclose(deck);
                return INVALID_FILE;
            }
        }

        if (parse_card(&(*cards)[*cardCount], line) != 0) {
            // could not parse the card
            free(*cards);
            fclose(deck);
            return INVALID_FILE;
        }
        *cardCount += 1;
        if (*cardCount + 1 >= capacity) {
            capacity *= 2;
            *cards = realloc(*cards, sizeof(struct Card) * capacity);
        }
        free(line);
    }

    fclose(deck);
    return 0;
}

/* Draws a card from the deck and moves it onto the board. Also will update all
 * players of this information. Takes as an argument a pointer to the game
 * state for updating.
 */
void draw_card(struct Game* game) {
    if (game->deckSize <= 0 || game->boardSize >= BOARD_SIZE) {
        return;
    }

    struct Card flip = game->deck[0];
    game->deckSize -= 1;
    memmove(game->deck, &game->deck[1], sizeof(struct Card) * game->deckSize);

    game->board[game->boardSize++] = flip;

    char* message = print_new_card_message(flip);
    for (int i = 0; i < game->playerCount; ++i) {
        fputs(message, game->players[i].process.toChild);
        fflush(game->players[i].process.toChild);
    }
    free(message);

    printf("New card = Bonus %c, worth %d, costs %d,%d,%d,%d\n",
            print_token(flip.discount), flip.points, flip.cost[TOKEN_PURPLE],
            flip.cost[TOKEN_BROWN], flip.cost[TOKEN_YELLOW],
            flip.cost[TOKEN_RED]);
}

/* Determines whether there are any cards left in the deck or on the board.
 * Used as part of calculating game overs. Takes as an argument a pointer to
 * the game struct, and returns true if there are cards left.
 */
bool cards_left(const struct Game* game) {
    if (game->boardSize == 0 && game->deckSize == 0) {
        return false;
    }

    return true;
}

/* Determines whether or not the game is currently over. Takes an an argument
 * the game state, and returns true if the game has completed, else false. Can
 * only be run at the end of a round.
 */
bool is_game_over(const struct Game* game) {

    if (!cards_left(game)) {
        return true;
    }

    for (int i = 0; i < game->playerCount; ++i) {
        if (game->players[i].playerState.score >= game->winScore) {
            return true;
        }
    }

    return false;
}

/* Alerts all players of a purchase that has been made by one player. Takes as
 * arguments the ID of the player who sent the message, the game (to access
 * children) and the body of the message sent by the purchasing player
 * requesting the purchase. Does not update game state in any way, or attempt
 * to validate the message.
 */
void send_purchased_message(int playerId, struct Game* game,
        struct PurchaseMessage received) {
    char* printed = print_purchased_message(received, playerId);
    for (int i = 0; i < game->playerCount; ++i) {
        fputs(printed, game->players[i].process.toChild);
        fflush(game->players[i].process.toChild);
    }
    free(printed);
}

/* Updates internal game state in response to a "purchase" message from a
 * player. Will return 0 if the message is valid (syntactically, semantically
 * and contextually), and the relevant exit code if it is not. Takes as
 * arguments the game state (to be updated), the message (to be parsed) and the
 * ID of the player the messgae was received from.
 */
enum ExitCode handle_purchase_message(int playerId, struct Game* game,
        const char* line) {
    struct PurchaseMessage body;
    if (parse_purchase_message(&body, line) != 0) {
        return PROTOCOL_ERROR;
    }

    // find the player involved
    struct Player* affected = &game->players[playerId].playerState;

    // find the card involved
    if (body.cardNumber < 0 || body.cardNumber >= game->boardSize) {
        return PROTOCOL_ERROR;
    }
    struct Card purchased = game->board[body.cardNumber];

    // test that the purchase is possible
    if (validate_costs(*affected, purchased, body.costSpent) != 0) {
        return PROTOCOL_ERROR;
    }

    // all is well, buy the card, remove the purchased card from the board and
    // start to tell everyone what happened
    buy_card(game->tokenCount, affected, purchased);
    send_purchased_message(playerId, game, body);

    game->boardSize -= 1;
    memmove(&game->board[body.cardNumber], &game->board[body.cardNumber + 1],
            sizeof(struct Card) * (game->boardSize - body.cardNumber));

    printf("Player %c purchased %d using %d,%d,%d,%d,%d\n", 'A' + playerId,
            body.cardNumber, body.costSpent[TOKEN_PURPLE],
            body.costSpent[TOKEN_BROWN], body.costSpent[TOKEN_YELLOW],
            body.costSpent[TOKEN_RED], body.costSpent[TOKEN_WILD]);

    draw_card(game);

    return 0;
}

/* Updates internal game state in response to a "take" message from a player.
 * Will return 0 if the message is valid (syntactically, semantically and
 * contextually), and the relevant exit code if it is not. Takes as arguments
 * the game state (to be updated), the message (to be parsed) and the Id of the
 * player the message was received from.
 */
enum ExitCode handle_take_message(int playerId, struct Game* game,
        const char* line) {
    struct TakeMessage body;
    if (parse_take_message(&body, line) != 0) {
        return PROTOCOL_ERROR;
    }

    struct Player* affected = &game->players[playerId].playerState;
    if (process_take_tokens(game->tokenCount, affected, body) < 0) {
        return PROTOCOL_ERROR;
    }

    char* printed = print_took_message(body, playerId);
    for (int i = 0; i < game->playerCount; ++i) {
        fputs(printed, game->players[i].process.toChild);
        fflush(game->players[i].process.toChild);
    }
    free(printed);

    printf("Player %c drew %d,%d,%d,%d\n", 'A' + playerId,
            body.tokens[TOKEN_PURPLE], body.tokens[TOKEN_BROWN],
            body.tokens[TOKEN_YELLOW], body.tokens[TOKEN_RED]);

    return 0;
}

/* Updates internal game state in response to a "wild" message from a player.
 * Takes as arguments the game state (to be updated), and the ID of the player
 * who sent the message.
 */
void handle_wild_message(int playerId, struct Game* game) {
    // update state
    game->players[playerId].playerState.tokens[TOKEN_WILD] += 1;

    // tell everyone
    char* printed = print_took_wild_message(playerId);
    for (int i = 0; i < game->playerCount; ++i) {
        fputs(printed, game->players[i].process.toChild);
        fflush(game->players[i].process.toChild);
    }
    free(printed);

    printf("Player %c took a wild\n", 'A' + playerId);
}

/* Find all players who are tying for the maximum score, and print out a comma
 * separated list of those players. Takes as its argument a pointer to the
 * game struct, and will rearrange the internal player list and leave it
 * invalid for further game play.
 */
void print_winners(struct Game* game) {
    int numWinners = 0;
    int winningScore = INT_MIN;

    for (int i = 0; i < game->playerCount; ++i) {
        int score = game->players[i].playerState.score;
        if (score > winningScore) {
            numWinners = 0;
        }

        if (score >= winningScore) {
            game->players[numWinners++].playerState =
                    game->players[i].playerState;
            winningScore = score;
        }
    }

    printf("Winner(s) ");
    for (int i = 0; i < numWinners; ++i) {
        printf("%c%c", 'A' + game->players[i].playerState.playerId,
                i + 1 == numWinners ? '\n' : ',');
    }
}

/* Spawns a child process. Returns 0 if the spawn is successful, and populates
 * the output struct. If the spawn fails in any way, cleans up and returns the
 * relevant exit code. Takes as arguments an output struct, the name of the
 * program to execute, and a null pointer terminated list of arguments for the
 * child process.
 */
enum ExitCode spawn_child(struct Child* output, const char* name,
        char* const* argv) {
    int toChild[2], fromChild[2], testPipe[2], errorFile;
    char buffer = 0;

    pipe(toChild);
    pipe(fromChild);
    pipe(testPipe);

    output->pid = fork();
    if (output->pid) {
        // we are the parent
        close(toChild[0]);
        close(fromChild[1]);
        close(testPipe[1]);

        int error = read(testPipe[0], &buffer, 1);
        if (error == 0) {
            // child spawned correctly, and the file closed on exec
            output->toChild = fdopen(toChild[1], "w");
            output->fromChild = fdopen(fromChild[0], "r");
            output->isAlive = 1;
            close(testPipe[0]);
            return NORMAL_EXIT;
        } else {
            // child did not spawn correctly, clean up and return the error
            close(toChild[1]);
            close(fromChild[0]);
            close(testPipe[0]);
            waitpid(output->pid, NULL, 0);
            return SPAWN_FAILURE;
        }
    } else {
        // we are the child
        // clean up, and set up
        close(toChild[1]);
        close(fromChild[0]);
        close(testPipe[0]);

        fcntl(testPipe[1], F_SETFD, FD_CLOEXEC);
        errorFile = open("/dev/null", O_WRONLY);
        dup2(toChild[0], STDIN_FILENO);
        dup2(fromChild[1], STDOUT_FILENO);
        dup2(errorFile, STDERR_FILENO);

        execvp(name, argv);

        // exec failed, report down test pipe
        write(testPipe[1], &buffer, 1);
        exit(EXIT_FAILURE);
    }
}

/* Reap any zombie children in existence. Takes as arguments an array (with
 * length) of children, and attempts to reap any of them. Does not block, and
 * returns the number of children successfully reaped.
 */
int reap_children(struct HubPlayer* players, int playerCount) {
    int deadChildren = 0, err;
    for (int i = 0; i < playerCount; ++i) {
        err = waitpid(players[i].process.pid,
                &players[i].process.meansOfDeath, WNOHANG);
        if (err == players[i].process.pid) {
            players[i].process.isAlive = 0;
            fclose(players[i].process.toChild);
            fclose(players[i].process.fromChild);
            deadChildren += 1;
        }
    }
    return deadChildren;
}

/* Kills all of the children of the hub. Does so by initially sending an "eog"
 * message, and then waiting for the children to die. If the children have not
 * all died within 2 seconds, they are killed with sigkill. This function will
 * not return until all child processes have died and been reaped. Takes as
 * arguments an array (and length) of players to terminate.
 */
void kill_children(struct HubPlayer* players, int playerCount) {
    // ask nicely
    for (int i = 0; i < playerCount; ++i) {
        fprintf(players[i].process.toChild, "eog\n");
        fflush(players[i].process.toChild);
    }

    // see if any children were already dead
    int deadChildren = reap_children(players, playerCount);

    // give them 2 seconds, then send SIGKILL
    if (deadChildren < playerCount) {
        int sigReceived;
        sigset_t waitFor, oldSignals;
        sigemptyset(&waitFor);
        sigaddset(&waitFor, SIGCHLD);
        sigaddset(&waitFor, SIGALRM);
        ignore_signal(SIGCHLD, 0);
        sigprocmask(SIG_BLOCK, &waitFor, &oldSignals);

        alarm(2);

        while (deadChildren < playerCount) {
            sigwait(&waitFor, &sigReceived);
            switch (sigReceived) {
                case SIGCHLD:
                    // one or more children just died
                    deadChildren += reap_children(players, playerCount);
                    break;
                case SIGALRM:
                    // time is up, kill violently
                    for (int i = 0; i < playerCount; ++i) {
                        if (players[i].process.isAlive) {
                            kill(players[i].process.pid, SIGKILL);
                        }
                    }
                    break;
            }
        }

        // clean up signal masks and alarms set during the function
        sigprocmask(SIG_SETMASK, &oldSignals, NULL);
        alarm(0);
    }
}

/* Prints information about how children exited, if they exited abnormally.
 * Takes as arguments an array (with length) of the players.
 */
void print_child_statuses(int playerCount, struct HubPlayer* players) {
    for (int i = 0; i < playerCount; ++i) {
        struct Child child = players[i].process;
        if (WIFEXITED(child.meansOfDeath)) {
            int status = WEXITSTATUS(child.meansOfDeath);
            if (status) {
                fprintf(stderr, "Player %c ended with status %d\n", 'A' + i,
                        status);
            }
        } else if (WIFSIGNALED(child.meansOfDeath)) {
            fprintf(stderr, "Player %c shut down after receiving signal %d\n",
                    'A' + i, WTERMSIG(child.meansOfDeath));
        }
    }
}

/* Spawns a child process for each player in the game, and configures the
 * struct that will store state relating to that player. Takes as arguments the
 * command line arguments that were provided to the hub, and a pointer to the
 * location in memory where the list of players should be stored after this
 * function completes. On success, this will return 0, and on failure this will
 * return the error code that the program should exit with. The value pointed
 * to by output, on successful completion of the program, will need to be
 * deallocated with free. If an error occurs this will not be necessary.
 */
enum ExitCode spawn_players(struct Args arguments,
        struct HubPlayer** output) {

    // make space in memory for the argv of each player
    char numberOfPlayers[LENGTH_OF_PLAYER_NUMBER];
    char thisPlayer[LENGTH_OF_PLAYER_NUMBER];
    char* argv[] = {NULL, numberOfPlayers, thisPlayer, NULL};

    // initialize things
    sprintf(numberOfPlayers, "%d", arguments.playerCount);
    *output = malloc(sizeof(struct HubPlayer) * arguments.playerCount);
    enum ExitCode err = NORMAL_EXIT;

    // do the spawning
    int i;
    for (i = 0; i < arguments.playerCount; ++i) {
        argv[0] = arguments.playerPrograms[i];
        sprintf(thisPlayer, "%d", i);

        err = spawn_child(&(*output)[i].process, argv[0], argv);
        if (err) {
            break;
        }
        initialize_player(&(*output)[i].playerState, i);
    }

    if (err) {
        // an error occured while spawning, kill everything
        kill_children(*output, i);
        free(*output);
    }

    return err;
}

/* Takes the command line arguments, and builds a game from them. Returns the
 * relevant exit code if something goes wrong. Returns 0 and stores the initial
 * game state in output on success. Once this function is complete, the first
 * round is ready to begin.
 */
enum ExitCode setup_game(struct Args arguments, struct Game* output) {
    enum ExitCode err = 0;
    char* message;

    output->winScore = arguments.winScore;

    // parse deck file
    err = parse_deck_file(
            &output->deckSize, &output->deck, arguments.deckFile);
    if (err) {
        return err;
    }

    // spawn players
    err = spawn_players(arguments, &output->players);
    if (err) {
        free(output->deck);
        return err;
    }
    output->playerCount = arguments.playerCount;

    // initialize token counts
    for (int i = 0; i < TOKEN_MAX - 1; ++i) {
        output->tokenCount[i] = arguments.tokens;
    }
    message = print_tokens_message(arguments.tokens);
    for (int i = 0; i < output->playerCount; ++i) {
        fputs(message, output->players[i].process.toChild);
        fflush(output->players[i].process.toChild);
    }
    free(message);

    // set up board
    output->boardSize = 0;
    for (int i = 0; i < BOARD_SIZE; ++i) {
        draw_card(output);
    }

    return err;
}

/* Process one player's turn, from sending the do what message to being ready
 * to send the do what message to the next player. Does not handle retries in
 * the case where the player sends an invalid message.
 */
enum ExitCode do_what(struct Game* game, int playerId) {
    enum ExitCode err = 0;
    FILE* toChild = game->players[playerId].process.toChild;
    FILE* fromChild = game->players[playerId].process.fromChild;

    fputs("dowhat\n", toChild);
    fflush(toChild);

    char* line;
    int readBytes = read_line(fromChild, &line, 0);
    if (readBytes <= 0) {
        if (ferror(fromChild) && errno == EINTR) {
            free(line);
            return INTERRUPTED;
        } else if (feof(fromChild)) {
            free(line);
            return PLAYER_CLOSED;
        }
    }
    enum MessageFromPlayer type = classify_from_player(line);
    switch(type) {
        case PURCHASE:
            err = handle_purchase_message(playerId, game, line);
            break;
        case TAKE:
            err = handle_take_message(playerId, game, line);
            break;
        case WILD:
            handle_wild_message(playerId, game);
            break;
        default:
            free(line);
            return PROTOCOL_ERROR;
    }
    free(line);
    return err;
}

/* Play the game, starting at the first round. Will return at the end of the
 * game, either with 0 or with the relevant exit code.
 */
enum ExitCode play_game(struct Game* game) {
    enum ExitCode err = 0;
    while (!is_game_over(game)) {
        // each player takes their turn
        for (int i = 0; i < game->playerCount && cards_left(game); ++i) {
            err = do_what(game, i);
            if (err == PROTOCOL_ERROR) {
                err = do_what(game, i);
            }

            if (err) {
                return err;
            }
        }
    }
    return 0;
}

/* Exit the program, and print the relevant game over message. Takes as its
 * argument the exit code that the program is exiting with.
 */
void game_over(enum ExitCode code) {
    switch (code) {
        case NORMAL_EXIT:
            break;
        case ARGUMENT_COUNT:
            fprintf(stderr, "Usage: austerity tokens points deck player player"
                    " [player ...]\n");
            break;
        case INVALID_ARGUMENTS:
            fprintf(stderr, "Bad argument\n");
            break;
        case OPEN_FILE:
            fprintf(stderr, "Cannot access deck file\n");
            break;
        case INVALID_FILE:
            fprintf(stderr, "Invalid deck file contents\n");
            break;
        case SPAWN_FAILURE:
            fprintf(stderr, "Bad start\n");
            break;
        case PLAYER_CLOSED:
            fprintf(stderr, "Client disconnected\n");
            break;
        case PROTOCOL_ERROR:
            fprintf(stderr, "Protocol error by client\n");
            break;
        case ILLEGAL_MOVE:
            fprintf(stderr, "Illegal move by client\n");
            break;
        case INTERRUPTED:
            fprintf(stderr, "SIGINT caught\n");
            break;
    }

    exit(code);
}

int main(int argc, char** argv) {
    // Ignores the sigint and sigpipe signal. Sigpipe is configured so that IO
    // that is interrupted by the signal will be restart, sigint is configured
    // so that IO that is interrupted by the signal will terminate. This means
    // that an early fail to do IO can be interpreted as a (probable) SIGINT.
    ignore_signal(SIGINT, 0);
    ignore_signal(SIGPIPE, SA_RESTART);

    enum ExitCode err;
    struct Args args;
    struct Game game;

    err = parse_args(&args, argc, argv);
    if (err) {
        game_over(err);
    }

    err = setup_game(args, &game);
    if (err) {
        game_over(err);
    }

    err = play_game(&game);
    if (err == PLAYER_CLOSED) {
        printf("Game ended due to disconnect\n");
    } else if (err == 0) {
        print_winners(&game);
    }

    kill_children(game.players, game.playerCount);
    if (err != INTERRUPTED) {
        print_child_statuses(game.playerCount, game.players);
    }

    free(game.players);
    free(game.deck);

    game_over(err);
}
