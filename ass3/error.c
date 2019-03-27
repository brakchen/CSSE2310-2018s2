#include <stdio.h>
#include <stdlib.h>
#include "error.h"

/**
 * playe error hanlder
 * @param p
 * @param who
 */
void player_error(ErrorPlayer p, char* who) {
    switch (p) {
        case ARGS_COUNT:
            fprintf(stderr, "Usage: %s pcount myid\n", who);
            exit(1);
        case BAD_PLAYER_COUNT:
            fprintf(stderr, "Invalid player count\n");
            exit(2);
        case BAD_ID:
            fprintf(stderr, "Invalid player ID\n");
            exit(3);
        case BAD_PIPE:/*pipe close before end of game or invalid message*/
            fprintf(stderr, "Communication Error\n");
            exit(6);
        default:
            exit(0);
    }
}

/**
 * hub error handler
 * @param h
 */
void hub_error(ErrorHub h) {
    switch (h) {
        case BAD_ARGS_COUNT:
            fprintf(stderr,
                    "Usage: austerity tokens points deck player player "
                    "[player ...]\n");
            exit(1);
        case BAD_ARGS:
            fprintf(stderr, "Bad argument\n");
            exit(2);
        case BAD_FILE:
            fprintf(stderr, "Cannot access deck file\n");
            exit(3);
        case BAD_CONTENT:
            fprintf(stderr, "Invalid deck file contents\n");
            exit(4);
        case BAD_START:
            fprintf(stderr, "Bad start\n");
            exit(5);
        case BAD_CONNECT:
            fprintf(stderr, "Client disconnected\n");
            exit(6);
        case BAD_PROTOCOL:
            fprintf(stderr, "Protocol error by client\n");
            exit(7);
        case BAD_SIGINT:
            fprintf(stderr, "SIGINT caught\n");
            exit(10);
        default:
            exit(0);

    }

}
