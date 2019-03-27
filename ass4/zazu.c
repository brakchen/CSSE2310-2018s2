#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ctype.h>
#include <util.h>
#include "zazu.h"

int main(int args, char** argv) {
    check_args(args, argv);
    game(args, argv);
    return 0;
}

/*main game*/
void game(int args, char** argv) {
    int conn = -1;
    build_connection(argv[2], &conn);
    int conn2 = dup(conn);
    FILE* fp[2] = {fdopen(conn, "r"), fdopen(conn2, "w")};
    enum PlayerErr pErr;
    if (strcmp(argv[3], "reconnect") == 0) {
        pErr = auth(fp, argv);
        if (pErr != P_NOTHING) {
            player_err(pErr);
        }
        reconnect_mode(argv, fp);
    } else {
        pErr = auth(fp, argv);
        if (pErr != P_NOTHING) {
            player_err(pErr);
        }
        normal_play_mode(args, argv, fp);
    }

}

/* function will handle all stuff related to
 * reconnect action,once a player try to reconnect
 * this function will be called
 * no return value
 * */
void reconnect_mode(char** argv, FILE* fp[2]) {
    char msg[TXT_SIZE];
    struct GameState gstate;
    struct Player player;
    gstate.players = &player;
    init_game_state(&gstate, argv);
    handshake(msg, &gstate, fp); /*parse playinfo*/
    /*main game logic*/
    while (fgets(msg, TXT_SIZE, fp[READ]) != NULL) {
        if (strstr(msg, "player") == msg) {
            if (handle_player_reconnect(&gstate, msg) == -1) {
                exit_with_free(P_BAD_COMM, &gstate, fp);
            }
        }
        start_game_logic(msg, &gstate, fp);
    }

}

/* the first time connection will call this
 * function to handle all things
 * no return value
 * */
void normal_play_mode(int args, char** argv, FILE* fp[2]) {
    char msg[TXT_SIZE];
    struct GameState gstate;
    struct Player player;
    gstate.players = &player;
    init_game_state(&gstate, argv);
    strcpy(gstate.players->name, argv[4]);/*player*/
    Reconnection* r = (Reconnection*) gstate.players->data;
    /*game Name*/
    strcpy(r->gameName, argv[3]);
    fgets(msg, TXT_SIZE, fp[READ]);
    remove_newline_char(msg);
    if (strstr(msg, "rid") == msg) {
        if (parse_rid(msg, (Reconnection*) gstate.players->data) == -1) {
            free(((Reconnection*) gstate.players->data)->gameName);
            exit_with_free(P_BAD_COMM, &gstate, fp);
        }
        fprintf(stdout, "%s\n", msg + 3);
    } else {/*error*/
        exit_with_free(P_BAD_COMM, &gstate, fp);
    }
    handshake(msg, &gstate, fp); /*parse playinfo*/

    for (int i = 0; i < gstate.playerCount; i++) {
        display_player_state(gstate.players[i]);

    }
    /*main game logic*/
    while (fgets(msg, TXT_SIZE, fp[READ]) != NULL) {
        if (strstr(msg, "player") == msg) {
            exit_with_free(P_BAD_COMM, &gstate, fp);
        }
        start_game_logic(msg, &gstate, fp);
    }

}

/* whatever the connection mode(new,reconnect)
 * this funciton will init a struct GameState
 * no return value
 * */
void init_game_state(struct GameState* gstate, char** argv) {
    struct Player* player = gstate->players;
    gstate->playerCount = 0;
    gstate->selfId = -1;
    memset(gstate->tokenCount, 0, TOKEN_MAX);
    gstate->boardSize = 0;
    Reconnection* rec = (Reconnection*) calloc(1, sizeof(Reconnection));
    player->name = (char*) calloc(strlen(argv[4]) + 1, sizeof(char));
    rec->gameName = (char*) calloc(strlen(argv[3]) + 1, sizeof(char));
    player->data = rec;
}

/* reconnect and new connect both will recive
 * "playinfo" from server so it's pretty much like
 * handshake.this funciton also handle a few error
 * situations,if the message is not playinfo will raise
 * error,if the message is keyword matched but invalid 
 * will raise error.no return value
 * */
void handshake(char* msg, struct GameState* gstate, FILE* fp[2]) {
    fgets(msg, TXT_SIZE, fp[READ]);
    remove_newline_char(msg);
    if (strstr(msg, "playinfo") == msg) {
        if (handle_play_info(gstate, msg) == -1) {
            exit_with_free(P_BAD_COMM, gstate, fp);
        }
    } else {/*error*/
        exit_with_free(P_BAD_COMM, gstate, fp);
    }
}

/* after auth and handshake,
 * the play will go to main game logic
 * whatever new or reconenct player
 * this process is same
 * no return value
 * also this function handle so error.
 * but most of the error comes from invalid mesage
 * so it will raise communication err
 * */
void start_game_logic(char* msg, struct GameState* gstate, FILE* fp[2]) {
    enum ErrorCode err = NOTHING_WRONG;
    remove_newline_char(msg);
    enum MessageFromHub hubMsg = classify_from_hub(msg);

    switch (hubMsg) {
        case DISCO:
            exit_with_free(P_NOTHING, gstate, fp);
            err = NOTHING_WRONG;
            break;
        case INVALID:
            exit_with_free(P_NOTHING, gstate, fp);
            err = NOTHING_WRONG;
            break;
        case END_OF_GAME:
            display_eog_info(gstate);
            exit_with_free(P_NOTHING, gstate, fp);
            err = NOTHING_WRONG;
            break;
        default:
            err = process_hub_msg(msg, gstate, fp);
    }
    if (err == COMMUNICATION_ERROR) {
        fclose(fp[WRITE]);
        fclose(fp[READ]);
        free_game_state(gstate);
        player_err(P_BAD_COMM);
    }
}

/* this is help function that handle
 * exit status will free,this function
 * SHOULD NOT appear in any other functions
 * except normal_play mode and reconnect_mode
 * no return value
 * */
void exit_with_free(enum PlayerErr err,
        struct GameState* gstate, FILE* fp[2]) {
    fclose(fp[WRITE]);
    fclose(fp[READ]);
    if (err != P_NOTHING) {
        free_game_state_err(gstate);
    } else {
        free_game_state(gstate);

    }
    player_err(err);

}

/* ass4 add some new msgs so this function is only used to
 * handle msgs from ass3,ass4 message will be handle in 
 * other function but there is a free error status will 
 * handle in start_game_logic becasue there are a few
 * situation need to free some of them.
 * return the enum ErrorCode
 * */
enum ErrorCode process_hub_msg(const char* msg,
        struct GameState* gstate, FILE* fp[2]) {
    enum MessageFromHub hubMsg = classify_from_hub(msg);
    enum ErrorCode err = NOTHING_WRONG;
    char* sig = NULL;
    switch (hubMsg) {
        case DO_WHAT:
            fprintf(stdout, "Received dowhat\n");
            fflush(stdout);
            sig = prompt(gstate);
            fprintf(fp[WRITE], "%s", sig);
            fflush(fp[WRITE]);
            free(sig);
            err = NOTHING_WRONG;
            break;
        case PURCHASED:
            err = handle_purchased_message(gstate, msg);
            display_turn_info(gstate);
            break;
        case TOOK:
            err = handle_took_message(gstate, msg);
            display_turn_info(gstate);
            break;
        case TOOK_WILD:
            err = handle_took_wild_message(gstate, msg);
            display_turn_info(gstate);
            break;
        case NEW_CARD:
            err = handle_new_card_message(gstate, msg);
            display_turn_info(gstate);
            break;
        case TOKENS:
            for (int i = 0; i < TOKEN_MAX; i++) {
                gstate->tokenCount[i] = atoi((msg + 6));
            }
            display_turn_info(gstate);
            break;
        default:
            return COMMUNICATION_ERROR;
    }
    return err;

}

/* this function will handle playinfo message
 * return -1 if message invalid otherwise return 0
 * */
int handle_play_info(struct GameState* gstate, char* msg) {
    struct Player* player;
    int rev = 0;
    int output[2] = {0};
    rev = parse_play_info(msg, output);
    if (rev == -1) {
        return rev;
    }
    gstate->selfId = output[0];
    gstate->playerCount = output[1];
    player = gstate->players;
    gstate->players = (struct Player*) calloc(output[1],
            sizeof(struct Player));
    for (int i = 0; i < gstate->playerCount; i++) {
        struct Player p;
        initialize_player(&p, i);
        gstate->players[i] = p;
    }
    gstate->players[gstate->selfId].name = player->name;
    for (int i = 0; i < TOKEN_MAX; i++) {
        if (i < 4) {
            gstate->players[gstate->selfId].discounts[i] = 0;
        }
        gstate->players[gstate->selfId].tokens[i] = 0;
    }
    gstate->players[gstate->selfId].data = player->data;
    return 0;

}

/* fun name.function will handle read human input
 * and remove a newline char to a give array pointer
 * no return value
 * */
void talkok(char buffer[BUF_SIZE], char* action) {
    fgets(buffer, BUF_SIZE, stdin);
    remove_newline_char(buffer);
}

/* this is main function that handle human
 * prompt actions,all human input will process here
 * return char* ,dont forgot to FREE
 * */
char* prompt(struct GameState* gstate) {
    struct PurchaseMessage purchaseMsg = {.cardNumber = 0};
    char buffer[BUF_SIZE];
    char tmp[20] = {'\0'};
    for (; ; ) {
        memset(buffer, '\0', BUF_SIZE);
        talkok(buffer, "Action>");
        fprintf(stdout, "Action> ");
        fflush(stdout);
        if (strcmp(buffer, "purchase") == 0) {
            prompt_purchase_dialog(&purchaseMsg, *gstate);
            return print_purchase_message(purchaseMsg);
        } else if (strcmp(buffer, "take") == 0) {
            struct TakeMessage takeMsg;
            char color[4] = "PBYR";
            for (int i = 0; i < 4; ) {
                memset(buffer, '\0', BUF_SIZE);
                sprintf(tmp, "Token-%c>", color[i]);
                fprintf(stdout, "Token-%c> ", color[i]);
                fflush(stdout);
                talkok(buffer, tmp);
                if (!is_all_numbers(buffer)) {
                    continue;
                }
                if (atoi(buffer) < 0 || gstate->tokenCount[i] < atoi(buffer)) {
                    continue;
                }
                takeMsg.tokens[i] = atoi(buffer);
                i++;
            }
            return print_take_message(takeMsg);
        } else if (strcmp(buffer, "wild") == 0) {
            char* msg = (char*) calloc(6, sizeof(char));
            sprintf(msg, "%s\n", "wild");
            return msg;
        } else {
            continue;
        }
    }
}

/*prompt's help function that prompt the purchase dialog
 * no return value
 * */
void prompt_purchase_dialog(struct PurchaseMessage* purchaseMsg,
        const struct GameState gstate) {
    char buffer[BUF_SIZE];
    for (; ; ) {
        memset(buffer, '\0', BUF_SIZE);
        talkok(buffer, "Card>");
        fprintf(stdout, "Card> ");
        fflush(stdout);
        if (!is_all_numbers(buffer)) {
            continue;
        }
        int cardNum = atoi(buffer);
        if (cardNum < 0 || cardNum > 7) {
            continue;
        } else {
            purchaseMsg->cardNumber = cardNum;
            break;
        }
    }
    prompt_purchase_token(purchaseMsg, gstate);
}

/*help function of promt purchase dialog*/
void prompt_purchase_token(struct PurchaseMessage* purchaseMsg,
        const struct GameState gstate) {
    struct Player this = gstate.players[gstate.selfId];
    char buffer[BUF_SIZE];
    char tmp[20] = {'\0'};
    char color[4] = "PBYR";
    for (int i = 0; i < 4; ) {
        memset(buffer, '\0', BUF_SIZE);
        if (this.tokens[i] > 0) {
            sprintf(tmp, "Token-%c>", color[i]);
            fprintf(stdout, "Token-%c> ", color[i]);
            fflush(stdout);
            talkok(buffer, tmp);
            if (!is_all_numbers(buffer)) {
                continue;
            }
            if (atoi(buffer) < 0) {
                continue;
            }
            purchaseMsg->costSpent[i] = atoi(buffer);
            i++;
        } else {
            i++;
            continue;
        }
    }
    for (; ; ) {
        if (this.tokens[4] > 0) {
            memset(buffer, '\0', BUF_SIZE);
            sprintf(tmp, "Token-%c>", 'W');
            fprintf(stdout, "Token-W> ");
            fflush(stdout);
            talkok(buffer, tmp);
            if (!is_all_numbers(buffer)) {
                continue;
            }
            purchaseMsg->costSpent[4] = atoi(buffer);
        }
        break;
    }


}

/* this is function that parse rid msg
 * the game name , game counter ,player id are included in msg
 * this funciton will split them and put int struct Reconnection
 * the Reconnection assgin to player->data
 * return -1 if msg invalid;otherwise 0
 * so some error like gamename error will raise if reach
 * */
int parse_rid(const char* msg, Reconnection* output) {
    char input[TXT_SIZE];
    char gameName[TXT_SIZE];
    int playerId = -1;
    memset(input, '\0', TXT_SIZE);
    memset(gameName, '\0', TXT_SIZE);
    strcpy(input, msg);
    char value[3][TXT_SIZE];/*game name,game counter,player id*/
    for (int i = 0; i < 3; i++) {
        memset(value[i], '\0', TXT_SIZE);
    }
    char* result = strtok(input, ",");/*split with ,*/
    for (int i = 0; result != NULL; i++) {
        strcpy(value[i], result);
        result = strtok(NULL, ",");
    }

    /*data validation*/
    strcpy(gameName, value[0] + 3);
    if (strcmp(gameName, "") == 0) {
        return -1;
    }
    if (!is_all_numbers(value[1])) {/*error*/
        return -1;
    }
    if (is_all_numbers(value[2])) {
        playerId = atoi(value[2]);
        if (playerId > 25) {/*out of range*/
            return -1;
        }
    } else {/*error */
        return -1;
    }
    if (strcmp(output->gameName, gameName) != 0) {
        return -1;
    }
    output->gameCounter = atoi(value[1]);
    output->playerId = playerId;
    return 0;

}

/* this is a function parse player info
 * also this is a handle_play_info help function
 * in this function will assign value to output
 * reutrn -1 if the msg invalid otherwise 0
 * */
int parse_play_info(const char* msg, int output[2]) {
    char amount[TXT_SIZE];
    char letter = msg[8];
    int id = letter_to_id(letter);
    memset(amount, '\0', TXT_SIZE);
    strcpy(amount, (msg + 10));
    //validation
    if (!is_all_numbers(amount)) {
        //exit with invalid message
        return -1;
    }
    if (id == -1) {
        return -1;
        //exit with invalid message
    }
    int amountPlayer = atoi(amount);
    if (amountPlayer < 2 || amountPlayer > 26) {
        //exit with out of range
        return -1;
    }
    //id==selfId
    if (id > (amountPlayer - 1)) {
        /*invalid id*/
        return -1;
    }
    output[0] = id;
    output[1] = atoi(amount);
    return 0;
}

/* function will handle player reconnection
 * return -1 means msg invalid otherwise 0
 * */
int handle_player_reconnect(struct GameState* gstate, char* msg) {
    PlayerMessage pMsg;
    memset(&pMsg, 0, sizeof(PlayerMessage));
    /*parse "player" msg*/
    if (parse_reconnect_info(msg, &pMsg) == -1) {
        return -1;
    }
    int id = letter_to_id(pMsg.letter);
    gstate->players[id].score = pMsg.points;
    for (int i = 0; i < TOKEN_MAX; i++) {
        if (i < 4) {
            gstate->players[id].discounts[i] = pMsg.discount[i];
        }
        gstate->players[id].tokens[i] = pMsg.tokens[i];
    }
    return 0;
}

/* function to parse "player" msg
 * return -1 means msg invalid otherwise 0
 * */
int parse_reconnect_info(const char* msg, PlayerMessage* pMsg) {
    char value[TXT_SIZE];
    char totalPoints[TXT_SIZE];
    memset(value, '\0', TXT_SIZE);
    memset(totalPoints, '\0', TXT_SIZE);
    strcpy(value, (msg + 6));/*P:TP:d.....*/
    pMsg->letter = value[0];
    strcpy(value, (value + 2));/*TP:d.....*/
    int tmp = first_comma_pos(value);
    strncpy(totalPoints, value, tmp);/*totalPoint catcher*/
    if (is_all_numbers(totalPoints)) {
        pMsg->points = atoi(totalPoints);
    } else {/*error*/
        return -1;
    }
    strcpy(value, value + tmp + 3);/*1,2,3,4,:t=....*/
    char input[TXT_SIZE];
    memset(input, '\0', TXT_SIZE);
    strncpy(input, value, 7);/*format:1,2,3,4*/
    char* output = strtok(input, ",");/*split with ,*/
    for (int i = 0; output != NULL; i++) {
        pMsg->discount[i] = atoi(output);
        output = strtok(NULL, ",");
    }
    tmp = first_comma_pos(value);
    strcpy(value, value + tmp + 3);
    output = strtok(value, ",");/*split with ,*/
    for (int i = 0; output != NULL; i++) {
        pMsg->tokens[i] = atoi(output);
        output = strtok(NULL, ",");
    }
    return 0;
}

/*this is help function of  parse_reconnect_info
 *return the first meet comma position
 * */
int first_comma_pos(const char* value) {
    int tmp = 0;
    for (int i = 1; i < strlen(value); i++) {
        if (value[i] == ':') {
            tmp = i;
            break;/*only need to meet the first common*/
        }
    }
    return tmp;

}

/* connect a addr via a given port
 * not return value
 * */
void build_connection(const char* port, int* conn) {
    struct addrinfo hints, * res, * result;
    int sock = 1;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if (getaddrinfo("localhost", port, &hints, &result)) {
        player_err(P_BAD_CNCT);
    }
    for (res = result; res != NULL; res = result->ai_next) {
        sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sock == -1) {
            freeaddrinfo(result);
            player_err(P_BAD_CNCT);
            continue;
        }
        if (connect(sock, res->ai_addr, res->ai_addrlen) == -1) {
            freeaddrinfo(result);
            player_err(P_BAD_CNCT);
            continue;
        }
        break;/*one socket builded*/
    }
    *conn = sock;
    freeaddrinfo(result);
}

/* recieve the "yes" msg from server
 * return enum PlayerErrs
 * those error code will be catch
 * at the game function to handle
 * auth problem
 * */
enum PlayerErr auth(FILE* fp[2], char** argv) {
    char msg[TXT_SIZE];
    if (strcmp(argv[3], "reconnect") == 0) {
        read_key_file(msg, argv[1]);
        /*reconnect+key*/
        fprintf(fp[WRITE], "%s%s\n", argv[3], msg);
        fflush(fp[WRITE]);
        fgets(msg, TXT_SIZE, fp[READ]);
        if (strstr(msg, "yes") != msg) {
            return P_BAD_AUTH;
        }
        /*send rid*/
        fprintf(fp[WRITE], "%s%s\n", "rid", argv[4]);
        fflush(fp[WRITE]);
        fgets(msg, TXT_SIZE, fp[READ]);
        if (strstr(msg, "no") == msg) {
            return P_BAD_ID;
        }

    } else {
        read_key_file(msg, argv[1]);
        /*player+key*/
        fprintf(fp[WRITE], "%s%s\n", "play", msg);
        fflush(fp[WRITE]);
        fgets(msg, TXT_SIZE, fp[READ]);
        remove_newline_char(msg);
        if (strstr(msg, "yes") != msg) {
            return P_BAD_AUTH;
        }
        fprintf(fp[WRITE], "%s\n", argv[3]);/*gameName*/
        fflush(fp[WRITE]);
        fprintf(fp[WRITE], "%s\n", argv[4]);/*playerName*/
        fflush(fp[WRITE]);
    }
    return P_NOTHING;
}

/* player args check
 * no return value
 * */
void check_args(int args, char** argv) {
    if (args != 5) {
        player_err(P_BAD_ARGS_CNT);
    }
    char keyFile[TXT_SIZE], port[TXT_SIZE], gameName[TXT_SIZE];
    char playerName[TXT_SIZE];
    strcpy(keyFile, argv[1]), strcpy(port, argv[2]);
    FILE* fp = fopen(keyFile, "r");
    if(fp == NULL) {
        player_err(P_BAD_FILE);
    } else {
        fclose(fp);
    }
    if (!is_all_numbers(port)) {
        player_err(P_BAD_CNCT);
    } else {
        if (atoi(port) < 1 || atoi(port) > 65535) { 
            player_err(P_BAD_CNCT); 
        }
    }
    if (strcmp(argv[3], "reconnect") != 0) {/*normal play*/
        strcpy(gameName, argv[3]), strcpy(playerName, argv[4]);
        if (strcmp(gameName, "") == 0 || strcmp(playerName, "") == 0) {
            player_err(P_BAD_NAME);
        }
        if (has_newline_char(gameName) || has_newline_char(playerName)) {
            player_err(P_BAD_NAME);
        }
        if (has_comma(gameName) || has_comma(playerName)) {
            player_err(P_BAD_NAME);
        }
    } else {
        player_name_check(argv[4]);
    }
}

/*check player name*/
void player_name_check(char* name) {
    char rid[TXT_SIZE];
    strcpy(rid, name);
    char* output = strtok(rid, ",");/*split with ,*/
    int i = 0;
    for (; output != NULL; i++) {
        if (i == 0) {
            if (has_newline_char(output)) {
                player_err(P_BAD_ID);
            }
            if (has_comma(output)) {
                player_err(P_BAD_ID);
            }
            if (strcmp(output, "") == 0) {
                player_err(P_BAD_ID);
            }
        } else {
            if (!is_all_numbers(output)) {
                player_err(P_BAD_ID);
            }
        }
        output = strtok(NULL, ",");
    }
    if (i < 3) {
        player_err(P_BAD_ID);
    }


}

/* once player into main game logic
 * function will be use to free struct
 * gameState
 * no return value
 * */
void free_game_state(struct GameState* gstate) {
    free(gstate->players[gstate->selfId].name);
    int selfId = gstate->selfId;
    Reconnection* r = (Reconnection*) gstate->players[selfId].data;
    free(r->gameName);
    free(r);
    free(gstate->players);
}

/* error before into main game logic
 * function will handle free some calloc
 * no return value
 * */
void free_game_state_err(struct GameState* gstate) {
    Reconnection* r = (Reconnection*) gstate->players->data;
    free(r->gameName);
    free(r);
    free(gstate->players->name);
}

/* use to read the key file
 * output
 *
 * */
void read_key_file(char* output, char* fileName) {
    FILE* fp = fopen(fileName, "r");
    char result[TXT_SIZE];
    memset(result, '\0', TXT_SIZE);
    int lines = 1;
    while (fgets(result, TXT_SIZE, fp) != NULL) {
        lines++;
    }
    if (lines > 2) {/*error*/
        output = NULL;
        player_err(P_BAD_FILE);
    }
    if (strcmp(result, "") == 0) {/*error*/
        output = NULL;
        player_err(P_BAD_FILE);
    }
    int len = strlen(result);
    if (result[len - 1] == '\n') {/*error*/
        output = NULL;
        player_err(P_BAD_FILE);
    }
    strcpy(output, result);
    fclose(fp);

}
