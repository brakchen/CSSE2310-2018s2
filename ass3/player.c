#include "player.h"

/*
 *parse message comes from parent process
 *Parameter:
 *  code->message
 *  player->player object
 *  desk->desk object which contain hub info
 *Return:
 *  -1 -> game over
 *   1 -> dowhat
 *   0 -> do nothing
 *   2 -> invalid message
 * */
int parsing_message(char* code) {
    message_format_check(code);/*player only need to check format*/
    int doWhat = parsing_keyword(code);
    switch (doWhat) {
        case HUB_EOG:/*player recived end of game message*/
            return HUB_EOG;
        case HUB_DOWHAT:
            fprintf(stderr, "Received dowhat\n");
            return HUB_DOWHAT;
        case HUB_TOKENS:
            return HUB_TOKENS;
        case HUB_NEWCARD:/*board has new card been added*/
            return HUB_NEWCARD;
        case HUB_PURCHASED:
            return HUB_PURCHASED;
        case HUB_TOOK:
            return HUB_TOOK;
        case HUB_WILD:
            return HUB_WILD;
        default:/*corroct format but invalid keyword should raise error*/
            player_error(BAD_PIPE, NULL);
            return -1;
    }
}

/*
 * pasring "tokens" message from hub
 * return a int value: number of tokens in each non-wild pile 
 */
int* get_tokens(char* code) {
    char* tmp = (char*) calloc(10, sizeof(char));
    for (int i = 6, j = 0; i < strlen(code); i++, j++) {
        //if(!is_number(tmp[j])){
        //  player_error(BAD_PIPE,NULL)
        //}
        tmp[j] = code[i];
    }
    int* result = (int*) calloc(1, sizeof(int));
    if (!is_all_numbers(tmp)) {
        player_error(BAD_PIPE, NULL);
    }
    *result = atoi(tmp);
    free(tmp);
    return result;
}

/*
 * pasring "newcard" message from hub
 * return a 1D int array which include:
 * [(char),int,int,int,int,int]
 * [colour of dicount,worth,colour,colour,colour,colour]
 */
int* get_newcard(char* code) {
    int* array = (int*) calloc(50, sizeof(int));
    int* result = (int*) calloc(6, sizeof(int));
    char tmp[20] = {'\0'}, dest[40] = {'\0'};
    //int flag=0;
    int lastComma = 0, codeSize = strlen(code), index = 0;
    for (int i = 0; i < codeSize; i++)
        if (code[i] == ':')
            lastComma = i;

    /*left size TODO:need to add more limit*/
    for (int i = 7, j = 0; i < lastComma; i++) {
        if (is_number(code[i]))
            tmp[j++] = code[i];
        else if (!is_comma_or_colon(code[i]))
            result[index++] = (char) code[i];
    }
    result[index++] = atoi(tmp);
    /*right side*/
    split(&array, strcpy(dest, code + lastComma + 1), 0);
    for (int i = 0; array[i] != -1; i++)
        result[index++] = array[i];

    free(array);
    return result;
}

/*
 * pasring "purchased" message from hub
 * return a 1D int array which include:
 * [(char),int,int,int,int,int]
 * [player,number,colour,colour,colour,colour]
 */
int* get_purchased(char* code) {
    char* array = (char*) calloc(50, sizeof(int));
    int* result = (int*) calloc(7, sizeof(int));
    char tmp[20] = {'\0'}, dest[40] = {'\0'};
    int lastComma = 0, codeSize = strlen(code), index = 0;
    for (int i = 0; i < codeSize; i++)
        if (code[i] == ':')
            lastComma = i;

    /*left size*/
    for (int i = 9, j = 0; i < lastComma; i++) {
        if (is_number(code[i]))
            tmp[j++] = code[i];
        else if (!is_comma_or_colon(code[i]))
            result[index++] = (char) code[i];
    }
    /*right side*/
    result[index++] = atoi(tmp);
    strcpy(dest, code + lastComma + 1);
    int j = 0;
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

/*
 * pasring "took" message from hub
 * return a 1D int array which include:
 * [(char),int,int,int,int]
 * [player,colour,colour,colour,colour]
 */
int* get_took(char* code) {
    char* array = (char*) calloc(50, sizeof(char));
    int* result = (int*) calloc(5, sizeof(int));
    char dest[40] = {'\0'};
    int lastComma = 0, codeSize = strlen(code), index = 0;
    for (int i = 0; i < codeSize; i++)
        if (code[i] == ':')
            lastComma = i;
    /*left size*/
    result[index++] = (char) code[4];
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

/*
 * pasring "wild" message from hub
 * return the player will took the wild
 */
int* get_wild(char* code) {
    int* result = (int*) calloc(1, sizeof(int));
    result[0] = code[4];
    return result;
}

/*
 * To construct and init the player object
 * Parameters:
 * ID:player's ID
 * indicator: Player's indicator 
 * pid:process pid
 * will return a Player struct(object)
 */
Player** get_player_objects(int totalPlayer) {
    char letters[26] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    Player** tmp = (Player**) calloc(totalPlayer, sizeof(Player*));
    for (int i = 0; i < totalPlayer; i++) {

        Player* p = (Player*) calloc(1, sizeof(Player));

        for (int j = 0; j < 4; j++) {
            p->discount[j] = 0;
            p->tokens[j] = 0;
        }
        p->tokens[5] = 0;
        p->totalPoints = 0;
        p->indicator = letters[i];
        p->fd[0] = 0;
        p->fd[0] = 0;
        tmp[i] = p;

    }

    return tmp;

}

/**
 * construct a desk object
 * @param totalPlayer
 * @return
 */
Desk* get_desk_object(int totalPlayer) {
    Desk* desk = (Desk*) calloc(1, sizeof(Desk));
    desk->tokens = (int*) calloc(5, sizeof(int));
    desk->board = NULL;
    for (int i = 0; i < 5; i++)
        desk->tokens[i] = 0;
    desk->totalPlayer = totalPlayer;
    return desk;
}

/**
 * add card to board
 * @param board
 * @param result
 */
void add_card_to_board(Cards** board, int* result) {
    Cards* card = (Cards*) calloc(1, sizeof(Cards));
    card->color = (char) result[0];
    card->No = 0;
    card->worth = result[1];
    for (int i = 0; i < 4; i++) {
        card->price[i] = result[i + 2];
    }
    add_element(board, card);

}

/*parse keyword that spec proviced
 * parameter:
 * role parse who'message
 *could input any chars 
 *return relative number if ok
 * elese return -1 means dismatch
 * */
int parsing_keyword(char* text) {
    char* keywords[7] = {
            "eog",
            "dowhat",
            "tokens",
            "newcard",
            "purchased",
            "took",
            "wild"};
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
 * check message
 * @param text
 */
void checker(char* text) {

    char* tmp = (char*) calloc(BUFFER_SIZE, sizeof(char));
    strcpy(tmp, text);
    for (int i = 0; i < strlen(tmp); i++) {
        if (is_comma_or_colon(tmp[i])) {
            tmp[i] = ',';
        } else {
            if (!is_number(tmp[i])) {
                if (!is_letter(tmp[i])) {
                    free(tmp);
                    player_error(BAD_PIPE, NULL);
                }
            }
        }

    }
    free(tmp);
}

/**
 * check message format
 * @param text
 */
void message_format_check(char* text) {
    int key = parsing_keyword(text);
    int* result = count_comma_and_colon(text);/*0-comma 1-colon*/
    int comma = result[0];
    int colon = result[1];
    free(result);
    result = NULL;
    switch (key) {
        case 3:/*tokensT 0colon 0comma check in get tokens*/
            if (comma != 0 || colon != 0) {
                player_error(BAD_PIPE, NULL);
            }
            checker(text);
            break;
        case 4:/*newcard 2colon 3comma*/
            if (comma != 3 || colon != 2) {
                player_error(BAD_PIPE, NULL);
            }
            checker(text);
            break;
        case 5:/*purchased 2colon 4comma*/
            if (comma != 4 || colon != 2) {
                player_error(BAD_PIPE, NULL);
            }
            checker(text);
            break;
        case 6:/*took 1colon 3 comma*/
            if (comma != 3 || colon != 1) {
                player_error(BAD_PIPE, NULL);
            }
            checker(text);
            break;
        case 7:/*wild 0colon 0comma*/
            if (comma != 0 || colon != 0) {
                player_error(BAD_PIPE, NULL);
            }
            if (!is_capital_letter(text[4])) {
                player_error(BAD_PIPE, NULL);
            }
            checker(text);
            break;
        default:/*no need to check format*/
            break;
    }
}

/**
 * get message from pipe
 * @param count
 * @return
 */
char** get_read(int* count) {
    //fprintf(stderr,"into function\n");
    *count = 0;
    int i = 0, j = 0, k = 0, flag = 0;
    char** message = (char**) calloc(BUFFER_SIZE, sizeof(char*));
    message[i] = (char*) calloc(BUFFER_SIZE, sizeof(char));
    char* buffer = (char*) calloc(BUFFER_SIZE, sizeof(char));
    int rev = read(STDIN_FD, buffer, BUFFER_SIZE);
    if (rev == -1) {
    } else if (rev == 0) {
        sleep(2);
    }
    while (1) {
        if (strlen(buffer) == j) {
            if (flag == 0) {
                (*count)++;
            } else {
                (*count) = flag + 1;
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
 * write message to pipe
 * @param buffer
 */
void do_write(char* buffer) {
    write(STDOUT_FD, buffer, strlen(buffer));
}

/*
 *update player's discount ->onely update the player who sent this message
 *Parameter:
 *  player -> player
 *  discount -> dicount array
 * */
void update_player_discount(Player* player, char discount) {
    char keyword[4] = "PBYR";
    for (int i = 0; i < 4; i++) {
        if (keyword[i] == discount)
            player->discount[i]++;
    }

}

/*update players tokens,minus tokens which are used to buy a card
 *if someone buy a card ,the player's tokens must be reduced
 *because the player will pay for it ,
 *if player took a tokens from desk
 *need to update tokens to player status
 *Parameter:
 *  action -> either minus or add
 *  desk -> desk obj
 *  tokens -> how many tokens be used to buy a card
 * */
void update_player_tokens(Player* player, int* tokens, char* action) {
    if (strcmp(action, "minus") == 0) {

        for (int i = 0; i < 4; i++) {
            player->tokens[i] -= tokens[i + 1];
        }
    }

    if (strcmp(action, "add") == 0) {

        for (int i = 0; i < 4; i++) {
            player->tokens[i] += tokens[i + 1];
        }
    }
}

/**
 * update desk tokens infomation
 * @param desk
 * @param tokens
 */
void update_desk_tokens_purchased(Desk* desk, int* tokens) {

    for (int i = 0; i < 4; i++) {
        desk->tokens[i] += tokens[i];
    }
}

/*update desk tokens,minus tokens means a player tokens  tokens
 *if someone buy a card ,the tokens will put it back to desk
 *Parameter:
 *  action -> either minus or add
 *  desk -> desk obj
 *  tokens -> how many tokens be used to buy a card
 * */
void update_desk_tokens(Desk* desk, int* tokens, char* action) {
    if (strcmp(action, "minus") == 0) {

        for (int i = 0; i < 4; i++) {
            desk->tokens[i] -= tokens[i + 1];
        }
    }

    if (strcmp(action, "add") == 0) {

        for (int i = 0; i < 4; i++) {
            desk->tokens[i] += tokens[i + 1];
        }
    }
}

/*
 *this is a help function that used to 
 *split the purchased message from hub
 *
 *Parameter:
 *  result -> the parsed message from hub
 *Return:
 *  [tokens,tokens,tokens,toeksn]-return how many tokens used
 *
 * */
int* split_card_tokens(int result[10]) {
    int* tmp = (int*) calloc(5, sizeof(int));
    for (int i = 0; i < 5; i++) {
        tmp[i] = result[i + 2];
    }
    return tmp;
}

/*->ONLY USE TO UPDATE DESK INFO<-
 *like tokens newcard message.those messages
 *DO NOT NEED TO UPDATE PLAYER ANY INFO
 *normally,this just run in first start game
 *Parameter:
 *  desk -> the player's desk including board and tokens
 *  result -> the parsed message
 *  what -> what action,tokens?newcard?
 *
 *
 * */
void update_desk_info(Desk* desk, char* code) {
    int* result = NULL;
    int what = parsing_message(code);
    switch (what) {
        case HUB_TOKENS:
            result = get_tokens(code);/*tokens dont mess up*/
            for (int i = 0; i < 4; i++) {
                (desk->tokens)[i] = result[0];
            }
            break;
        case HUB_NEWCARD:
            result = get_newcard(code);
            add_card_to_board(&(desk->board), result);
            refresh_id(desk->board);
            break;
        case HUB_TOOK:
            result = get_took(code);/*took dont mess up*/
            update_desk_tokens(desk, result, "minus");
            break;
        default:/*ivalid message should raise error*/
            break;
    }
    free(result);
}

void update_player_tokens_purchase(Player* player, int* tokens) {
    for (int i = 0; i < 5; i++) {
        player->tokens[i] -= tokens[i];
    }
}

/*->ONLYE USE TO UPDATE PLAYER'S INFO<-
 *update player's info including self info like tokens and  wild
 *also player is maintaing deskas well
 *so player must update board and token which in struct desk
 *everyone desk info need to be updated
 *but self info only update by who send this message
 *Parameter:
 *  player -> the player's
 *  desk -> the player's desk including board and tokens
 *  result -> the parsed message
 *  what -> what action,tokens?newcard?
 * */
void update_player_info(Player** player,
                        Desk* desk, char* code) {
    int who = NULL;
    Cards* card = NULL;
    Cards* board = desk->board;
    int cardNum = -2;
    int* cardTokens = NULL;
    int* result = NULL;
    int what = parsing_message(code);
    switch (what) {
        case HUB_PURCHASED:
            result = get_purchased(code);/*purchased a new card*/
            who = get_id(result[0]);
            cardNum = result[1];
            card = find_element(board, cardNum);
            cardTokens = split_card_tokens(result);
            /*add token back to desk*/
            update_player_discount(player[who], card->color);
            update_player_tokens_purchase(player[who], cardTokens);
            update_desk_tokens_purchased(desk, cardTokens);
            player[who]->totalPoints += card->worth;
            del_element(&(desk->board), cardNum);
            refresh_id(desk->board);/*do not foget to refres id*/
            free(cardTokens);
            break;
        case HUB_TOOK:/*player take tokens need to refresh desk's tokens*/
            result = get_took(code);
            who = get_id(result[0]);
            update_player_tokens(player[who], result, "add");
            break;
        case HUB_WILD:
            result = get_wild(code);
            who = get_id(result[0]);
            /*[self]onely update who take wild*/
            player[who]->tokens[4] += 1;
            break;
        default:/*ivalid message should raise error*/
            break;
    }
    free(result);
}

/*
 *copy a card obj
 * */
Cards* copy(Cards* card) {
    Cards* tmp = (Cards*) calloc(1, sizeof(Cards));
    tmp->color = card->color;
    tmp->worth = card->worth;
    tmp->No = card->No;
    for (int i = 0; i < 4; i++) {
        tmp->price[i] = card->price[i];
    }
    tmp->next = NULL;
    return tmp;

}


/*only use to construct a message to purchase a card
 *Parameter:
 *  card -> the card info want to send
 *  wild -> how many wild use to buy this card
 *Return:
 *  message -> the constructed message
 * */
char* construct_purchase_message(Cards* card, int* price) {
    char* message = (char*) calloc(BUFFER_SIZE, sizeof(char));
    snprintf(message, BUFFER_SIZE, "purchase%d:%d,%d,%d,%d,%d\n",
             card->No,
             price[0],
             price[1],
             price[2],
             price[3],
             price[4]);
    return message;
}

/*only use to construct a message to take tokens
 *Parameter:
 *  tokens -> how many toeksn wants to take
 *Return:
 *  message -> the constructed message
 * */
char* construct_take_message(int tokens[4]) {
    char* message = (char*) calloc(BUFFER_SIZE, sizeof(char));
    snprintf(message, BUFFER_SIZE, "take%d,%d,%d,%d\n",
             tokens[0],
             tokens[1],
             tokens[2],
             tokens[3]);
    return message;
}

/*only use to construct a messaage to take wild
 *Return:
 *  message -> the constructed message
 * */
char* construct_wild_message() {
    char* message = (char*) calloc(BUFFER_SIZE, sizeof(char));
    snprintf(message, BUFFER_SIZE, "wild\n");
    return message;
}

/**
 * output bard infomation
 * @param desk
 */
void output_board_status(Desk* desk) {
    char* message = (char*) calloc(BUFFER_SIZE, sizeof(char));
    Cards* card = desk->board;
    /*TODO:double check if is necessary*/
    while (card != NULL) {
        snprintf(message, BUFFER_SIZE, "Card %d:%c/%d/%d,%d,%d,%d",
                 card->No,
                 card->color,
                 card->worth,
                 card->price[0],
                 card->price[1],
                 card->price[2],
                 card->price[3]);
        fprintf(stderr, "%s\n", message);
        memset(message, '\0', BUFFER_SIZE);
        card = card->next;
    }
    free(message);

}

/**
 * output player information
 * @param player
 * @param totalPlayer
 * @param self
 */
void output_player_status(Player** player, int totalPlayer, int self) {
    char* message = (char*) calloc(BUFFER_SIZE, sizeof(char));
    for (int i = 0; i < totalPlayer; i++) {

        snprintf(message, BUFFER_SIZE,
                 "Player %c:%d:Discounts=%d,%d,%d,%d:Tokens=%d,%d,%d,%d,%d",
                 player[i]->indicator,
                 player[i]->totalPoints,
                 player[i]->discount[0],
                 player[i]->discount[1],
                 player[i]->discount[2],
                 player[i]->discount[3],
                 player[i]->tokens[0],
                 player[i]->tokens[1],
                 player[i]->tokens[2],
                 player[i]->tokens[3],
                 player[i]->tokens[4]);
        fprintf(stderr, "%s\n", message);
        memset(message, '\0', BUFFER_SIZE);
    }

    free(message);
}

/**
 * check player args
 * @param args
 * @param argv
 * @param who
 */
void check_player_args(int args, char** argv, char* who) {
    char* name = NULL;
    int i = 0;
    int count = 0;
    if (who != NULL) {
        while (1) {
            if (who[i] != '.')
                count++;
            else
                break;
            i++;
        }
        name = (char*) calloc(BUFFER_SIZE, sizeof(char));
        strncpy(name, who, count);

    }
    if (args != PLAYER_MAX_ARGS_COUNT) {
        player_error(ARGS_COUNT, name);
        free(name);
    }
    if (!is_all_numbers(argv[1]))
        player_error(BAD_PLAYER_COUNT, NULL);

    if (!is_all_numbers(argv[2]))
        player_error(BAD_ID, NULL);

    int id = atoi(argv[2]);
    int playerCount = atoi(argv[1]);
    if (playerCount < 1 || playerCount > MAX_PLAYER)
        player_error(BAD_PLAYER_COUNT, NULL);

    if (id > MAX_PLAYER - 1)
        /*start from 0 so the player id should less or euqal 25*/
        player_error(BAD_ID, NULL);
    if (playerCount < 2) {
        player_error(BAD_PLAYER_COUNT, NULL);
    }
    if (id > playerCount)
        player_error(BAD_ID, NULL);

    free(name);
}

/**
 * get player id
 * @param indicator
 * @return
 */
int get_id(char indicator) {
    if (is_number(indicator)) {
        return -1;
    }
    char letter[26] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    for (int i = 0; i < 26; i++) {
        if (letter[i] == indicator) {
            return i;
        }
    }
    return -1;
}

/**
 * output end of game
 * @param desk
 * @param player
 * @param self
 */
void output_eog(Desk* desk, Player** player, int self) {
    char* message = (char*) calloc(BUFFER_SIZE, sizeof(char));
    int index = 0;
    char winners[16] = {'\0'};
    int maxPoint = 0;
    int maxPoints[26];
    for (int i = 0; i < 26; i++) {
        maxPoints[i] = -1;
    }
    for (int i = 0; i < desk->totalPlayer; i++) {
        if (player[i]->totalPoints > maxPoint) {
            maxPoint = player[i]->totalPoints;
        }
    }
    for (int i = 0; i < desk->totalPlayer; i++) {
        if (player[i]->totalPoints >= maxPoint) {
            maxPoints[index++] = i;
        }
    }
    index = 0;
    for (int i = 0; i < 26; i++) {
        if (maxPoints[i] != -1) {
            winners[index++] = player[maxPoints[i]]->indicator;
            winners[index++] = ',';
        }
    };
    strcat(message, "Game over. Winners are ");
    strcat(message, winners);
    int length = strlen(message);
    message[length - 1] = '\0';
    fprintf(stderr, "%s\n", message);
    free(message);
    exit(0);

}
