#include "austerity.h"

static int debugA = 0;

/**
 * main
 * @param args
 * @param argv
 * @return
 */
int main(int args, char** argv) {
    check_hub_args(args, argv);
    bad_start_check(args, argv);
    game(args, argv);
}

/**
 * main game function
 * @param arg
 * @param args
 */
void game(int arg, char** args) {
    Hub* hub = (Hub*) calloc(1, sizeof(Hub));
    read_cards(&(hub->deck), args[3]);
    int tokens = atoi(args[1]), endPoint = atoi(args[2]), totalPlayer =
            arg - 4;
    hub->players = player_setup(totalPlayer, args);
    hub->childPipes = (int**) calloc(26, sizeof(int*));
    hub->parentPipes = (int**) calloc(26, sizeof(int*));
    hub->totalPlayer = arg - 4;
    hub->pids = (int*) calloc(totalPlayer, sizeof(int));
    hub->endPoint = endPoint;
    for (int i = 0; i < 5; i++) {
        hub->token[i] = tokens;
    }
    pid_t parentPid = getpid();
    for (int i = 0; i < totalPlayer; i++) {
        hub->parentPipes[i] = (int*) calloc(2, sizeof(int));
        hub->childPipes[i] = (int*) calloc(2, sizeof(int));
        pipe(hub->childPipes[i]);/*children pipe setup*/
        pipe(hub->parentPipes[i]);
        if (getpid() == parentPid) {
            switch (fork()) {
                case 0:/*child*/
                    hub->pids[i] = getpid();
                    child(hub, i);
                    return;
                case -1:/*fork fail*/
                    printf("Faild to fokr()");
                    exit(90);
                    break;
                default:
                    break;
            }
        }
    }
    /*parent finish fork reading to send message*/
    if (getpid() == parentPid)
        //init_player_info(hub);
        start_game(hub);
    /*wait all children finish*/
    wait(NULL);
}

/**
 * check is win
 * @param hub hub data
 * @return if 1 yes ,0 no
 */
int is_win(Hub* hub) {
    if (is_someone_reach_win(hub)) {
        return 1;
    };
    if (get_card_amount(hub->board) == 0) {
        //fflush(stdout);
        return 1;
    }
    return 0;
}

/**
 * handle parent stuff.
 * Erros:
 * will rise error if there is invalid message happened twice
 * @param hub hub data
 */
void start_game(Hub* hub) {/*main logical function*/
    hub->board = construct_cards_board(&(hub->deck));
    int* parentPipe = NULL;
    int* childPipe = NULL;
    int totalPlayer = hub->totalPlayer;
    Player* player = NULL;
    send_tokens_action(hub);
    send_new_cards_action(hub);
    sleep(1);/*incase override*/
    char** rev = NULL;
    int bad_message = 0, count = 0, win = 0;
    while (win == 0) {
        for (int i = 0; i < totalPlayer;) {
            player = (hub->players)[i];
            parentPipe = hub->parentPipes[i];
            childPipe = hub->childPipes[i];
            send(parentPipe, "dowhat", childPipe);
            debugA++;
            rev = get(childPipe, &count);
            for (int j = 0; j < count; j++) {
                if (rev[j] == NULL) {
                    fflush(stdout);
                }
                if (!interpret_message(rev[j], player, hub)) {
                            
                    fflush(stdout);
                    bad_message++;
                    if (bad_message == 2) {/*should rasie error*/
                        hub_error(BAD_PROTOCOL);
                    }
                    continue;/*reask dowhat*/
                } else {
                    i++;/*valid messsage will turn to next player*/
                }
            }
        }
        win = is_win(hub);
    }
    for (int i = 0; i < totalPlayer; i++) {/*end of game*/
        player = (hub->players)[i];
        parentPipe = hub->parentPipes[i];
        childPipe = hub->childPipes[i];
        send(parentPipe, "eog", childPipe);
    }
    hub_output_eog(hub);

    for (int i = 0; i < count; i++) {
        free(rev[i]);
    }
    free(rev);

}

/**
 * child process
 * @param hub hub data
 * @param who child 's name
 */
void child(Hub* hub, int who) {
    int totalPlayer = hub->totalPlayer;
    Player* player = hub->players[who];
    int* parentPipe = hub->parentPipes[who];
    int* childPipe = hub->childPipes[who];
    char* tmp = (char*) calloc(100, sizeof(char));
    char* tmp1 = (char*) calloc(100, sizeof(char));
    sprintf(tmp, "%d", totalPlayer);
    sprintf(tmp1, "%d", player->ID);
    //close(parentPipe[WRITE]);
    // close(childPipe[READ]);
    dup2(childPipe[WRITE], STDOUT_FD);
    dup2(parentPipe[READ], STDIN_FD);
    int waste = open("/dev/null", O_WRONLY);
    dup2(waste, STDERR_FD);
    char* playerArgs[4] = {player->name, tmp, tmp1, NULL};
    if (strlen(player->name) > 0) {/*call player process*/
        if (execvp((player->name), playerArgs) < 0) {
            hub_error(BAD_START);
            free(tmp);
            free(tmp1);
        }

        free(tmp);
        free(tmp1);
    }


}

/**
 * inform other player tokens is comming
 * @param hub
 */
void send_tokens_action(Hub* hub) {
    char message[BUFFER_SIZE] = {'\0'};
    int totalPlayer = hub->totalPlayer;
    int* parentPipe, * childPipe;
    for (int i = 0; i < totalPlayer; i++) {
        parentPipe = hub->parentPipes[i];
        childPipe = hub->childPipes[i];
        snprintf(message, BUFFER_SIZE, "tokens%d", hub->token[0]);
        send(parentPipe, message, childPipe);
    }

}

/**
 * inform other player someone buy tokens
 * @param hub
 * @param who
 * @param token
 */
void send_take_action(Hub* hub, char who, int* token) {
    char message[BUFFER_SIZE] = {'\0'};
    int* parentPipe, * childPipe;
    int totalPlayer = hub->totalPlayer;
    snprintf(message, BUFFER_SIZE,
             "took%c:%d,%d,%d,%d",
             who,
             token[0],
             token[1],
             token[2],
             token[3]);
    for (int i = 0; i < totalPlayer; i++) {
        parentPipe = hub->parentPipes[i];
        childPipe = hub->childPipes[i];
        send(parentPipe, message, childPipe);
        //close(childPipe[READ]);
    }
    memset(message, '\0', BUFFER_SIZE);
    fprintf(stdout, message);
    fflush(stdout);
}

/**
 * infom other player some one buy a newcard
 * @param hub
 */
void send_new_cards_action(Hub* hub) {
    Cards* tmp = hub->board;
    char message[BUFFER_SIZE] = {'\0'};
    int* parentPipe, * childPipe;
    int totalPlayer = hub->totalPlayer;
    for (; tmp != NULL;) {
        snprintf(message, BUFFER_SIZE,
                 "newcard%c:%d:%d,%d,%d,%d",
                 tmp->color,
                 tmp->worth,
                 tmp->price[0],
                 tmp->price[1],
                 tmp->price[2],
                 tmp->price[3]);
        for (int i = 0; i < totalPlayer; i++) {
            parentPipe = hub->parentPipes[i];
            childPipe = hub->childPipes[i];
            send(parentPipe, message, childPipe);
            //close(childPipe[READ]);
        }
        memset(message, '\0', BUFFER_SIZE);
        snprintf(message, BUFFER_SIZE,
                 "New card = Bonus %c, worth %d, costs %d,%d,%d,%d\n",

                 tmp->color,
                 tmp->worth,
                 tmp->price[0],
                 tmp->price[1],
                 tmp->price[2],
                 tmp->price[3]);
        fprintf(stdout, message);
        fflush(stdout);
        tmp = tmp->next;
    }
}


/**
 * info all player who buy a call
 * hub
 * who -> who bought a card
 * num -> card number
 * @param hub
 * @param who
 * @param cardNum
 * @param num
 */
void send_purchased_action(Hub* hub, char who, int cardNum, int* num) {
    char message[BUFFER_SIZE] = {'\0'};
    int* parentPipe, * childPipe;
    int totalPlayer = hub->totalPlayer;
    snprintf(message, BUFFER_SIZE,
             "purchased%c:%d:%d,%d,%d,%d,%d",
             who,
             cardNum,
             num[1],
             num[2],
             num[3],
             num[4],
             num[5]
    );
    for (int i = 0; i < totalPlayer; i++) {
        parentPipe = hub->parentPipes[i];
        childPipe = hub->childPipes[i];
        send(parentPipe, message, childPipe);
    }
    memset(message, '\0', BUFFER_SIZE);
}

/**
 * inform other player someone take wild
 * @param hub
 * @param who
 */
void send_wild_action(Hub* hub, char who) {

    char message[BUFFER_SIZE] = {'\0'};
    int* parentPipe, * childPipe;
    int totalPlayer = hub->totalPlayer;
    snprintf(message, BUFFER_SIZE, "wild%c", who);
    for (int i = 0; i < totalPlayer; i++) {
        parentPipe = hub->parentPipes[i];
        childPipe = hub->childPipes[i];
        send(parentPipe, message, childPipe);
    }


}

/**
 * update player info
 * @param player
 * @param tokens
 * @param action
 */
void update_hub_player_tokens(Player* player, int* tokens, char* action) {
    if (strcmp(action, "add") == 0) {
        for (int i = 0; i < 4; i++) {
            player->tokens[i] += tokens[i];
        }
    }
    if (strcmp(action, "minus") == 0) {

        for (int i = 0; i < 4; i++) {
            player->tokens[i] -= tokens[i];
        }
    }
}

/**
 * update player info
 * @param hub
 * @param tokens
 * @param action
 */
void update_hub_tokens(Hub* hub, int* tokens, char* action) {
    if (strcmp(action, "add") == 0) {
        for (int i = 0; i < 4; i++) {
            hub->token[i] += tokens[i];
        }
    }
    if (strcmp(action, "minus") == 0) {

        for (int i = 0; i < 4; i++) {
            hub->token[i] -= tokens[i];
        }
    }
}

/**
 * update player info
 * @param player
 * @param discount
 */
void update_hub_player_discount(Player* player, char discount) {
    char keyword[4] = "PBYR";
    for (int i = 0; i < 4; i++) {
        if (keyword[i] == discount)
            player->discount[i]++;
    }


}

/**
 *interpret the message back from child process
 *code-> the message
 *player-> who send this meesage
 *return: 1 if valid message ,0 is invalid message
 * @param code
 * @param player
 * @param hub
 * @return
 */

int interpret_message(char* code, Player* player, Hub* hub) {
    int* result = NULL;
    Cards* card = NULL;
    int tokens[4];
    char who = player->indicator;
    switch (interpret_keyword(code)) {
        case PLAYER_PURCHASE:/*player bought a card*/
            result = interpret_purchase(code);
            card = find_element(hub->board, result[0]);
            update_hub_player_discount(player, card->color);
            update_hub_tokens(hub, tokens, "add");
            update_hub_player_tokens(player, tokens, "minus");
            send_purchased_action(hub, who, card->No, result);
            del_element(&(hub->board), result[0]);
            hub_output(PLAYER_PURCHASE, who, result);
            /*refill card*/
            if (get_card_amount(hub->deck) != 0) {
                refill_a_card_and_send_message(hub);
            }
            refresh_id(hub->board);
            player->totalPoints += card->worth;
            free(result);
            return 1;
        case PLAYER_TAKE:/*player took tokens*/
            result = interpret_take(code);
            update_hub_tokens(hub, result, "minus");
            update_hub_player_tokens(player, result, "add");
            hub_output(PLAYER_TAKE, who, result);
            send_take_action(hub, who, result);
            free(result);
            return 1;
        case PLAYER_WILD:/*Player took a wild*/
            send_wild_action(hub, player->indicator);
            player->tokens[4]++;
            hub_output(PLAYER_WILD, who, result);
            free(result);
            return 1;
        default:/*ivalid message should raise error*/
            return 0;
    }
}

/**
 * pasring "purchase" message from hub
 * return a 1D int array which include:
 * [int,int,int,int,int,int]
 * [number,colour,colour,colour,colour,colour]
 * @param code
 * @return
 */
int* interpret_purchase(char* code) {
    char* array = (char*) calloc(50, sizeof(char));
    int* result = (int*) calloc(6, sizeof(int));
    char dest[40] = {'\0'};
    int lastComma = 0, codeSize = strlen(code), index = 0;
    for (int i = 0; i < codeSize; i++)
        if (code[i] == ':')
            lastComma = i;

    /*left size*/
    for (int i = 8; i < lastComma; i++) {
        if (is_number(code[i]))
            result[index++] = atoi(&code[i]);
    }
    /*right side*/
    int j = 0;
    strcpy(dest, code + lastComma + 1);
    for (int i = 0; i < strlen(dest); i++) {
        if (is_comma_or_colon(dest[i])) {
            result[index++] = atoi(array);
            memset(array, '\0', 50);
            j = 0;
        } else {
            array[j++] = dest[i];
        }
    }

    result[index++] = atoi(array);
    free(array);
    return result;
}

/**
 * pasring "take" message from hub
 * return a 1D int array which include:
 * [int,int,int,int]
 * [colour,colour,colour,colour]
 * @param code
 * @return
 */
int* interpret_take(char* code) {
    int* array = (int*) calloc(50, sizeof(int));
    int* result = (int*) calloc(5, sizeof(int));
    char dest[40] = {'\0'};
    int index = 0;
    /*right side*/
    split(&array, strcpy(dest, code + 4), 0);
    for (int i = 0; array[i] != -1; i++)
        result[index++] = array[i];

    free(array);
    return result;
}


/**
 * parse keyword that spec proviced
 * parameter:
 * role parse who'message
 *could input any chars
 *return relative number if ok
 * elese return -1 means dismatch
 * @param text
 * @return
 */
int interpret_keyword(char* text) {
    char* keywords[3] = {
            "purchase",
            "take",
            "wild",};
    int keywordsSize = sizeof(keywords) / sizeof(char*);
    for (int i = 0; i < keywordsSize; i++) {
        /*base of keywrods*/
        int textSize = strlen(keywords[i]);
        for (int j = 0; j < textSize; j++) {
            if (keywords[i][j] != text[j]) {
                /* if there is one dismatch*/
                break;
            }
            if ((j + 1) == textSize) {/* match string*/

                return i + 1;
            }
        }
    }
    return -1;
}

/**
 * check input args
 * @param args
 * @param argv
 */
void check_hub_args(int args, char** argv) {
    if (args < 6 || args >= 31)/*player >26 included*/
        hub_error(BAD_ARGS_COUNT);
    if (!is_all_numbers(argv[1]) || !is_all_numbers(argv[2]))
        hub_error(BAD_ARGS);

}

/**
 *send message to child process from parent
 *where-> send to where
 *message -> the message need to be seed
 *toWho-> send to who
 * @param where
 * @param message
 * @param toWho
 */
void send(int where[2], char* message, int toWho[2]) {
    /*where means send to where*/
    //close(where[READ]);
    //close(toWho[WRITE]);
    write(where[WRITE], message, BUFFER_SIZE);
    //close(where[WRITE]);
}

/**
 *read message from child process
 *return -1 means error
 *return 0 means empty buffer
 *return >0 means fully readed
 * @param fromWho
 * @param count
 * @return
 */
char** get(int fromWho[2], int* count) {/*fromWho means from which plauyer*/
    int i = 0, j = 0, k = 0, flag = 0;
    *count = 0;
    int rev = 0;
    char** message = (char**) calloc(BUFFER_SIZE, sizeof(char*));
    message[i] = (char*) calloc(BUFFER_SIZE, sizeof(char));
    char* buffer = (char*) calloc(BUFFER_SIZE, sizeof(char));
    rev = read(fromWho[READ], buffer, BUFFER_SIZE);
    if (rev == -1) {

    } else if (rev == 0) {
        fflush(stdout);
        return NULL;
    }
    //fprintf(stderr,"buffer:%s",buffer);
    while (1) {
        if (strlen(buffer) == j) {

            if (flag == 0) {
                (*count)++;
            } else {
                (*count) = flag;
            }

            break;
        }
        if (buffer[j] == '\n') {/*new line*/
            flag++;
            j++;
            k = 0;
            i++;
            message[i] = (char*) calloc(BUFFER_SIZE, sizeof(char));
        } else {
            message[i][k] = buffer[j];
            k++;
            j++;
        }
    }
    free(buffer);
    return message;

}

/**
 * setup player
 * @param totalPlayer
 * @param argv
 * @return
 */
Player** player_setup(int totalPlayer, char** argv) {
    char letter[26] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    Player** tmp = (Player**) calloc(totalPlayer, sizeof(Player*));
    char files[26][100];
    for (int i = 0; i < totalPlayer; i++) {
        Player* p = (Player*) calloc(1, sizeof(Player));
        strcpy(files[i], argv[i + 4]);
        p->ID = i;
        p->indicator = letter[i];
        p->totalPoints = 0;
        p->name = files[i];
        p->fd[0] = 0;
        p->fd[1] = 0;
        tmp[i] = p;
        for (int i = 0; i < 5; i++) {
            p->tokens[i] = 0;
        }
    }
    return tmp;
}

/**
 * output hub info
 * @param what
 * @param who
 * @param data
 */
void hub_output(int what, char who, int data[6]) {
    char buffer[BUFFER_SIZE] = {'\0'};
    switch (what) {
        case PLAYER_PURCHASE:
            snprintf(buffer, BUFFER_SIZE,
                     "Player %c purchased %d using %d,%d,%d,%d,%d\n",
                     who,
                     data[0],
                     data[1],
                     data[2],
                     data[3],
                     data[4],
                     data[5]);
            break;
        case PLAYER_TAKE:
            snprintf(buffer, BUFFER_SIZE,
                     "Player %c drew %d,%d,%d,%d\n",
                     who,
                     data[0],
                     data[1],
                     data[2],
                     data[3]);
            break;
        case PLAYER_WILD:
            snprintf(buffer, BUFFER_SIZE, "Player %c took a wild\n", who);
            break;
        default:
            return;

    };
    fprintf(stdout, buffer);
    fflush(stdout);
}

/**
 * construct board using chain data struct
 * paramaters:cards , size: how many cards
 * return the board
 * @param deck
 * @return
 */
Cards* construct_cards_board(Cards** deck) {
    Cards* board = NULL;
    for (int i = 0; i < 8; i++) {
        add_element(&board, pop_first_one(deck));
        refresh_id(board);
    }

    return board;
}

/**
 * refill a card to board
 * @param hub
 */
void refill_a_card_and_send_message(Hub* hub) {
    Cards* card = pop_first_one(&(hub->deck));
    insert_element(hub->board, -1, card);
    char message[BUFFER_SIZE] = {'\0'};
    int* parentPipe, * childPipe;
    int totalPlayer = hub->totalPlayer;
    snprintf(message, BUFFER_SIZE,
             "newcard%c:%d:%d,%d,%d,%d",
             card->color,
             card->worth,
             card->price[0],
             card->price[1],
             card->price[2],
             card->price[3]);
    for (int i = 0; i < totalPlayer; i++) {
        parentPipe = hub->parentPipes[i];
        childPipe = hub->childPipes[i];
        send(parentPipe, message, childPipe);
    }
    memset(message, '\0', BUFFER_SIZE);
    snprintf(message, BUFFER_SIZE,
             "New card = Bonus %c, worth %d, costs %d,%d,%d,%d\n",
             card->color,
             card->worth,
             card->price[0],
             card->price[1],
             card->price[2],
             card->price[3]);
    fprintf(stdout, message);
    memset(message, '\0', BUFFER_SIZE);
}

/**
 * check wins
 * @param hub
 * @return
 */
int is_someone_reach_win(Hub* hub) {

    for (int i = 0; i < hub->totalPlayer; i++) {
        if (hub->players[i]->totalPoints >= hub->endPoint) {
            return 1;
        }
    }
    return 0;
}

/**
 * output eog
 * @param hub
 */
void hub_output_eog(Hub* hub) {
    char* message = (char*) calloc(BUFFER_SIZE, sizeof(char));
    char tmp = 'A';
    int privous = 0;
    strcat(message, "Winner(s) ");
    if (get_card_amount(hub->board) == 0) {
        for (int i = 0; i < hub->totalPlayer; i++) {
            if (hub->players[i]->totalPoints > privous) {
                privous = hub->players[i]->totalPoints;
                tmp = hub->players[i]->indicator;
            }
        }
        strcat(message, &tmp);
        fprintf(stdout, "%s", message);
        fprintf(stdout, "\n");
        return;
    }

    int index = 0;
    char winners[8] = {'\0'};
    for (int i = 0; i < hub->totalPlayer; i++) {
        if (hub->players[i]->totalPoints >= hub->endPoint) {
            winners[index++] = hub->players[i]->indicator;
        }
    }
    int num = strlen(winners);
    for (int i = 0; i < num; i++) {
        strcat(message, &winners[i]);
        strcat(message, ",");
    }
    int len = strlen(message);
    message[len - 1] = '\0';
    fprintf(stdout, message);
    fprintf(stdout, "\n");
    free(message);

}

/**
 * bad start check
 * @param args
 * @param argv
 */
void bad_start_check(int args, char** argv) {
    for (int i = 4; i < args; i++) {
        char* filename = get_file_name(argv[i]);
        if (strcmp(filename, "shenzi") == 0) {

        } else if (strcmp(filename, "banzai") == 0) {
        } else if (strcmp(filename, "ed") == 0) {

        } else {
            hub_error(BAD_START);
        }
        free(filename);
    }

}

/**
 * get file name
 * @param data
 * @return
 */
char* get_file_name(char* data) {
    char* tmp = (char*) calloc(BUFFER_SIZE, sizeof(char));
    int i = 0;
    int len = strlen(data);
    while (1) {
        if (data[i] != '/') {
            i++;
        } else {
            i++;
            break;
        }
        if (i >= len) {
            break;
        }
    }
    strcpy(tmp, data + i);
    return tmp;
}

