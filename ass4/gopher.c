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


#include "gopher.h"

int main(int args, char** argv) {
    check_args(args, argv);
    int conn = 0;
    build_connection(argv[1], &conn);
    int conn2 = dup(conn);
    FILE* fp[2] = {fdopen(conn, "r"), fdopen(conn2, "w")};
    char rev[TXT_SIZE];
    fprintf(fp[WRITE], "%s", "scores\n");
    fflush(fp[WRITE]);
    fgets(rev, TXT_SIZE, fp[READ]);
    if (auth(rev) == -1) {/*auth check*/
        fclose(fp[READ]);
        fclose(fp[WRITE]);
        scores_err(S_BAD_SERVER);

    }
    score(args, argv, fp);
    fclose(fp[READ]);
    fclose(fp[WRITE]);
    return 0;
}

/* main logic will apply in score
 * the mian function will handle the auth things
 * no return value
 * */
void score(int args, char** argv, FILE* fp[2]) {
    char tmp[BUF_SIZE];
    memset(tmp, '\0', BUF_SIZE);
    Message msg = {.text = tmp, .size = 0};
    //return message check
    while (fgets(msg.text, BUF_SIZE, fp[READ]) != NULL) {
        fprintf(stdout, "%s", msg.text);
        fflush(stdout);
    }
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
        scores_err(S_BAD_CNCT);
    }
    for (res = result; res != NULL; res = result->ai_next) {
        sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sock == -1) {
            freeaddrinfo(result);
            scores_err(S_BAD_CNCT);
            continue;
        }
        if (connect(sock, res->ai_addr, res->ai_addrlen) == -1) {
            freeaddrinfo(result);
            scores_err(S_BAD_CNCT);
            continue;
        }
        break;/*one socket builded*/
    }
    *conn = sock;
    freeaddrinfo(result);
}

/* check if player exist in list
 * return 1 if yes ,-1 if no
 *
 * */
int exist(Score* scoreBoard, Message msg) {
    int index = 0;
    int comma[3] = {0};
    char tmp[1000];
    memset(tmp, '\0', 1000);
    int i = 0;
    do {
        if (msg.text[i] == ',') {
            comma[index++] = i;
        }
        i++;
    } while (msg.text[i] != '\0');
    strncpy(tmp, msg.text, comma[0]);
    for (int i = 0; i < msg.size; i++) {
        if (strcmp(scoreBoard[i].player, tmp) == 0) {
            return i;
        }
    }
    return -1;

}

/* check args no return value
 *
 * */
void check_args(int args, char** argv) {
    if (args != 2) {
        scores_err(S_BAD_ARGS_CNT);
    }
    if (!is_all_numbers(argv[1])) {
        scores_err(S_BAD_CNCT);
    }
    int port = atoi(argv[1]);
    if (port < 1 || port > 65535) {

        scores_err(S_BAD_CNCT);
    }
}

/* recieve the "yes" msg from server
 * return 1 means right message,
 * 0 means apart from yes
 *
 * */
int auth(const char* msg) {
    int len = strlen(msg);
    char tmp[len + 1];
    memset(tmp, '\0', len + 1);
    if (msg[len - 1] == '\n') {
        strcpy(tmp, msg);
        tmp[len - 1] = '\0';
    } else {
        strcpy(tmp, msg);
    }
    if (strcmp(tmp, "yes") != 0) {
        /*any msg apart from yes treat as invalid msg*/
        //scores_err(S_BAD_SERVER);
        return -1;
    }
    return 0;
}
