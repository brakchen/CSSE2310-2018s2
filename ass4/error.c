#include "error.h"

/*score err handle*/
void scores_err(int code) {
    switch (code) {
        case S_BAD_ARGS_CNT:
            fprintf(stderr, "Usage: gopher port");
            break;
        case S_BAD_CNCT:
            fprintf(stderr, "Failed to connect");
            break;
        case S_BAD_SERVER:
            fprintf(stderr, "Invalid server");
            break;
        default:
            exit(0);
            break;
    }
    fprintf(stderr, "\n");
    exit(code);
}

/*player err handle*/
void player_err(int code) {
    switch (code) {
        case P_BAD_ARGS_CNT:
            fprintf(stderr, "Usage: zazu keyfile port game pname");
            break;
        case P_BAD_FILE:
            fprintf(stderr, "Bad key file");
            break;
        case P_BAD_NAME:
            fprintf(stderr, "Bad name");
            break;
        case P_BAD_CNCT:
            fprintf(stderr, "Failed to connect");
            break;
        case P_BAD_AUTH:
            fprintf(stderr, "Bad auth");
            break;
        case P_BAD_ID:
            fprintf(stderr, "Bad reconnect id");
            break;
        case P_BAD_COMM:
            fprintf(stderr, "Communication Error");
            break;
        default:
            exit(0);
            break;
    }
    fprintf(stderr, "\n");
    exit(code);
}

/*other player err handle*/
void other_player_err(int code, char letter) {
    switch (code) {

        case OP_DISCNCT:
            fprintf(stderr, "Player %c disconnected", letter);
            break;
        case OP_BAD_MSG:
            fprintf(stderr, "Player %c sent invalid message", letter);
            break;
        default:
            exit(0);
            break;
    }
    fprintf(stderr, "\n");
    exit(code);
}

/*server err handle*/
void server_err(int code) {
    switch (code) {
        case BAD_ARGS_CNT:
            fprintf(stderr, "Usage: rafiki keyfile deckfile statfile timeout");
            break;
        case BAD_KEY_FILE:
            fprintf(stderr, "Bad keyfile");
            break;
        case BAD_STAT_FILE:
            fprintf(stderr, "Bad statfile");
            break;
        case BAD_DECK_FILE:
            fprintf(stderr, "Bad deckfile");
            break;
        case BAD_TIMEOUT:
            fprintf(stderr, "Bad timeout");
            break;
        case BAD_LISTEN:
            fprintf(stderr, "Failed listen");
            break;
        case SYS_ERROR:
            fprintf(stderr, "System error");
            break;
    }
    fprintf(stderr, "\n");
    exit(code);
}
