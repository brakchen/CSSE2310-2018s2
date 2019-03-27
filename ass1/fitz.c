#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


/* store tile info */
struct Tile {
    int numOfTile;
    char **tile;
    int index;
};

/* store player info during the game */
struct Player {
    char shape;
    char type;
    int win;
    int aNext;
    int rNext;
    int cNext;
    int turns;

};

/* store board info during the game*/
struct Board {
    int height;
    int width;
    char **board;
    int whoseTurn;
};

/* take previous player placed coordinate */
struct Start {
    int aNext;
    int rNext;
    int cNext;
    int turns;
};

void rotate(char **);

int run_diff_game(struct Player *, char **, struct Board *, struct Tile *,
        struct Start *);

void read_tile_from_file(FILE *, struct Tile *);

char **martix_transitive(char *);

char **martix_rotate(char **);

void place_tile_in_board(char **, int, int, char, int, struct Board *);

void game_start(char *, char *, int, struct Board *, struct Tile *,
        struct Start *);

void human_play(struct Player *, char **, struct Board *, struct Tile *,
        struct Start *);

void show_rotated_matrix(struct Tile *);

void handle_err(int);

void print_tile(char **);

void print_board(struct Board *);

int auto_play_one(struct Player *, char **, struct Board *, struct Tile *,
        struct Start *);

int auto_play_two(struct Player *, char **, struct Board *, struct Tile *,
        struct Start *);

int is_placable(char **, int, int, int, struct Board *);

void read_saved_game(char *, struct Board *, struct Tile *t);

int save_game(int, int, char *fileName, struct Board *);

int is_board_full(char **, struct Board *);

void free_char_n_dimension(char **, int);

void handle_err_usage(char *);

void set_up_board(int, int, struct Board *);

void flip_turn(struct Board *);

void read_saved_game_first_line(FILE *, struct Board *, struct Tile *);

void reset_start(int, int, int, struct Start *);

void stdin_check_read(struct Player *, int *, struct Board *board,
        struct Tile *);

int stdin_check_input(int *, char **, struct Player *, struct Board *,
        struct Tile *t, struct Start *);

void reset_player_start(struct Player *, int, int, int);

void auto_algorithm_one(int *, int *, struct Board *);

void auto_algorithm_two(int *, int *, struct Board *);

int is_player_type_legal(char *, char *);

int is_file_open_legal(char *);

int is_single_number(char);

int is_every_number_legal(char *);

int is_args_legal(int, char **);

int is_board_size_legal(char *, char *);

int trigger_save(struct Player *, struct Board *, struct Tile *);

void init_struct(struct Board *, struct Tile *, struct Start *);

/**
 * init the struct value
 * @param board (struct *) the struct board pointer
 * @param t (struct *) the tile struct pointer
 * @param start (struct *) the start postion pointer
 */
void init_struct(struct Board *board, struct Tile *t, struct Start *start) {
    //setup
    board->height = 0;
    board->width = 0;
    board->board = NULL;
    board->whoseTurn = 1;
    t->numOfTile = 0;
    t->tile = NULL;
    t->index = 0;
    start->aNext = 0;
    start->rNext = -2;
    start->cNext = -2;
    start->turns = 0;
}

/*
 * main function is a program entry
 * return 0 if success
 * */
int main(int argv, char **args) {
    //structs
    struct Board board;
    struct Tile t;
    struct Start start;
    init_struct(&board, &t, &start);

    //after check must be safe
    is_args_legal(argv, args);
    read_tile_from_file(fopen(args[1], "r"), &t);
    char *savedFileName = NULL;
    int height = 0, width = 0;
    switch (argv) {
        case 2:
            //output tile
            show_rotated_matrix(&t);
            free_char_n_dimension(t.tile, t.numOfTile);
            break;
        case 5:
            //the name of a file containing a saved game to load
            savedFileName = args[4];
            read_saved_game(savedFileName, &board, &t);
            game_start(args[2], args[3], board.whoseTurn, &board, &t, &start);
            //free tile
            free_char_n_dimension(t.tile, t.numOfTile);
            break;
        case 6:
            //board height width readed
            height = atoi(args[4]), width = atoi(args[5]);
            set_up_board(height, width, &board);
            print_board(&board);
            //start the game
            game_start(args[2], args[3], 0, &board, &t, &start);
            //free board
            free_char_n_dimension(board.board, board.height);
            free_char_n_dimension(t.tile, t.numOfTile);
            break;
        default:
            break;
    }
    return 0;
}

/*
 * specific handle the usage err
 * throw exit(10) status and printf the usage msg
 */
void handle_err_usage(char *fileName) {
    int flag = 0;
    //file name
    char *mainFileName = NULL;
    mainFileName = (char *) calloc(70, sizeof(char));
    while (1) {
        if (flag > (int) strlen(fileName)) {
            break;
        }
        //if touch .
        if (fileName[flag] == '.') {
            //if follow is /
            if (fileName[flag + 1] == '/') {
                //copy the file name
                strcpy(mainFileName, fileName + flag + 2);
                break;
            }
        }
        flag++;
    }
    fprintf(stderr, "Usage: %s tilefile [p1type p2type [height width | "
            "filename]]\n", mainFileName);
    free((void *) mainFileName);
    exit(1);

}

/**
 * check if board size legal
 * @param height(char *) board height
 * @param width(char *) board width
 * @return 1 if success, 0 if fail
 */
int is_board_size_legal(char *height, char *width) {
    int intHeight = 0;
    int intWdith = 0;
    //check if is digit
    //if board height len =1
    if ((strlen(height) == 1)) {
        if (!is_single_number(height[0])) {
            return 0;
        }
    } else {
        //if board height len !=1
        if (!is_every_number_legal(height)) {
            return 0;
        }
    }
    if (strlen(width) == 1) {
        if (!is_single_number(width[0])) {
            return 0;
        }
    } else {
        if (!is_every_number_legal(width)) {
            return 0;
        }
    }
    //heck if is in the range
    intHeight = atoi(height);
    intWdith = atoi(width);
    if (intHeight <= 1 || intHeight >= 1000 || intWdith <= 1 ||
            intWdith >= 1000) {
        return 0;
    }
    return 1;
}

/*
 *check if is a number and single
 * @param data(char)
 * @return 1 if success, 0 if fail
 */
int is_single_number(char data) {
    //check acssii code for numebr
    if (data >= 48 && data <= 57) {
        return 1;
    }
    return 0;
}

/*
 *  check if player type legal
 * @param p1 (char *) position x
 * @param p2 (char *) position y
 * @return 1 if success, 0 if fail
 */
int is_player_type_legal(char *p1, char *p2) {
    //if boath h
    if (*p1 == 'h' && *p2 == 'h') {
        if (strlen(p1) == 1 && strlen(p2) == 1) {
            return 1;
        } else {
            return 0;
        }
    }
    //if player one is h
    if (*p1 == 'h') {
        //check size of h
        if (strlen(p1) != 1) {
            return 0;
        }
    } else {
        // not h check other char's size
        if (strlen(p1) != 1) {
            return 0;
        }
        //not 1 or 2
        if (*p1 != '1' && *p1 != '2') {
            return 0;
        }
    }

    if (*p2 == 'h') {
        if (strlen(p2) != 1) {
            return 0;
        }
    } else {
        if (strlen(p2) != 1) {
            return 0;
        }
        if (*p2 != '1' && *p2 != '2') {
            return 0;
        }
    }
    return 1;
}

/**
 * help method for free memory
 * @param data (char **)
 * @param times int
 */
void free_char_n_dimension(char **data, int times) {
    //free n time memory
    for (int j = 0; j < times; j++) {
        free((void *) *(data + j));
    }
    free((void *) data);
}

/**
 * help method for setup board
 * @param height (int) board height
 * @param width (int) board width
 * @param board (struct *) the struct board pointer
 */
void set_up_board(int height, int width, struct Board *board) {
    board->height = height, board->width = width;
    //calloc the memory for board
    board->board = (char **) calloc((unsigned int) board->height,
            sizeof(char *));
    for (int i = 0; i < board->height; i++) {
        *(board->board + i) = (char *) calloc((unsigned int) board->width,
                sizeof(char));
        //fill . and construct the board
        memset(*(board->board + i), '.', board->width);
    }

}

/**
 * check if file path is legal
 * @param path (char *) file path
 * @return 1 if success, 0 if fail
 */
int is_file_open_legal(char *path) {
    //open file
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        return 0;
    } else {
        //yes then close bbuff
        fclose(fp);
        return 1;
    }
}

/**
 * check every char is in the number range
 * @param data (char *) data need to check
 * @return 1 if success, 0 if fail
 */
int is_every_number_legal(char *data) {
    //check every char
    for (int i = 0; i < (int) strlen(data); i++) {
        if (!is_single_number(*(data + i))) {
            return 0;
        }
    }
    return 1;
}

/**
 * check all args are legal or not
 * @param argv (int) number  of args
 * @param args (char **) the args array
 * @return 1 if success, 0 if fail
 */
int is_args_legal(int argv, char **args) {
    char *tmp = (char *) calloc(70, sizeof(char));
    char *fileName = args[0];
    //there is  args=1，2，5，6 condition
    switch (argv) {
        case 2:
            //case 2 only tile file
            if (!is_file_open_legal(args[1])) {
                handle_err(2);
            }
            break;
        case 5:
            //check tile file if legal
            if (!is_file_open_legal(args[1])) {
                handle_err(2);
            }
            //check saved file if legal
            if (!is_file_open_legal(args[4])) {
                handle_err(6);
            }
            //player check
            if (!is_player_type_legal(args[2], args[3])) {
                handle_err(4);
            }
            break;
        case 6:
            //check  tile file if legal
            if (!is_file_open_legal(args[1])) {
                handle_err(2);
            }
            //check player
            if (!is_player_type_legal(args[2], args[3])) {
                handle_err(4);
            }
            //check board size if legal
            if (!is_board_size_legal(args[4], args[4])) {
                handle_err(5);
            }
            break;
        default:
            //other condition thorw usage
            handle_err_usage(fileName);
            break;
    }
    //free memory
    free((void *) tmp);
    return 1;
}

/**
 * load the file first line date
 * @param fp (FILE *) file pointer
 * @param board (struct *) struct board pointer
 * @param t (struct *) struct tile pointer
 */
void read_saved_game_first_line(FILE *fp, struct Board *board,
        struct Tile *t) {
    char ch, *tmp = (char *) calloc(70, sizeof(char));

    int k = 0, l = 0, tmpArray[4] = {0};
    do {
        ch = fgetc(fp);
        if (ch != ' ' && ch != '\n') {
            if (ch <= 57 && ch >= 48) {
                *(tmp + k++) = ch;
            } else {
                handle_err(7);
            }
            continue;
        } else {
            tmpArray[l] = atoi(tmp);
            l++;
            memset(tmp, '\0', 70);
            k = 0;
            continue;
        }
    } while (ch != '\n');
    board->whoseTurn = tmpArray[1];
    t->index = tmpArray[0];
    set_up_board(tmpArray[3], tmpArray[2], board);

    free((void *) tmp);
}

/**
 * read the saved game from file
 * @param path (char *) file path
 * @param board (struct *) the struct board pointer
 * @param t (struct *) the tile struct pointer
 */
void read_saved_game(char *path, struct Board *board, struct Tile *t) {
    FILE *fp = fopen(path, "r");
    char ch = 'a';
    int j = 0, i = 0;
    if (fp != NULL) {
        read_saved_game_first_line(fp, board, t);
        do {
            ch = getc(fp);
            if (ch == EOF) {
                break;
            }
            if (ch == '*') {
                *(*(board->board + i) + j) = ch;
                j++;
                continue;
            }
            if (ch == '#') {
                *(*(board->board + i) + j) = ch;
                j++;
                continue;
            }
            if (ch == '\n') {
                i++;
                j = 0;
                continue;
            }
            if (ch == '.') {
                j++;
                continue;
            }
            if (ch != '*' && ch != '#' && ch != '\n' && ch != '.') {
                handle_err(7);
            }

        } while (ch != EOF);
        fclose(fp);
        print_board(board);
    } else {
        handle_err(6);
    }
}

/**
 * save the game to file
 * @param nextTile (int) the next tile
 * @param player (int) start from which player
 * @param rows (int) the board's row
 * @param columns (int) the board's columns
 * @param fileName (char *) file path
 * @param board (struct *) the struct board pointer
 * @return 1 if succ ,o if fail
 */
int save_game(int nextTile, int player, char *fileName, struct Board *board) {
    //TODO:need to check
    int rows = board->width;
    int columns = board->height;
    FILE *fp = NULL;
    fp = fopen(fileName, "w");
    if (fp != NULL) {
        //output formated buffer
        fprintf(fp, "%d %d %d %d\n", nextTile, player, rows, columns);
        for (int i = 0; i < board->width; i++) {
            for (int j = 0; j < board->height; j++) {
                //output board
                fprintf(fp, "%c", *(*(board->board + i) + j));
            }
            fprintf(fp, "\n");
        }
        fclose(fp);
        return 1;
    } else {
        return 0;
    }

}

/**
 * use to indicate which type of game should run
 * @param player (struct Player *) which player will play
 * @param tile (char **) just tile
 * @return 1 if success, 0 if fail
 * @param board (struct *) the struct board pointer
 * @param t (struct *) the tile struct pointer
 * @param start (struct *) the start postion pointer
 */
int run_diff_game(struct Player *player, char **tile, struct Board *board,
        struct Tile *t, struct Start *start) {
    int flag = -1;
    if (player->type == '1') {
        //auto play type one
        flag = auto_play_one(player, tile, board, t, start);
    } else if (player->type == '2') {
        //auto play type two
        flag = auto_play_two(player, tile, board, t, start);
    } else if (player->type == 'h') {
//        print_board(board);
        if (is_board_full(tile, board) == 1) {
            printf("board full\n");
            //player->win = 0;
            if (board->whoseTurn == 0) {
                board->whoseTurn = 1;
            } else {
                board->whoseTurn = 0;
            }
            return 0;
        }
        //human play

        human_play(player, tile, board, t, start);
    }
    return flag;
}

/**
 * init some date before start game
 * @param playerOne (struct Player *) player
 * @param playerTwo (struct Player *) player
 */
void set_up_game_start(struct Player *playerOne, struct Player *playerTwo) {
    //set up struct init value
    playerOne->shape = '*';
    playerTwo->shape = '#';
    playerOne->win = 0;
    playerTwo->win = 0;
    playerOne->cNext = -2;
    playerOne->rNext = -2;
    playerOne->aNext = 0;
    playerTwo->cNext = -2;
    playerTwo->rNext = -2;
    playerTwo->aNext = 0;
    playerOne->turns = 0;
    playerTwo->turns = 0;
}

/**
 * start the game
 * @param playerOneType (struct Player *) player
 * @param playerTwoType (struct Player *) player
 * @param whoseTurn (int) which player's turn
 * @param board (struct *) the struct board pointer
 * @param t (struct *) the tile struct pointer
 * @param start (struct *) the start postion pointer
 */
void game_start(char *playerOneType, char *playerTwoType, int whoseTurn,
        struct Board *board, struct Tile *t, struct Start *start) {
    struct Player playerOne;
    struct Player playerTwo;
    playerOne.type = *playerOneType;
    playerTwo.type = *playerTwoType;
    board->whoseTurn = whoseTurn;
    set_up_game_start(&playerOne, &playerTwo);
    char **tmp = NULL;
    while (1) {
        if (t->index >= t->numOfTile) {
            t->index = 0;
        }
        tmp = martix_transitive(*(t->tile + t->index));
        //run game dependence on whose turn
        switch (board->whoseTurn) {
            case 0:
                playerOne.win = run_diff_game(&playerOne, tmp, board, t,
                        start);
                break;
            case 1:
                playerTwo.win = run_diff_game(&playerTwo, tmp, board, t,
                        start);
                break;
            default:
                break;
        }
        // free unuse tile memory
        free_char_n_dimension(tmp, 5);
        if (playerOne.win == 1 || playerTwo.win == 1) {
            //someone wins
            break;
        }
    }

    if (playerOne.win == 1 || playerTwo.win == 1) {
        if (board->whoseTurn == 0) {
            printf("Player * wins\n");
        } else {
            printf("Player # wins\n");
        }
    }
}

/**
 * trigger the save game function
 * @param player (struct Player *) which player want to  save
 * @param board (struct *) the struct board pointer
 * @param t (struct *) the tile struct pointer
 * @return 1 if success, 0 if fail
 */
int trigger_save(struct Player *player, struct Board *board, struct Tile *t) {
    char *string = (char *) calloc(70, sizeof(char));
    char ch;
    int flag = 1;
    char *saveString = (char *) calloc(4, sizeof(char));
    string[0] = 's';

    while ((ch = fgetc(stdin)) != '\n') {
        *(string + flag) = ch;
        flag++;
    }
    strncpy(saveString, string, 4);
    if (strcmp(saveString, "save") != 0) {
        free(saveString);
        return 0;
    }
    char *path = (char *) calloc(70, sizeof(char));
    strcpy(path, string + 4);
    if (player->shape == '*') {
        if (!save_game(t->numOfTile, 1, path, board)) {
            free(saveString);
            free(path);
            return 0;
        }
    } else {
        if (!save_game(t->numOfTile, 0, path, board)) {
            free(saveString);
            free(path);
            return 0;
        }
    }
    //free "save" saveString
    free(saveString);
    //free the path allocted memory
    free(path);
    return 1;
}

/**
 * read the user input
 * @param player (struct Player *) which player's input
 * @param board (struct *) the struct board pointer
 * @param t (struct *) the tile struct pointer
 * @param argvs (int *) number of args
 */
void stdin_check_read(struct Player *player, int *argvs, struct Board *board,
        struct Tile *t) {
    char ch;
    char *string = (char *) calloc(70, sizeof(char));
    int flag = 0;
    int space = 0;
    do {
        ch = fgetc(stdin);
        if (ch == EOF) {
            free(string);
            handle_err(10);
        }
        if (ch == ' ' || ch == '\n') {
            argvs[space] = atoi(string);
            space++;
            memset(string, '\0', 70);
            //if not set flag to zero will lead   to atoi string to 0
            flag = 0;

        } else {
            if (ch <= 57 && ch >= 48) {
                *(string + flag) = ch;
                flag++;
            } else if (ch == 45) {
                //nagetive number
                *(string + flag) = ch;
                flag++;
            } else if (ch == 115 && flag == 0) {
                //start with s
                *(string + flag) = ch;
                flag++;
                if (!trigger_save(player, board, t)) {
                    continue;
                } else {
                    break;
                }
            }
        }
    } while (ch != '\n');
    argvs[3] = space;
    free((void *) string);
}

/**
 * check the user input
 * @param argvs (int*) the arguments
 * @param tmp (char **)tile
 * @param player (struct Player *)playyer
 * @param board (struct *) the struct board pointer
 * @param t (struct *) the tile struct pointer
 * @param start (struct *) the start postion pointer
 * @return 1 if success, 0 if fail
 */
int stdin_check_input(int *argvs, char **tmp, struct Player *player,
        struct Board *board, struct Tile *t, struct Start *start) {
    int angle = 0, x, y, rotation = 0, space = 0;

    while (1) {
        printf("Player %c] ", player->shape);
        stdin_check_read(player, argvs, board, t);
        space = argvs[3];
        if (space == 3) {
            x = argvs[0];
            y = argvs[1];
            angle = argvs[2];
        } else {
            continue;
        }
        //check angle range
        if (angle == 0 || angle == 90 || angle == 180 || angle == 270) {
            rotation = angle / 90;
        } else {
            continue;
        }
        //if placable
        if (is_placable(tmp, x, y, rotation, board)) {
            t->index++;
            flip_turn(board);
            place_tile_in_board(tmp, x, y, player->shape, rotation, board);
            //TODO :need to double check
            reset_start(x, y, angle, start);
            return 0;
        } else {
            //check if board is full
            if (is_board_full(tmp, board) == 1) {
                player->win = 1;
                return 1;
            }
            continue;
        }
    }
}

/**
 * start human play
 * @param player (struct Player *)player
 * @param tmp (char **) tile
 * @param board (struct *) the struct board pointer
 * @param t (struct *) the tile struct pointer
 * @param start (struct *) the start postion pointer
 */
void human_play(struct Player *player, char **tmp, struct Board *board,
        struct Tile *t, struct Start *start) {
    //this function will call by diff player
    int argvs[4] = {0};
    if (t->index >= t->numOfTile) {
        t->index = 0;
    }
    print_tile(tmp);
    stdin_check_input(argvs, tmp, player, board, t, start);
}

/**
 * reset all next turn data
 * @param r (int) row
 * @param c (int) column
 * @param angle (int) angle
 * @param start (struct *) the start postion pointer
 */
void reset_start(int r, int c, int angle, struct Start *start) {
    //set start value
    start->turns++;
    start->aNext = angle;
    start->cNext = c;
    start->rNext = r;
}

/**
 * flip player turns
 * @param board (struct *) the struct board pointer
 */
void flip_turn(struct Board *board) {
    //flip whose turn
    if (board->whoseTurn == 0) {
        board->whoseTurn = 1;
    } else {
        board->whoseTurn = 0;
    }
}

/**
 * check board if is full
 * @param tmp (char **) tmp
 * @param board (struct *) the struct board pointer
 * @return 1 if success, 0 if fail
 */
int is_board_full(char **tmp, struct Board *board) {
    int r = -2, c = -2, startAngle = 0;
    //player1
    while (1) {
        if (is_placable(tmp, r, c, startAngle / 90, board)) {
            return 0;
        }
        c++;
        if (c > board->width + 2) {
            c = -2;
            r++;
        }
        if (r > board->height + 2) {
            r = -2;
        }
        if (r == board->height + 2 && c == board->width + 2) {
            startAngle += 90;
        }

        if (startAngle > 270) {
            return 1;
        }
    }
}


/**
 * auto play type one
 * @param player (struct Player *)player
 * @param tmp (char **) tile
 * @param board (struct *) the struct board pointer
 * @param t (struct *) the tile struct pointer
 * @param start (struct *) the start postion pointer
 * @return 1 if win, 0 if lose
 */
int auto_play_one(struct Player *player, char **tmp, struct Board *board,
        struct Tile *t, struct Start *start) {
    int r = -2, c = -2, startAngle = 0, fixedAngle = 90;
    if (start->turns == 0) {
        //player1
        r = -2;
        c = -2;
        startAngle = 0;
        fixedAngle = 90;
    } else {
        r = start->rNext;
        c = start->cNext;
        startAngle = 0;
    }
    while (1) {
        int rotation = startAngle / fixedAngle;
        if (is_placable(tmp, r, c, rotation, board)) {
            //can place
            printf("Player %c => %d %d rotated %d\n", player->shape, r, c,
                    startAngle);
            place_tile_in_board(tmp, r, c, player->shape, rotation, board);
            reset_start(r, c, startAngle, start);
            t->index++;
            flip_turn(board);
            player->win = 0;
            return 0;

        }
        auto_algorithm_one(&c, &r, board);
        if (r == start->rNext && c == start->cNext) {
            startAngle += 90;
        }
        if (startAngle > 270) {
            reset_start(r, c, startAngle, start);
            flip_turn(board);
            player->win = 1;
            return 1;
        }
    }
}

/**
 * reset all player next turn data
 * @param player (struct Player *)playyer
 * @param r (int) row
 * @param c (int) column
 * @param angle (int) angle
 */
void reset_player_start(struct Player *player, int r, int c, int angle) {
    player->turns++;
    player->rNext = r;
    player->cNext = c;
    player->aNext = angle;
}

/**
 * help method for auto type one
 * @param c (int) column
 * @param r (int) row
 * @param board (struct *) the struct board pointer
 */
void auto_algorithm_one(int *c, int *r, struct Board *board) {
    (*c)++;
    if ((*c) > board->width + 2) {
        *c = -2;
        (*r)++;
    }
    if (*r > board->height + 2) {
        (*r) = -2;
    }

}

/**
 * help method for auto type two
 * @param c (int) column
 * @param r (int) row
 * @param board (struct *) the struct board pointer
 */
void auto_algorithm_two(int *c, int *r, struct Board *board) {
    (*c)--;
    if ((*c) < -2) {
        (*c) = board->height + 2;
        (*r)--;
    }
    if ((*r) < -2) {
        (*r) = board->width + 2;
    }

}

/**
 * auto play type two
 * @param player (struct Player *) player
 * @param tmp (char **)tile
 * @return 1 if win, 0 if lose
 * @param board (struct *) the struct board pointer
 * @param t (struct *) the tile struct pointer
 * @param start (struct *) the start postion pointer
 */
int auto_play_two(struct Player *player, char **tmp, struct Board *board,
        struct Tile *t, struct Start *start) {
    int r = -2, c = -2, startAngle = 0, fixedAngle = 90;
    if (player->turns == 0 && player->shape == '*') {
        //player1
        r = -2;
        c = -2;
    } else if (player->turns == 0 && player->shape == '#') {
        r = board->height + 2;
        c = board->width + 2;
    } else {
        r = player->rNext;
        c = player->cNext;
    }
    while (1) {
        int rotation = startAngle / fixedAngle;
        if (is_placable(tmp, r, c, rotation, board)) {
            printf("Player %c => %d %d rotated %d\n", player->shape, r, c,
                    startAngle);
            place_tile_in_board(tmp, r, c, player->shape, rotation, board);
            reset_player_start(player, r, c, startAngle);
            reset_start(r, c, startAngle, start);
            t->index++;
            flip_turn(board);
            player->win = 0;
            return 0;
        }
        startAngle += 90;
        if (startAngle > 270) {
            if (player->shape == '*') {
                auto_algorithm_one(&c, &r, board);
            }
            if (player->shape == '#') {
                auto_algorithm_two(&c, &r, board);
            }
            startAngle = 0;
            if (r == player->rNext && c == player->cNext) {
                reset_player_start(player, r, c, startAngle);
                reset_start(r, c, startAngle, start);
                flip_turn(board);
                player->win = 1;
                return 1;
            }
        }
    }
}

/**
 * check if is tile placeble
 * @param tile (char **) tile
 * @param x (int) x-axis
 * @param y (int) y-axis
 * @param rotation (int) angle to rotation
 * @param board (struct *) the struct board pointer
 * @return 1 if success, 0 if fail
 */
int is_placable(char **tile, int x, int y, int rotation, struct Board *board) {
    //tmp memory for save the tile
    char **tmp = (char **) calloc(5, sizeof(char *));
    for (int i = 0; i < 5; i++) {
        *(tmp + i) = (char *) calloc(5, sizeof(char));
        for (int j = 0; j < 5; j++) {
            *(*(tmp + i) + j) = *(*(tile + i) + j);
        }
    }
    //rotate n times
    for (int k = 0; k < rotation; k++) {
        rotate(tmp);
    }
    //check if placeable
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            if (*(*(tmp + i) + j) == '!') {
                int relativeX = i - 2 + x;
                int relativeY = j - 2 + y;
                //not empty part  out of board
                if (relativeX < 0 || relativeX > board->height - 1 ||
                        relativeY < 0 || relativeY > board->width - 1) {
                    for (int l = 0; l < 5; l++) {
                        free((void *) *(tmp + l));
                    }
                    free((void *) tmp);
                    return 0;
                }
                //overlap
                if (*(*(board->board + relativeX) + relativeY) == '#' ||
                        *(*(board->board + relativeX) + relativeY) == '*') {
                    for (int l = 0; l < 5; l++) {
                        free((void *) *(tmp + l));
                    }
                    free((void *) tmp);
                    return 0;
                }
            }
        }
    }

    for (int i = 0; i < 5; i++) {
        free((void *) *(tmp + i));
    }
    free((void *) tmp);
    return 1;
}

/**
 * place tile in board
 * @param tile (char **) tile
 * @param x (int) x-axis
 * @param y (int) y-axis
 * @param rotation (int) angle to rotation
 * @param player (struct Player *) player
 * @param board (struct *) the struct board pointer
 */
void place_tile_in_board(char **tile, int x, int y,
        char player, int rotation, struct Board *board) {
    //save tile to a tmp array
    char **tmp = (char **) calloc(5, sizeof(char *));
    for (int i = 0; i < 5; i++) {
        *(tmp + i) = (char *) calloc(5, sizeof(char));
        for (int j = 0; j < 5; j++) {
            *(*(tmp + i) + j) = *(*(tile + i) + j);
        }
    }
    //rotate the tile N times
    for (int k = 0; k < rotation; k++) {
        rotate(tmp);
    }
    //place tile
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            int relativeX = i - 2 + x;
            int relativeY = j - 2 + y;
            if (*(*(tmp + i) + j) == '!') {
                *(*(board->board + relativeX) + relativeY) = player;
            }
        }
    }
    //free memory
    for (int j = 0; j < 5; j++) {
        free((void *) *(tmp + j));
    }
    free((void *) tmp);
    //print board
    print_board(board);
}

/**
 * print tile
 * @param tile (char **) tile
 */
void print_tile(char **tile) {
    char **tmp = tile;
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            printf("%c", *(*(tmp + i) + j));
        }
        printf("\n");
    }
}

/**
 * read tile from file
 * @param fileOpen (FILE *) FILE object
 * @param t (struct *) the tile struct pointer
 */
void read_tile_from_file(FILE *fileOpen, struct Tile *t) {

    char **tile = (char **) calloc(1, sizeof(char *));
    *(tile) = (char *) calloc(5 * 5, sizeof(char));
    if (fileOpen != NULL) {
        char *array = (char *) calloc(25, sizeof(char));
        char ch = 'a';
        int count = 0, i = 0;
        while ((ch = fgetc(fileOpen)) != EOF) {
            //read file validations
            if (ch != '\n') {
                if (ch != '!' && ch != ',' && ch != '\n') {
                    handle_err(3);
                }
                count = 0;
                array[i] = ch;
                i++;
                if (i == 25) {
                    for (int k = 0; k < 25; k++) {
                        *(*(tile + t->numOfTile) + k) = *(array + k);
                    }
                    t->numOfTile++;
                }
            } else {
                count++;
                if (count == 2) {
                    i = 0;
                    tile = realloc(tile, sizeof(char *) * (t->numOfTile + 1));
                    if (tile != NULL) {
                        *(tile + t->numOfTile) = (char *) calloc(5 * 5,
                                sizeof(char));
                    }
                }
            }
        }
        free((void *) array);
    } else {
        //Tile fıle can’t be read
        exit(2);
    }
    fclose(fileOpen);
    //give it to other var
    t->tile = tile;
}

/**
 * transfer the martix
 * @param a (char **) tile
 * @return (char **) return the transfer tile
 */
char **martix_rotate(char **a) {
    //a new  temp memory
    char **temp = (char **) calloc(5, sizeof(char *));
    for (int i = 0; i < 5; i++) {
        *(temp + i) = (char *) calloc(5, sizeof(char));
        for (int j = 0; j < 5; j++) {
            *(*(temp + i) + j) = *(*(a + i) + j);
        }
    }
    //returned memory
    char **rotated = (char **) calloc((5 * 4), sizeof(char *));
    for (int i = 0, k = 0, c = 0; k < 4; k++) {
        for (int b = 0, j = 0; c < 5; i++, c++) {
            *(rotated + i) = (char *) calloc(5, sizeof(char));
            for (j = 0; j < 5; j++, b++) {
                *(*(rotated + i) + j) = *(*(temp + c) + j);
            }
        }
        c = 0;
        rotate(temp);
    }
    //Free  temp memory
    for (int i = 0; i < 5; i++) {
        free((void *) *(temp + i));
    }
    free((void *) temp);
    return rotated;
}

/**
 * Converse a array[n][25] to a array[5][5]
 * @param a (char *) tile
 * @return (char**) array[5][5]
 */
char **martix_transitive(char *a) {
    //a new copy of martix
    //TODO:need to trace the memory and free
    char **array = (char **) calloc(5, sizeof(char *));
    for (int i = 0, k = 0; i < 5; i++) {
        *(array + i) = (char *) calloc(5, sizeof(char));
        for (int j = 0; j < 5; j++, k++) {
            *(*(array + i) + j) = *(a + k);
        }
    }
    return array;
}

/**
 * rotate the tile
 * @param a (char **) tile
 */
void rotate(char **a) {
    char temp = ' ';
    for (int i = 0; i < 5; ++i) {
        for (int j = 0; j < i; ++j) {
            temp = *(*(a + i) + j);
            *(*(a + i) + j) = *(*(a + j) + i);
            *(*(a + j) + i) = temp;
        }
    }
    for (int i = 0; i < 5; ++i) {
        for (int j = 5 - 1; j >= 5 / 2; --j) {
            temp = *(*(a + i) + j);
            *(*(a + i) + j) = *(*(a + i) + (5 - j - 1));
            *(*(a + i) + (5 - j - 1)) = temp;
        }
    }
}

/**
 * print out  the roated matrix
 * @param t (struct *) the tile struct pointer
 */
void show_rotated_matrix(struct Tile *t) {
    for (int num = 0; num < t->numOfTile; num++) {
        char **transitive = martix_transitive(t->tile[num]);
        char **rotated = martix_rotate(transitive);
        for (int k = 0; k < 5; k++) {
            for (int i = k, flag = 0; i < 20; i = i + 5, ++flag) {
                for (int j = 0; j < 5; j++) {
                    printf("%c", *(*(rotated + i) + j));
                }
                if (flag < 3) {
                    printf(" ");
                }
            }
            printf("\n");
        }
        if (num < t->numOfTile - 1) {
            printf("\n");
        }
        free_char_n_dimension(rotated, 20);
        free_char_n_dimension(transitive, 5);
    }
}

/**
 * print  the board
 */
void print_board(struct Board *board) {
    for (int i = 0; i < board->height; i++) {
        for (int j = 0; j < board->width; j++) {
            if (*(*(board->board + i) + j) != '\0') {
                printf("%c", *(*(board->board + i) + j));
            }
        }
        fprintf(stdout, "\n");
    }
}

/**
 * handle the diff err
 * @param code (int) the error code
 */
void handle_err(int code) {
    switch (code) {
        case 2:
            fprintf(stderr, "Can't access tile file\n");
            exit(2);
        case 3:
            fprintf(stderr, "Invalid tile file contents\n");
            exit(3);
        case 4:
            fprintf(stderr, "Invalid player type\n");
            exit(4);
        case 5:
            fprintf(stderr, "Invalid dimensions\n");
            exit(5);
        case 6:
            fprintf(stderr, "Can't access save file\n");
            exit(6);
        case 7:
            fprintf(stderr, "Invalid save file contents\n");
            exit(7);
        case 10:
            fprintf(stderr, "End of input\n");
            exit(10);

        default:
            exit(11);

    }
}

