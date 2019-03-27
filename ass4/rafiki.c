
#include "rafiki.h"

/*golbal var use to catch signal*/
static int sigCatch = SIGNAL_NORMAL;

int main(int args, char** argv) {
    args_validation(args, argv);
    struct sigaction action = {
        .sa_handler = sig_handler,
        .sa_flags = 0
    };
    sigemptyset(&action.sa_mask);
    sigaddset(&action.sa_mask, SIGINT);
    sigaddset(&action.sa_mask, SIGTERM);
    sigaction(SIGTERM, &action, 0);
    sigaction(SIGINT, &action, 0);
    server(args, argv);
    return 0;


}

/* handle signal
 * no return value
 * */
void sig_handler(int signo) {
    if (signo == SIGINT) {
        sigCatch = SIGNAL_INT;
    } else if (signo == SIGTERM) {
        sigCatch = SIGNAL_TERM;
    }
}

/*setup socketpool*/
void server_setup_socketpool(Middleware* m, char* fileName) {
    StatFile* stats = NULL;
    int lines = read_stat(&stats, fileName);
    pthread_t tid;/*thread pid*/
    m->sp = (SocketPool*) calloc(lines, sizeof(SocketPool));
    for (int i = 0; i < lines; i++) {
        m->sp[i].tokens = 0;
        m->sp[i].points = 0;
        m->sp[i].players = 0;
        m->sp[i].port = 0;
        m->sp[i].sock = 0;

    }
    if (lines == -1) {
        server_err(BAD_STAT_FILE);
    } else {/*read stat file ok*/
        swap_stat_to_socket_pool(m->sp, stats, lines);
        for (int i = 0; i < lines; i++) {
            build_listen(&(m->sp[i]));/*build listen*/
        }
        for (int i = 0; i < lines; i++) {
            pthread_create(&tid, NULL, pthread_wait_connection, m);
            pthread_detach(tid);
        }
        output_port(m->sp, lines);/*output port with format*/
    }
    free(stats);
}

/*free the stuff when catch sig*/
void sig_catch_free(Middleware m, struct Game* game, int gameCount) {
    if (m.sp != NULL) {
        free(m.sp);
    }
    for (int i = 0; i < gameCount; i++) {
        struct Game g = game[i];
        free(g.name);/*free gamename*/
        for (int j = 0; j < g.playerCount; j++) {
            struct GamePlayer p = game[i].players[j];
            free(p.state.name);/*free player name*/
        }
        free(g.players);/*free this game players*/
        free(g.data);/*free game's data*/
    }
    free(game);


}

/*help function that use to handle sig in server function*/
int server_handle_sig(Middleware* m, char* fileName, int gameCount,
        struct Game* game) {
    if (sigCatch == SIGNAL_INT) {
        free(m->sp);
        (*m).spPos = 0;
        server_setup_socketpool(m, fileName);
        sigCatch = SIGNAL_NORMAL;/*set singal back*/
        return 1;
    }
    if (sigCatch == SIGNAL_TERM) {
        pthread_mutex_unlock(&(m->resource));
        if (game == NULL) {
            if (m->sp != NULL) {
                free(m->sp);
            }
        } else {
            sig_catch_free(*m, game, gameCount);
        }
        exit(0);
    }
    return 0;
}

/*
 * main function
 * */
void server(int args, char** argv) {
    //after validation data is safe
    Middleware m;
    init_middleware(&m, argv[1]);
    pthread_t signalVar;
    server_setup_socketpool(&m, argv[3]);
    int connectionLen = -1;
    int gameCount = 0;
    struct Game* game = NULL;
    int timeout = atoi(argv[4]);
    int lines = lines_count(argv[3]);
    SigData sigData = {
        .sp = m.sp,
                .size = lines
    };
    pthread_create(&signalVar, NULL, pthread_handle_signal, &sigData);
    pthread_detach(signalVar);
    while (1) {
        if (server_handle_sig(&m, argv[3], gameCount, game)) {
            continue;
        }
        connectionLen = strlen(m.connectionMode);
        if (connectionLen > 0) {/*make sure size not less than 0*/
            pthread_mutex_lock(&m.resource);
            if (strstr(m.connectionMode, "reconnect") == m.connectionMode) {
                handle_reconnection(&game, gameCount, &m);
            } else if (strstr(m.connectionMode, "score") == m.connectionMode) {
                /*this score modew toPlayer denote to toScore*/
                handle_score_client(game, gameCount, m.fd);

            } else if (strstr(m.connectionMode, "play") == m.connectionMode) {
                handle_play_connection(&m, &game, m.sp, lines, &gameCount);
            }
            if (game != NULL) {
                resolve_start_new_game(game, gameCount, timeout, argv[2]);
            }
            wipe_middleware(&m);
            pthread_mutex_unlock(&m.resource);
            // fprintf(stderr,"main mutex unlock len:%d\n",strlen(m.gameName));
        }
    }
    exit(0);
}

/* pthread use to listen signal only
 * */
void* pthread_handle_signal(void* args) {
    SigData* sigData = (SigData*) args;
    while (sigCatch == 0) { 
        ;
    }
    if (sigCatch == SIGNAL_INT || sigCatch == SIGNAL_TERM) {
        for (int i = 0; i < sigData->size; i++) {
            shutdown(sigData->sp[i].sock, SHUT_RDWR);
        }
    }
    return NULL;

}

/*signal catch in pthread*/
int pthread_func_catch_signal(Middleware* m) {
    pthread_mutex_lock(&m->mutex);
    pthread_mutex_lock(&m->resource);
    if (sigCatch == SIGNAL_INT || sigCatch == SIGNAL_TERM) {
        pthread_mutex_unlock(&(m->mutex));
        pthread_mutex_unlock(&m->resource);
        return -1;
    }
    return 0;
}


/* pthread use to listen all come in connection
 * and write the data into middleware
 * */
void* pthread_wait_connection(void* args) {
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    Middleware* m = (Middleware*) args;
    pthread_mutex_lock(&m->mutex);
    SocketPool sp = m->sp[m->spPos];
    m->spPos++;/*should increase 1 for every thread*/
    pthread_mutex_unlock(&m->mutex);
    struct sockaddr_in ss;
    socklen_t sslen = sizeof(ss);
    int fd = 1;
    char buffer[BUF_SIZE];
    int connectionLen = -1;
    memset(buffer, '\0', BUF_SIZE);
    while (1) {
        fd = accept(sp.sock, (struct sockaddr*) &ss, &sslen);
        if (fd < 0) {
            break;
        }
        if (pthread_func_catch_signal(m) == -1) {
            pthread_exit(NULL);
        }
        if (pthread_func_handshake(fd, m, buffer) == -1) {
            continue;
        }
        pthread_mutex_unlock(&m->resource);
        for (; ; ) {
            connectionLen = strlen(m->connectionMode);
            if (connectionLen == 0) {
                /*game thread already get info
                * so realse the mutex*/
                pthread_mutex_unlock(&(m->mutex));
                break;
            }
        }
    }
    /* invalid fd*/
    return NULL;
}

/* pthread_wait_connection help function
 * this funciton use to deal with handshakre stuff
 * catch the msg form player and send yes or no
 * handle reconnection,player and score client's handshake
 * */
int pthread_func_handshake(int fd, Middleware* m, char buffer[TXT_SIZE]) {
    FILE* fp[2];
    fp[READ] = fdopen(fd, "r");
    int fd2 = dup(fd);
    fp[WRITE] = fdopen(fd2, "w");
    memset(buffer, '\0', BUF_SIZE);
    fgets(buffer, BUF_SIZE, fp[READ]);
    remove_newline_char(buffer);
    if (strstr(buffer, "play") == buffer) {/*handshake*/
        strcpy(m->connectionMode, buffer);
        if (check_key(buffer, 4, fp, m->key) == -1) {
            return -1;
        }
        pthread_func_setup_new_player(fp, fd, m, buffer);
    } else if (strstr(buffer, "score") == buffer) {
        /*TODO :if no game running*/
        strcpy(m->connectionMode, buffer);
        fprintf(fp[WRITE], "%s\n", "yes");
        fflush(fp[WRITE]);
    } else if (strstr(buffer, "reconnect")) {
        strcpy(m->connectionMode, buffer);
        if (check_key(buffer, 9, fp, m->key) == -1) {
            return -1;
        }
        pthread_func_setup_reconnect_player(fp, fd, m, buffer);
    } else {
        fprintf(fp[WRITE], "%s\n", "no");
        fflush(fp[WRITE]);
        return -1;
    }
    return 0;

}

/*check user key message*/
int check_key(char* buffer, int offset, FILE* fp[2], char* key) {
    char* keyString = strdup(buffer + offset);
    if (strcmp(keyString, key) == 0) {
        fprintf(fp[WRITE], "%s\n", "yes");
        fflush(fp[WRITE]);
    } else {
        fprintf(fp[WRITE], "%s\n", "no");
        fflush(fp[WRITE]);
        return -1;
    }
    free(keyString);
    return 0;
}

/*handle reconnect player*/
void pthread_func_setup_reconnect_player(FILE* fp[2], int fd,
        Middleware* m, char buffer[TXT_SIZE]) {
    remove_newline_char(buffer);
    memset(buffer, '\0', BUF_SIZE);
    fgets(buffer, BUF_SIZE, fp[READ]);/*gamename from client*/
    strcpy(m->gameName, buffer);/*set game name*/
    m->gamePlayer.fileDescriptor = fd;
    m->gamePlayer.toPlayer = fp[WRITE];
    m->gamePlayer.fromPlayer = fp[READ];
}

/* pthread wait connection
 * this function use to copy game name to middlware
 * set up player array with in game array
 * and copy player's name into player aray
 * no return value
 * */
void pthread_func_setup_new_player(FILE* fp[2], int fd, Middleware* m,
        char buffer[TXT_SIZE]) {
    remove_newline_char(buffer);
    memset(buffer, '\0', BUF_SIZE);
    fgets(buffer, BUF_SIZE, fp[READ]);/*gamename from client*/
    strcpy(m->gameName, buffer);/*set game name*/
    memset(buffer, '\0', BUF_SIZE);
    fgets(buffer, BUF_SIZE, fp[READ]);/*playername from client*/
    int len = strlen(buffer) + 1;
    m->gamePlayer.state.name = (char*) calloc(len, sizeof(char));
    strcpy(m->gamePlayer.state.name, buffer);/*set palyer name*/
    m->gamePlayer.fileDescriptor = fd;
    m->gamePlayer.toPlayer = fp[WRITE];
    m->gamePlayer.fromPlayer = fp[READ];
    memset(buffer, '\0', BUF_SIZE);
}

/* handle_play_connection help funciton
 * create a new game obj when game is null
 * */
void create_new_game_obj(Middleware* m, struct Game** game,
        SocketPool* sp, int lines, int* gameCount) {
    int index = *gameCount;
    struct sockaddr_in conn;
    socklen_t connLen = sizeof(conn);
    int playerConnectPort = 0;
    *game = (struct Game*) calloc(1, sizeof(struct Game));
    init_game_struct(&((*game)[index]));/*init*/
    getsockname(m->gamePlayer.fileDescriptor,
            (struct sockaddr*) &conn, &connLen);
    playerConnectPort = ntohs(conn.sin_port);/*player connected port*/
    for (int i = 0; i < lines; i++) {
        if (sp[i].port == playerConnectPort) {/*take info from port*/
            (*game)[index].winScore = sp[i].points;
            for (int j = 0; j < TOKEN_MAX - 1; j++) {
                ((*game)[index]).tokenCount[j] = sp[i].tokens;
            }
            (*game)[index].players = (struct GamePlayer*) calloc(
                    sp[i].players, sizeof(struct GamePlayer));
            (*game)[index].playerCount = sp[i].players;
            break;
        }
    }
    int len = strlen(m->gameName) + 1;
    (*game)[index].name = (char*) calloc(len, sizeof(char));/*gameName*/
    strcpy((*game)[index].name, m->gameName);
    (*game)[index].players[0].fileDescriptor = m->gamePlayer.fileDescriptor;
    (*game)[index].players[0].toPlayer = m->gamePlayer.toPlayer;
    (*game)[index].players[0].fromPlayer = m->gamePlayer.fromPlayer;
    (*game)[index].players[0].state.name = m->gamePlayer.state.name;
    m->gamePlayer.state.name = NULL;
    (((GameCounter*) ((*game)[index].data))->currentPlayer)++;/*game data*/
    (*gameCount)++;/*gamecounter ++*/
}

/* handle connection help function
 * will realloc space for two suitaion
 * 1.game overflow
 * 2.no game exist but game is not nulll
 * no return value
 * */
void realloc_new_game_obj(Middleware* m, struct Game** game,
        SocketPool* sp, int lines, int* gameCount) {
    int index = *gameCount;
    struct sockaddr_in conn;
    socklen_t connLen = sizeof(conn);
    int playerConnectPort = 0;
    if (realloc(game, (*gameCount + 1) * sizeof(struct Game)) == NULL) {
        /*should raise system error*/
        server_err(SYS_ERROR);
    }
    init_game_struct(&((*game)[index]));/*init*/
    getsockname(m->gamePlayer.fileDescriptor,
            (struct sockaddr*) &conn, &connLen);
    playerConnectPort = ntohs(conn.sin_port);
    for (int i = 0; i < lines; i++) {
        if (sp[i].port == playerConnectPort) {
            (*game)[index].winScore = sp[i].points;
            for (int j = 0; j < TOKEN_MAX - 1; j++) {
                ((*game)[index]).tokenCount[j] = sp[i].tokens;
            }
            (*game)[index].players = (struct GamePlayer*) calloc(
                    sp[i].players, sizeof(struct GamePlayer));
            (*game)[index].playerCount = sp[i].players - 1;
            break;
        }
    }
    int len = strlen(m->gameName) + 1;
    (*game)[index].name = (char*) calloc(len, sizeof(char));
    strcpy((*game)[index].name, m->gameName);
    (*game)[index].players[0].fileDescriptor = m->gamePlayer.fileDescriptor;
    (*game)[index].players[0].toPlayer = m->gamePlayer.toPlayer;
    (*game)[index].players[0].fromPlayer = m->gamePlayer.fromPlayer;
    (*game)[index].players[0].state.name = m->gamePlayer.state.name;
    (((GameCounter*) ((*game)[index].data))->currentPlayer)++;/*game data*/
    (*gameCount)++;/*gamecounter ++*/

}

/*free game struct*/
void free_game(struct Game* game) {
    for (int i = 0; i < game->playerCount; i++) {
        free(game->players[i].state.name);
    }
    free(game->players);
    free(game->name);
    game->name = NULL;
}

/* thread function
 * a new pthread will be raise
 * and play the game will dozen of players
 * */
void* pthread_play_new_game(void* args) {
    int gameTurns = 1;
    GameToPass* gargs = (GameToPass*) args;
    pthread_func_init_game_stats(gargs, gameTurns);
    struct Game* game = gargs->game;
    sort_player(game);
    char msg[TXT_SIZE];
    memset(msg, '\0', TXT_SIZE);

    /*be sure checked before start the thread
     *the deck file should not cause any errors
     * */
    parse_deck_file(&(game->deckSize), &(game->deck), gargs->deckFileName);

    for (int i = 0; i < BOARD_SIZE; i++) {
        draw_card(game);
    }
    pthread_func_main_game_logic(game, gargs->timeout);
    free_game(gargs->game);
    free(gargs->deckFileName);
    free(gargs);
    return NULL;

}

/* sort player use on first start up game
 * sort the player by name in order
 * */
void sort_player(struct Game* game) {
    struct GamePlayer tmp;
    int size = game->playerCount;
    for (int i = 0; i < size - 1; i++) {
        for (int j = 0; j < size - i - 1; j++) {
            char current = game->players[j].state.name[0];
            char after = game->players[j + 1].state.name[0];
            if (current > after) {
                tmp = game->players[j];
                game->players[j] = game->players[j + 1];
                game->players[j + 1] = tmp;
            } else if (current == after) {
                continue;
            }
        }
    }
}

/* before start the game
 * there is free stuff need to send before
 * 1.rid
 * 2.playinfo
 * 3.tokens
 *
 * */
void pthread_func_init_game_stats(GameToPass* gargs, int gameTurns) {
    struct Game* game = gargs->game;
    sort_player(game);
    char msg[TXT_SIZE];
    memset(msg, '\0', TXT_SIZE);
    /*setup player id and init palyer and send init msg to player*/
    for (int i = 0; i < game->playerCount; i++) {
        game->players[i].state.playerId = i;
        for (int j = 0; j < TOKEN_MAX; j++) {
            if (j < (TOKEN_MAX - 1)) {
                game->players[i].state.discounts[i] = 0;
            }
            game->players[i].state.tokens[i] = 0;
        }
        remove_newline_char(game->name);
        sprintf(msg, "rid%s,%d,%d", game->name, gameTurns, i);
        fprintf(game->players[i].toPlayer, "%s\n", msg);
        memset(msg, '\0', TXT_SIZE);
        char letter = (char) (i + 'A');
        sprintf(msg, "playinfo%c/%d", letter, game->playerCount);
        fprintf(game->players[i].toPlayer, "%s\n", msg);
        memset(msg, '\0', TXT_SIZE);
        sprintf(msg, "tokens%d", game->tokenCount[0]);
        fprintf(game->players[i].toPlayer, "%s\n", msg);
        fflush(game->players[i].toPlayer);
        game->players[i].state.score = 0;
    }
}

/* sned the end of game msg to players
 *
 * */
void pthread_func_send_eog(struct Game game) {
    for (int j = 0; j < game.playerCount; j++) {/*this player*/
        struct GamePlayer this = game.players[j];
        FILE* toPlayer = this.toPlayer;
        fprintf(toPlayer, "eog\n");
        fflush(toPlayer);
    }

}

/* parse come in message from players
 * reutrn the errorcode 
 * */
enum ErrorCode pthread_func_msg_from_player(char* msg,
        struct GamePlayer this, struct Game* game) {
    enum ErrorCode err = NOTHING_WRONG;
    remove_newline_char(msg);
    if (strstr(msg, "purchase") == msg) {
        err = handle_purchase_message(this.state.playerId, game, msg);
    } else if (strstr(msg, "take") == msg) {
        err = handle_take_message(this.state.playerId, game, msg);
    } else if (strstr(msg, "wild") == msg) {
        handle_wild_message(this.state.playerId, game);
        err = NOTHING_WRONG;
    }
    return err;
}

/*handle a player disconnect during the game*/
int disco_during_game(char* buffer, int timeout, struct GamePlayer this) {
    clock_t discoTime = clock();
    memset(buffer, '\0', TXT_SIZE);
    while ((int) ((clock() - discoTime) / CLOCKS_PER_SEC) <= timeout) {
        if (fgets(buffer, TXT_SIZE, this.fromPlayer) == NULL) {/*disco*/
            continue;
        } else {
            return 0;
        }
    }
    return -1;

}

/* main game logic
 * */
void pthread_func_main_game_logic(struct Game* game, int timeout) {
    char msg[TXT_SIZE];
    memset(msg, '\0', TXT_SIZE);
    enum ErrorCode err = NOTHING_WRONG;
    int invalidMsg = 0;
    /*infinity loop*/
    for (int i = 0; ; ) {
        if (is_game_over(game)) {
            pthread_func_send_eog(*game);
            pthread_exit(NULL);
        } else if (i >= game->playerCount) {/*reset index*/
            i = 0;
        }
        struct GamePlayer this = game->players[i];
        FILE* toPlayer = this.toPlayer;
        FILE* fromPlayer = this.fromPlayer;
        fprintf(toPlayer, "dowhat\n");
        fflush(toPlayer);
        if (fgets(msg, TXT_SIZE, fromPlayer) == NULL) {/*disco*/
            if (disco_during_game(msg, timeout, this) == -1) {
                display_disco_message(*game, this);
                free_game(game);
                pthread_exit(NULL);
            }
        }
        err = pthread_func_msg_from_player(msg, this, game);
        if (err == PROTOCOL_ERROR) {
            if ((++invalidMsg) == 2) {
                display_invalid_message(*game, this);
                pthread_exit(NULL);
            }
            memset(msg, '\0', TXT_SIZE);
            continue;
        } else {
            invalidMsg = 0;
            i++;
        }
        memset(msg, '\0', TXT_SIZE);
        (((GameCounter*) (game->data))->gameTurns)++;/*game data*/

    }
}

/* display the invalid message once player
 * send invalid message twice
 *
 * */
void display_invalid_message(struct Game game, struct GamePlayer this) {
    char who = id_to_letter(this.state.playerId);
    FILE* toPlayer = NULL;
    for (int i = 0; i < game.playerCount; i++) {
        toPlayer = game.players[i].toPlayer;
        fprintf(toPlayer, "invalid%c\n", who);
        fflush(toPlayer);
    }
}

/* displaye the disconnect message to all players
 * taht info there is a player disco causing eog
 *
 * */
void display_disco_message(struct Game game, struct GamePlayer this) {
    FILE* toPlayer = NULL;
    char who = id_to_letter(this.state.playerId);
    for (int i = 0; i < game.playerCount; i++) {
        if (this.state.playerId != game.players[i].state.playerId) {
            toPlayer = game.players[i].toPlayer;
            fprintf(toPlayer, "disco%c\n", who);
            fflush(toPlayer);
        }
    }

}

/* this function use to build the socket listen*/
int build_listen(SocketPool* sp) {
    char strPort[TXT_SIZE];
    memset(strPort, '\0', TXT_SIZE);
    sprintf(strPort, "%d", sp->port);
    struct addrinfo hints, * res, * result;
    int sock = -1;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if (getaddrinfo("localhost", strPort, &hints, &result)) {
        //fail cearte addrinfo
        return -1;
    }
    for (res = result; res != NULL; res = result->ai_next) {
        sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sock == -1) {/*socket error*/
            freeaddrinfo(result);
            return -1;
        }
        if (bind(sock, res->ai_addr, res->ai_addrlen) == -1) {/*bind error*/
            freeaddrinfo(result);
            close(sock);
            return -1;
        }
        if (listen(sock, SOMAXCONN) == -1) {/*listen error*/
            freeaddrinfo(result);
            close(sock);
            return -1;
        }
        break;/*one socket builed*/
    }
    int optVal = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(int)) < 0) {
        freeaddrinfo(result);
        close(sock);
        return -1;
    }
    struct sockaddr_in address;
    socklen_t len = sizeof(address);
    getsockname(sock, (struct sockaddr*) &address, &len);

    if (sp->port == 0) {
        sp->port = ntohs(address.sin_port);
    }
    sp->sock = sock;
    freeaddrinfo(result);
    return 0;
}

/* check if this playre already exist in score baord
 * if yes return the index of scoreBoard
 * otherwise return -1
 * */
int is_exist_in_scoreboard(const Score* scoreBoard, 
        const char* name, int size) {
    if (scoreBoard == NULL) {
        return -1;
    }
    for (int i = 0; i < size; i++) {
        if (strcmp(scoreBoard[i].player, name) == 0) {
            return i;
        }
    }
    return -1;

}

/* sore the score board in corect order
 * pre send preperation
 * */
void sort_socre_board(Score* scoreBoard, int size) {
    Score tmp;
    for (int i = 0; i < size - 1; i++) {
        for (int j = 0; j < size - i - 1; j++) {
            if (scoreBoard[j].points < scoreBoard[j + 1].points) {
                tmp = scoreBoard[j];
                scoreBoard[j] = scoreBoard[j + 1];
                scoreBoard[j + 1] = tmp;

            }
        }
    }
    for (int i = 0; i < size - 1; i++) {
        for (int j = 0; j < size - i - 1; j++) {
            if (scoreBoard[j].points == scoreBoard[j + 1].points) {
                if (scoreBoard[j].tokens > scoreBoard[j + 1].tokens) {
                    tmp = scoreBoard[j];
                    scoreBoard[j] = scoreBoard[j + 1];
                    scoreBoard[j + 1] = tmp;
                }

            }
        }
    }
}

/*displayer the score board*/
void display_score_board(Score* scoreBoard, int size, FILE* toScore) {
    for (int i = 0; i < size; i++) {
        Score s = scoreBoard[i];
        fprintf(toScore, "%s,%d,%d\n", s.player, s.tokens, s.points);
        fflush(toScore);
    }
}

/* handle socre client connection
 * send stream of game infomation
 * */
void handle_score_client(struct Game* game, int gameSize, int fd) {
    FILE* toScore = fdopen(fd, "r");

    fprintf(toScore, "Player Name,Total Tokens,Total Points");
    fflush(toScore);
    Score* scoreBoard = NULL;
    char msg[BUF_SIZE];
    memset(msg, '\0', BUF_SIZE);
    int size = 0, index = -1;
    for (int i = 0; i < gameSize; i++) {
        struct Game g = game[i];
        for (int j = 0; j < g.playerCount; j++) {
            struct Player p = g.players[j].state;
            if ((index = is_exist_in_scoreboard(scoreBoard, p.name, size)) 
                    == -1) {
                if ((scoreBoard = (Score*) realloc(scoreBoard,
                        (j + 1) * (i + 1))) == NULL) {
                    /*system error*/
                    server_err(SYS_ERROR);
                }
                Score s;
                memset(s.player, '\0', 100);
                strcpy(s.player, p.name);
                for (int k = 0; k < TOKEN_MAX - 1; k++) {
                    s.tokens += p.tokens[k];
                }
                s.points = p.score;
            } else {/*already exist on*/
                Score s = scoreBoard[index];
                for (int k = 0; k < TOKEN_MAX - 1; k++) {
                    s.tokens += p.tokens[k];
                }
                s.points += p.score;
            }
            size++;
        }
    }
    sort_socre_board(scoreBoard, size);
    display_score_board(scoreBoard, size, toScore);
    free(scoreBoard);
    fclose(toScore);
    shutdown(fd, SHUT_RDWR);
}

/*send new card msg to reconnection player*/
void reconnection_send_new_card(struct Game* game, FILE* toPlayer) {
    for (int i = 0; i < game->boardSize; i++) {
        struct Card flip = game->deck[1];
        char* message = print_new_card_message(flip);
        fprintf(toPlayer, "%s", message);
        fflush(toPlayer);
        free(message);
    }
}

/*send play info to reconnection player*/
void reconnection_player_info(struct Game* game, FILE* toPlayer) {
    char buffer[TXT_SIZE];
    for (int i = 0; i < game->playerCount; i++) {
        memset(buffer, '\0', TXT_SIZE);
        struct Player p = game->players[i].state;
        sprintf(buffer, "player%c:%d:d=%d,%d,%d,%d:t=%d,%d,%d,%d,%d",
                id_to_letter(p.playerId),
                p.score,
                p.discounts[0],
                p.discounts[1],
                p.discounts[2],
                p.discounts[3],
                p.tokens[0],
                p.tokens[1],
                p.tokens[2],
                p.tokens[3],
                p.tokens[4]);
        fprintf(toPlayer, "%s\n", buffer);
        fflush(toPlayer);
    }
}

/*check income reconnection msg is valid*/
int rid_validation(char* buffer) {
    remove_newline_char(buffer);
    char* str = (strdup(buffer) + 3);
    int i = 0;
    char result[3][TXT_SIZE];
    for (int i = 0; i < 3; i++) {
        memset(result[i], '\0', TXT_SIZE);
    }
    char* output = strtok(str, ",");/*split with ,0.gamename 1.gc 2.playerid*/
    for (; output != NULL; i++) {
        if (i == 0) {
            if (has_newline_char(output)) { 
                return -1;
            }
            if (has_comma(output)) {
                return -1;
            }
            if (strcmp(output, "") == 0) {
                return -1;
            }
        } else {
            if (!is_all_numbers(output)) {
                return -1;
            }
        }
        strcpy(result[0], output);
        output = strtok(NULL, ",");
    }
    if (!is_all_numbers(result[1])) {
        return -1;
    }
    if (!is_all_numbers(result[2])) {
        return -1;
    }
    int playerId = atoi(result[2]);
    if (playerId < 0 || playerId > 25) {
        return -1;
    }
    return 0;
}

/*handle player reconnection*/
int handle_reconnection(struct Game** game,
        int gameCount, Middleware* m) {
    char buffer[TXT_SIZE];
    memset(buffer, '\0', TXT_SIZE);
    fgets(buffer, TXT_SIZE, m->gamePlayer.fromPlayer);/*should rec rid..*/
    if (rid_validation(buffer) == -1) {
        fprintf(m->gamePlayer.toPlayer, "no");
        fflush(m->gamePlayer.toPlayer);
    }
    remove_newline_char(buffer);
    char* str = strdup(buffer);
    int i = 0;
    char result[3][TXT_SIZE];
    for (int i = 0; i < 3; i++) {
        memset(result[i], '\0', TXT_SIZE);
    }
    char* output = strtok(str, ",");/*split ,0.gamename 1.gc 2.playerid*/
    for (; output != NULL; i++) {
        strcpy(result[0], output);
        output = strtok(NULL, ",");
    }

    int index = find_game_index(*game, result[0], gameCount);
    if (index == -1) {/*game no longger exist*/
        fprintf(m->gamePlayer.toPlayer, "%s\n", "no");
        fflush(m->gamePlayer.toPlayer);
    } else {
        int id = atoi(result[2]);
        struct GamePlayer gp = m->gamePlayer;
        /*update file pointers*/
        (*game)[index].players[id].fileDescriptor = gp.fileDescriptor;
        (*game)[index].players[id].toPlayer = gp.toPlayer;
        (*game)[index].players[id].fromPlayer = gp.fromPlayer;
        reconnection_send_new_card(*game, gp.toPlayer);
        reconnection_player_info(*game, gp.toPlayer);
        wipe_middleware(m);
    }
    free(str);
    return 0;
}

/*init a game struct
 *no return value
 * */
void init_game_struct(struct Game* game) {
    game->name = NULL;
    game->winScore = 0;
    game->playerCount = 0;
    game->players = NULL;
    game->boardSize = 0;
    game->deckSize = 0;
    game->deck = NULL;
    for (int i = 0; i < TOKEN_MAX - 1; i++) {
        game->tokenCount[i] = 0;
    }
    GameCounter* gc = (GameCounter*) calloc(1, sizeof(GameCounter));
    gc->currentPlayer = 0;
    gc->gameTurns = 0;
    game->data = (void*) gc;
}

/* function will check all of exited game
 * and return the game index of array
 * if there is matched game name but game
 * is full or already started,it will skip over
 * if the game is overflowed or not exist return -1
 * if the game name is not overflowed and 
 * there is name matched then return the index
 * normally,index range will be 0<index<totalgame
 *
 * */
int find_game_index(const struct Game* game,
        const char* gameName, int gameCount) {
    for (int i = 0; i < gameCount; i++) {
        if (strcmp(game[i].name, gameName) == 0) {
            GameCounter* gc = ((GameCounter*) (game[i].data));
            int currentPlayer = gc->currentPlayer;/*game data*/
            if (game[i].playerCount == currentPlayer) {
                /*if same name game already started or overflow*/
                continue;
            } else {
                return i;
            }
        }
    }
    return -1;
}

/* once player connected via any port,
 * function will be called,
 * function will check around the game array
 * see if game name matached
 * if yes put this player into
 * if no realloc a new game and put this player into
 * also,if there is game name matched but overflow
 * function will realloc memeory and create a new game
 * with the game name a put this player into
 *no return value
 * */
void handle_play_connection(Middleware* m, struct Game** game,
        SocketPool* sp, int lines, int* gameCount) {
    if ((*game) == NULL) {/*strut game NULL,create a game with a player*/
        create_new_game_obj(m, game, sp, lines, gameCount);
    } else {/*at leat one game*/
        int index = find_game_index(*game, m->gameName, *gameCount);
        if (index == -1) {/*no game name existi,ceate one*/
            realloc_new_game_obj(m, game, sp, lines, gameCount);
        } else {
            /* check overflow*/
            GameCounter* gc = ((GameCounter*) ((*game)[index].data));
            int currentPlayer = gc->currentPlayer;/*game data*/
            if (currentPlayer != (*game)[index].playerCount) {
                struct GamePlayer* p = NULL;
                p = &(*game)[index].players[currentPlayer];/*dangour segfalut*/
                p->fileDescriptor = m->gamePlayer.fileDescriptor;
                p->toPlayer = m->gamePlayer.toPlayer;
                p->fromPlayer = m->gamePlayer.fromPlayer;
                p->state.name = m->gamePlayer.state.name;
                (((GameCounter*) ((*game[index]).data))->currentPlayer)++;

            } else {/*overflow,create a new game with a player*/
                realloc_new_game_obj(m, game, sp, lines, gameCount);

            }
        }

    }
}

/* function will be a checker that check
 * is there any game full,
 * if game is full then will ready to game
 * create a new thread and start play the game
 * all the argv will save in struct
 * and pass into thread
 * */
void resolve_start_new_game(struct Game* game, int gameCount,
        int timeout, char* deckFileName) {
    pthread_t t;
    for (int i = 0; i < gameCount; i++) {
        GameCounter* gc = ((GameCounter*) (game[i].data));
        int currentPlayer = gc->currentPlayer;/*game data*/
        if (game[i].playerCount == currentPlayer) {
            /*if lock then do not change this array*/
            GameToPass* gtp = (GameToPass*) calloc(1, sizeof(GameToPass));
            gtp->game = &game[i];
            gtp->timeout = timeout;
            gtp->deckFileName = strdup(deckFileName);
            pthread_create(&t, NULL, pthread_play_new_game, gtp);
            pthread_detach(t);
        }
    }
}

/* middleware will share accroess all
 * port listener.for safe reason,this function
 * will wipe all data before release mutex
 * no returen value
 * */
void wipe_middleware(Middleware* m) {
    memset(m->gameName, '\0', TXT_SIZE);
    memset(m->connectionMode, '\0', TXT_SIZE);
    memset(m->rid, '\0', TXT_SIZE);
    m->gamePlayer.fileDescriptor = -1;
    m->gamePlayer.toPlayer = NULL;
    m->gamePlayer.fromPlayer = NULL;
    m->gamePlayer.state.name = NULL;

}

/*check file content*/
void check_file_content(FILE* fp) {
    char buffer[BUF_SIZE];
    int i = 0;
    for (; fgets(buffer, BUF_SIZE, fp) != NULL; i++) {
        if (i > 1) {/*error*/
            fclose(fp);
            server_err(BAD_KEY_FILE);
        }
        if (strcmp(buffer, "") == 0) {/*error*/
            fclose(fp);
            server_err(BAD_KEY_FILE);
        }
        int len = strlen(buffer);
        if (buffer[len - 1] == '\n') {/*error*/
            fclose(fp);
            server_err(BAD_KEY_FILE);
        }
        memset(buffer, '\0', BUF_SIZE);
    }
    if (i == 0) {
        fclose(fp);
        server_err(BAD_KEY_FILE);
    }
    fclose(fp);
}

/* The function response to check arg count
 * file's missing ,file's content valid or not
 * any essential check if on this function
 * the pars passed from main,args and argv pointer
 * no return value
 * */
void args_validation(int args, char** argv) {
    if (args != 5) {/*error*/
        server_err(BAD_ARGS_CNT);
    }
    FILE* fp = NULL;
    /*key file open fail*/
    if ((fp = fopen(argv[1], "r")) == NULL) {/*error*/
        server_err(BAD_KEY_FILE);
    } else {/*open ok check if invalid*/
        check_file_content(fp);
    }
    /*stat file*/
    if ((fp = fopen(argv[3], "r")) == NULL) {
        server_err(BAD_STAT_FILE);
    }
    fclose(fp);
    struct Card* card = NULL;
    int cardCount = 0;
    enum DeckStatus ds;
    ds = parse_deck_file(&cardCount, &card, argv[2]);
    if (ds == DECK_ACCESS || ds == DECK_INVALID) {
        server_err(BAD_DECK_FILE);
    } else {
        free(card);
    }

    /*check timeout*/
    if (!is_all_numbers(argv[4])) {
        server_err(BAD_TIMEOUT);
    }

}

/* this is read statfile function that
 * read as a line and other help function
 * will do othere stuff.anyway it need a 
 * stats pointer and a fileName
 * no return value
 *
 * */
int read_stat(StatFile** stats, const char* fileName) {
    int lines = lines_count(fileName);
    *stats = (StatFile*) calloc(lines, sizeof(StatFile));
    memset(*stats, 0, lines * sizeof(StatFile));
    FILE* fp = fopen(fileName, "r");

    /*check newline start with '\n'*/
    int strlen = 0;
    char* output = NULL;
    char tmp[4][1024];
    for (int i = 0; i < 4; i++) {
        memset(tmp[i], '\0', 1024 * sizeof(char));
    }
    for (int i = 0; i < lines; i++) {
        if ((strlen = read_line(fp, &output, 0)) > -1) {/*read line ok*/
            if (has_space(output) || strcmp(output, "") == 0 || 
                    strlen == 0) {/*check has space*/
                read_stat_exit_with_err(fp, output, *stats);
                return -1;
                //server_err(BAD_STAT_FILE);
            }
            if (parse_stat_file(tmp, output) == -1) {
                read_stat_exit_with_err(fp, output, *stats);
                return -1;
            }
            if (equal(*stats, atoi(tmp[0]), i)) {
                read_stat_exit_with_err(fp, output, *stats);
                return -1;
            }
            StatFile s = {
                .port = atoi(tmp[0]),
                        .tokens = atoi(tmp[1]),
                        .points = atoi(tmp[2]),
                        .players = atoi(tmp[3])
            };
            (*stats)[i] = s;
            for (int j = 0; j < 4; j++) {
                memset(tmp[j], '\0', 1024);
            }
            memset(output, '\0', 1);
            free(output);
        }
    }
    fclose(fp);
    return lines;
}

/* help function
 * used to exit when read stat error
 * */
void read_stat_exit_with_err(FILE* fp, char* output, StatFile* stats) {
    free(output);
    fclose(fp);
    free(stats);
    stats = NULL;
}

/* read_stat's help function
 * this function use to copy 
 * raw data to  tmp array
 * no return value
 * */
int parse_stat_file(char output[4][1024], char* input) {
    int index = 0;
    int len = strlen(input);
    int comma[3];
    for (int j = 0; j < len; j++) {
        if (input[j] == ',') {
            comma[index++] = j;
        } else {
            if (!is_number(input[j])) {
                return -1;
            }
        }
    }
    strncpy(output[0], input, comma[0]);
    strncpy(output[1], input + comma[0] + 1, comma[1] - comma[0] - 1);
    strncpy(output[2], input + comma[1] + 1, comma[2] - comma[1] - 1);
    strcpy(output[3], input + comma[2] + 1);
    for (int i = 1; i < 4; i++) {
        if (output[i][0] == '-') {/*negative number*/
            return -1;
            //server_err(BAD_STAT_FILE);
        }
    }
    int amountPlayers = atoi(output[3]);
    if (amountPlayers < 2 || amountPlayers > 26) {/*2-26 players range*/
        return -1;
        //server_err(BAD_STAT_FILE);
    }
    memset(comma, 0, 3 * sizeof(int));
    return 0;
}

/* format series port to space separated
 * string and output to | stderr |
 * no return value
 * */
void output_port(const SocketPool* sp, int size) {
    char port[BUF_SIZE];
    char text[BUF_SIZE];
    memset(port, '\0', BUF_SIZE);
    memset(text, '\0', BUF_SIZE);
    for (int i = 0; i < size; i++) {
        sprintf(text, "%d ", sp[i].port);
        strcat(port, text);
        memset(text, '\0', BUF_SIZE);
    }
    int strSize = strlen(port);
    text[strSize - 1] = '\0';
    fprintf(stderr, "%s\n", port);


}

/* read_stat's help function
 * this function use to count how many
 * lines in file
 * return the number of new lines in file
 * */
int lines_count(const char* fileName) {
    FILE* fp = fopen(fileName, "r");
    char* buffer = (char*) calloc(1024, sizeof(char));
    int count = 0;
    memset(buffer, '\0', 1024);
    //count lines
    while (fgets(buffer, 1024, fp) != NULL) {
        count++;
        memset(buffer, '\0', 1024);
    }
    //if newline start with '\n' raise error
    if (buffer[0] == '\n') {
        free(buffer);
        fclose(fp);
        server_err(BAD_STAT_FILE);
    }
    free(buffer);
    fclose(fp);
    return count;
}

/* read_stat's help function
 * this function use to checkout
 * if there is any dup prot in array
 * return 1 if dup otherwise 0
 */
int equal(StatFile* stats, int port, int size) {
    for (int j = 0; j < size; j++) {
        if (stats[j].port == 0) {
            continue;
        }
        if (stats[j].port == port) {
            return 1;
        }
    }
    return 0;

}

/*read the key file*/
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
}

/* init middleware struct
 *
 * */
void init_middleware(Middleware* m, char* keyFile) {
    pthread_mutex_init(&(m->mutex), NULL);
    pthread_mutex_init(&(m->resource), NULL);
    m->spPos = 0;
    memset(m->connectionMode, '\0', TXT_SIZE);
    memset(m->rid, '\0', TXT_SIZE);
    memset(m->gameName, '\0', TXT_SIZE);
    memset(m->key, '\0', TXT_SIZE);
    FILE* keyFp = fopen(keyFile, "r");
    fgets(m->key, TXT_SIZE, keyFp);/*read key file*/
    fclose(keyFp);/*causing one test timeout*/
    m->sp = NULL;
    m->fd = 0;

}

/* swap stat file data to socket pool
 *
 * */
void swap_stat_to_socket_pool(SocketPool* sp, StatFile* stats, int lines) {
    for (int i = 0; i < lines; i++) {/*fill socketpool*/
        sp[i].port = stats[i].port;
        sp[i].tokens = stats[i].tokens;
        sp[i].points = stats[i].points;
        sp[i].players = stats[i].players;
    }
}
