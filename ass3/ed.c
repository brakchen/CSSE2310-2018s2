#include "ed.h"

static int ed = 0;

/**
 * main
 * @param args
 * @param argv
 * @return
 */
int main(int args, char** argv) {
    /*TODO:check vaild*/
    check_player_args(args, argv, __FILE__);
    int totalPlayer = atoi(argv[1]);
    int self = atoi(argv[2]);
    char indicator = argv[2][0];
    Player** player = get_player_objects(indicator);
    Desk* desk = get_desk_object(totalPlayer);
    ed_game(player, desk, self);
    for (int i = 0; i < totalPlayer; i++) {

        free(player[i]->name);
        free(player[i]);
    }
    free(player);
    //free_list(desk->board
    free(desk->board);
    free(desk);
    return 0;
}

/**
 * get indicator from id
 * @param self
 * @return
 */
char get_indicator_by_id(int self) {
    char keyword[26] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    return keyword[self];
}

/**
 * ed main game
 * @param player
 * @param desk
 * @param self
 */
void ed_game(Player** player, Desk* desk, int self) {
    int rev = 0, doWhat = -1;
    char** buffer = NULL;
    while (1) {

        ed++;
        buffer = get_read(&rev);
        if (rev > 0) {/*ok there are messages*/
            for (int i = 0; i < rev; i++) {
                char* code = buffer[i];
                if (code == NULL) {
                    sleep(1);
                    /*blcking io*/
                    /*TODO:NEED to CHECK*/
                    continue;
                }
                doWhat = parsing_message(code);
                switch (doWhat) {
                    case HUB_DOWHAT:/*dowhat*/
                        do_what(desk, player, code, self);
                        break;
                    case HUB_EOG:/*game over*/
                        output_eog(desk, player, self);
                        fprintf(stderr, "eog\b");
                        return;
                    case -1:/*error message*/
                        player_error(BAD_PIPE, NULL);
                        break;
                    default:/*invalid message handle on parent class*/
                        update_desk_info(desk, code);
                        update_player_info(player, desk, code);
                        output_board_status(desk);
                        output_player_status(player, desk->totalPlayer, self);
                        break;
                }
                free(buffer[i]);/*finish*/
            }
        } else if (rev == 0) {/*empty buffer*/
            player_error(BAD_PIPE, NULL);/*conmmnication error*/
        }
    }
}

/**
 * no card can take
 * @param desk
 * @param selfID
 */
void no_card_can_take(Desk* desk, int selfID) {
    char* message = NULL;
    if (can_take_token(desk->tokens)) {/*can take tokens*/
        int* tokens = take_token(desk->tokens);
        message = construct_take_message(tokens);
        do_write(message);
        free(tokens);
        free(message);
    } else {/*can not take tokens*/
        message = construct_wild_message();
        message = construct_wild_message();
        do_write(message);
        free(message);
    }
}

/*
 *if yes return cardnum
 *if no return -1
 * */
int only_one_card_can_take(Desk* desk, Player** player, int selfID) {
    char self = get_indicator_by_id(selfID);
    int totalPlayer = desk->totalPlayer;
    int** buyList = (int**) calloc(totalPlayer, sizeof(int*));
    int index = 0;
    Cards* board = desk->board;
    Cards* card = NULL;
    int count = 0;
    int cardNum = -1;
    for (int i = 0; i < totalPlayer; i++) {
        if (player[i]->indicator != self) {/*other player*/
            buyList[i] = (int*) calloc(8, sizeof(int));
            fill_data(buyList[i], 8, -1);
            for (int j = 0; j < get_card_amount(desk->board); j++) {
                card = find_element(desk->board, j);
                card = copy(card);
                if (can_buy_a_card(player[i],
                                   card)) {/*if can buy put in list*/
                    buyList[i][index++] = i;
                }
            }
        }
        index = 0;
    }
    /*find high worth*/
    /*in player order not including self*/
    int** worthList = (int**) calloc(totalPlayer, sizeof(int*));

    for (int i = 0; i < totalPlayer; i++) {/*loop each player*/
        if (player[i]->indicator !=
            self) {/*loop each player not include self*/
            worthList[i] = (int*) calloc(8, sizeof(int));
            memset(worthList[i], -1, 8);
            for (int j = 0; j < 8; j++) {
                if (buyList[i][j] != -1) {
                    card = find_element(board, buyList[i][j]);
                    card = copy(card);
                    worthList[i][j] = card->worth;
                }
            }
        }
        ordered_worth(buyList, worthList, totalPlayer - 1);
    }
    int maxWorth = most_worth_in_list(worthList, totalPlayer - 1);

    for (int i = 0; i < totalPlayer - 1; i++) {
        for (int j = 0; j < 8; j++) {
            if (worthList[i][j] >= maxWorth) {
                count++;
                cardNum = buyList[i][j];
            }
        }
    }
    if (count == 1) {/*one card*/
        return cardNum;
    } else {/*more than one card*/
        return -1;
    }
}

/**
 * at least one card can buy
 * @param desk
 * @param player
 * @param code
 * @param selfID
 * @return
 */
int at_least_one_card_can_buy(Desk* desk, Player** player,
                              char* code, int selfID) {

    char self = get_indicator_by_id(selfID);
    int totalPlayer = desk->totalPlayer;
    int** buyList = (int**) calloc(totalPlayer, sizeof(int*));
    int index = 0;
    Cards* card = NULL;
    for (int i = 0; i < totalPlayer; i++) {
        if (player[i]->indicator != self) {/*other player*/
            buyList[i] = (int*) calloc(8, sizeof(int));
            fill_data(buyList[i], 8, -1);
            for (int j = 0; j < get_card_amount(desk->board); j++) {
                card = find_element(desk->board, j);
                card = copy(card);
                if (can_buy_a_card(player[i], card)) {
                    /*if can buy put in list*/
                    buyList[i][index++] = i;
                    return 1;
                }
            }
        }
        index = 0;
    }
    return 0;
}

/**
 * fill data in list
 * @param list
 * @param size
 * @param data
 */
void fill_data(int* list, int size, int data) {
    for (int i = 0; i < size; i++) {
        list[i] = data;
    }
}

/*
 *dowhat
 * */
void do_what(Desk* desk, Player** player, char* code, int selfID) {
    int cardNum = -1;
    if (at_least_one_card_can_buy(desk, player, code, selfID)) {
        if ((cardNum = only_one_card_can_take(desk, player, selfID)) != -1) {
            buy_a_card(cardNum, player[selfID], desk);

        } else {/*more than one has height worth*/
            char self = get_indicator_by_id(selfID);
            int totalPlayer = desk->totalPlayer;
            int** buyList = (int**) calloc(totalPlayer, sizeof(int*));
            int index = 0;
            Cards* card = NULL;
            int cardNum = -1;
            for (int i = 0; i < totalPlayer; i++) {
                if (player[i]->indicator != self) {/*other player*/
                    buyList[i] = (int*) calloc(8, sizeof(int));
                    fill_data(buyList[i], 8, -1);
                    for (int j = 0; j < get_card_amount(desk->board); j++) {
                        card = find_element(desk->board, j);
                        card = copy(card);
                        if (can_buy_a_card(player[i],
                                           card)) {/*if can buy put in list*/
                            buyList[i][index++] = i;
                        }
                    }
                }
                index = 0;
            }
            cardNum = buy_which_card(buyList, desk, player, selfID);
            buy_a_card(cardNum, player[selfID], desk);
        }
    } else {/*take tokens*/
        no_card_can_take(desk, selfID);
    }

    //free(buyList);

}

/**
 * buy a card
 * @param which
 * @param player
 * @param desk
 */
void buy_a_card(int which, Player* player, Desk* desk) {
    Cards* board = desk->board;
    Cards* card = find_element(board, which);
    card = copy(card);
    int* tokensNeed = NULL;
    char* message = NULL;
    if (can_buy_a_card(player, card)) {
        int* tokensNeed = tokens_need(card, player);
        message = construct_purchase_message(card, tokensNeed);
        do_write(message);
        free(tokensNeed);
        free(card);
        refresh_id(board);
    } else {
        if (can_buy_after_take(desk, player, card)) {
            tokensNeed = take_tokens_to_buy_card(desk, player, card);
            message = construct_take_message(tokensNeed);
            do_write(message);
            free(message);
            free(tokensNeed);
        } else {
            message = construct_wild_message();
            do_write(message);
            free(message);
        }
    }

}

/**
 * check deskt tokens
 * @param tokens
 * @return
 */
int check_desk_tokens_empty(int tokens[4]) {
    int count = 0;
    for (int i = 0; i < 4; i++) {
        if (tokens[i] == 0) {
            count++;
        }
    }
    if (count == 4) {
        return 1;
    }
    return 0;
}

/**
 * copy player
 * @param player
 * @return
 */
Player* copy_player(Player* player) {
    Player* tmp = (Player*) calloc(1, sizeof(Player));
    for (int i = 0; i < 5; i++) {
        tmp->tokens[i] = player->tokens[5];
    }
    return tmp;
}

/**
 * check if player can take card
 * @param desk
 * @param player
 * @param card
 * @return
 */
int can_buy_after_take(Desk* desk, Player* player, Cards* card) {
    take_tokens_to_buy_card(desk, player, card);
    Player* p = copy_player(player);
    if (can_buy_a_card(player, card)) {
        free(p);
        return 1;
    }
    free(p);
    return 0;

}

/**
 * take token to buy card
 * @param desk
 * @param player
 * @param card
 * @return
 */
int* take_tokens_to_buy_card(Desk* desk, Player* player, Cards* card) {

    int cardPrice[4] = {0};
    int* needTake = (int*) calloc(4, sizeof(int));
    int playerTokens[4];
    int discountcpy[4];
    int deskTokens[4];
    int takeCount = 0;
    int order[4] = {2, 3, 1, 0};
    for (int i = 0; i < 4; i++) {
        cardPrice[i] = card->price[i];
        playerTokens[i] = player->tokens[i];
        discountcpy[i] = player->discount[i];
        deskTokens[i] = desk->tokens[i];
    }
    for (int i = 0; i < 4; i++) {
        while (cardPrice[i] != 0 && discountcpy[i] != 0) {
            cardPrice[i]--;
            discountcpy[i]--;
        }
        while (cardPrice[i] != 0 && playerTokens[i] != 0) {
            cardPrice[i]--;
            playerTokens[i]--;
        }
    }
    for (int i = 0;; i++) {
        int which = order[i];
        if (cardPrice[which] != 0 && deskTokens[which] > 0) {
            cardPrice[which]--;
            deskTokens[which]--;
            needTake[which]++;
            takeCount++;
        }
        if (check_desk_tokens_empty(deskTokens) || takeCount == 3) {
            break;
        }
        if (i > 4) {
            i = 0;
        }
    }
    return needTake;
}

/**
 * finr the worth heighest card in list
 */
int most_worth_in_list(int** mostWorth, int length) {
    int max = 0;
    for (int i = 0; i < length; i++) {
        if (mostWorth[i][0] > max) {
            max = mostWorth[i][0];
        }
    }
    return max;

}

/**
 * solve if multiple same most heighest worth
 * @param mostWorth
 * @param desk
 * @param worthList
 * @param buyList
 * @param selfID
 * @return
 */
int multiple_most_worth(int mostWorth, Desk* desk,
                        int** worthList, int** buyList, int selfID) {

    Cards* board = desk->board;
    int totalPlayer = desk->totalPlayer;
    int* playerList = (int*) calloc(totalPlayer - 1, sizeof(int));;
    memset(playerList, -1, totalPlayer - 1);
    int cardCount = 0;
    Cards* card = NULL;
    int oldest = 7;
    int index = 0;
    for (int i = 0; i < totalPlayer - 1; i++) {
        if (mostWorth == worthList[i][0]) {
            playerList[index++] = i;
        }
    }
    for (int playerID = selfID + 1;; playerID++) {
        if (playerID > totalPlayer) {
            playerID = 0;
        }
        for (int i = 0; i < totalPlayer - 1; i++) {
            if (playerList[i] == playerID) {
                for (int j = 0; j < 8; j++) {
                    if (worthList[playerID][j] == mostWorth) {
                        cardCount++;
                    }
                }
                if (cardCount > 1) {
                    for (int k = 0; k < 8; k++) {
                        if (buyList[playerID][k] != -1) {
                            card = find_element(board, buyList[playerID][k]);
                            card = copy(card);
                            if (card->No < oldest) {
                                oldest = card->No;
                            }
                        }
                    }
                    free(playerList);
                    return oldest;

                } else {
                    free(playerList);
                    return buyList[playerID][0];
                }

                //return buyList[playerID][0];
            }
        }
    }
    return -1;
}

/*
 *from cardList decise buy which card
 *Parameters:
 *  board -> all cards
 *  canBuyList->cardList ,a list of card could be buy
 *Return:
 * -1 -> there is not card can buy.It is impossible value bug check needed
 *  ? -> card number
 * */
int buy_which_card(int** buyList, Desk* desk, Player** player, int selfID) {
    Cards* board = desk->board;
    char self = get_indicator_by_id(selfID);
    int totalPlayer = desk->totalPlayer;
    Cards* card = NULL;


    /*in player order not including self*/
    int** worthList = (int**) calloc(totalPlayer, sizeof(int*));

    for (int i = 0; i < totalPlayer; i++) {/*loop each player*/
        if (player[i]->indicator !=
            self) {/*loop each player not include self*/
            worthList[i] = (int*) calloc(8, sizeof(int));
            memset(worthList[i], -1, 8);
            for (int j = 0; j < 8; j++) {
                if (buyList[i][j] != -1) {
                    card = find_element(board, buyList[i][j]);
                    card = copy(card);
                    worthList[i][j] = card->worth;
                }
            }
        }
        ordered_worth(buyList, worthList, totalPlayer - 1);
    }

    int mostWorthInAllPlayer = most_worth_in_list(worthList, totalPlayer - 1);
    return multiple_most_worth(mostWorthInAllPlayer, desk, worthList, buyList,
                               selfID);

}

/**
 * check repeat
 * @param list
 * @param length
 * @return
 */
int check_repeat(int* list, int length) {

    for (int i = 0; i < length; i++) {
        for (int j = 0; j < length; j++) {
            if (j == i) {
                continue;
            } else {
                if (list[i] == list[j]) {
                    return 1;
                }
            }
        }
    }

    return 0;

}

/*
 *can take token?
 *Parameter: 
 *  tokens[4] -> tokens from board
 *Return:
 *  1 -> yes
 *  0 -> no
 *
 * */
int can_take_token(int tokens[4]) {
    int count = 0;
    if (is_yellow_can_take(tokens)) {
        count++;
    }

    if (is_brown_can_take(tokens)) {
        count++;
    }
    if (is_purple_can_take(tokens)) {
        count++;
    }
    if (is_red_can_take(tokens)) {
        count++;
    }

    if (count >= 3) {

        return 1;
    }
    return 0;
}

/*
 *take tokens
 *Parameters:
 *  tokens -> tokens in board
 *Return:
 *  
 *  int*:the tokens needed to take
 * */
int* take_token(int tokens[4]) {
    int order[4] = {2, 3, 1, 0};
    int copy[4];
    for (int i = 0; i < 3; i++) {
        copy[i] = tokens[i];
    }

    int* tmp = (int*) calloc(4, sizeof(int));
    int count = 0;
    for (int i = 0; i < 4; i++) {

        if (tokens[order[i]] != 0) {
            tmp[order[i]]++;
            copy[order[i]]--;
            count++;
        }
        if (count == 3) {
            break;
        }
    }

    return tmp;

}

/**
 * order a list from big to smale
 */

void sort_list(int* list, int count) {
    int swap;
    for (int i = 0; i <= count; i++) {
        for (int j = i + 1; j < count; j++) {
            if (list[i] < list[j]) {
                swap = list[i];
                list[i] = list[j];
                list[j] = swap;
            }
        }

    }
}

/**
 * ordered worth
 * @param buyList
 * @param worthList
 * @param length
 */
void ordered_worth(int** buyList, int** worthList, int length) {
    int count = length;
    int swap;
    int swapCard;
    for (int k = 0; k < count; k++) {
        for (int i = 0; i < 8; i++) {
            for (int j = i + 1; j < 8; j++) {
                if (worthList[k][i] < worthList[k][j]) {
                    swap = worthList[k][i];
                    swapCard = buyList[k][i];
                    worthList[k][i] = worthList[k][j];
                    buyList[k][i] = buyList[k][j];
                    worthList[k][j] = swap;
                    buyList[k][j] = swapCard;
                }
            }

        }
    }
}

/*
 *bug checked? no
 *check if shenzi can buy this card
 *return:
 * 1 -> yes
 * 0 -> no
 * */
int can_buy_a_card(Player* player, Cards* card) {
    /*ordered worth list*/

    int money[4] = {0};
    int cardPrice[4] = {0};
    int wild = player->tokens[4];
    for (int i = 0; i < 4; i++) {
        money[i] = player->tokens[i] + player->discount[i];
        cardPrice[i] = card->price[i] - money[i];/*reduce by tokens&discount*/
        while (cardPrice[i] > 0 && wild > 0) {/*check wild*/
            cardPrice[i]--;
            wild--;
        }
    }
    for (int i = 0; i < 4; i++) {
        if (cardPrice[i] > 0) {/*can not buy*/
            return 0;
        }
    }
    return 1;
}

/*
 *how many wild need buy a card
 *Parameter:
 *  card -> the card wants to buy
 *  player -> who buy this card
 *Return:
 *  ? -> number of wilds
 * */
int* tokens_need(Cards* card, Player* player) {
    int* need = (int*) calloc(5, sizeof(int));
    int cardPrice[4];
    int tokens[5];
    int discountcpy[4];
    for (int i = 0; i < 4; i++) {

        cardPrice[i] = card->price[i];
        tokens[i] = player->tokens[i];
        discountcpy[i] = player->discount[i];
    }
    tokens[4] = player->tokens[4];
    for (int i = 0; i < 4; i++) {
        while (discountcpy[i] != 0 && cardPrice[i] != 0) {
            cardPrice[i]--;
            discountcpy[i]--;
        }
        while (cardPrice[i] != 0 && tokens[i] != 0) {
            cardPrice[i]--;
            tokens[i]--;
            need[i]++;
        }
        while (cardPrice[i] != 0) {
            cardPrice[i]--;
            tokens[4]--;
            need[4]++;
        }
    }
    return need;
}

/*
 *cal sum of array which size is 4
 *Paramter:
 *  price[4] -> the array
 *Return:
 *  ? -> sum
 * */
int cal_sum(int price[4]) {
    int tmp = 0;
    for (int i = 0; i < 4; i++) {
        tmp += price[i];
    }
    return tmp;
}

/**
 * can take yellow?
 * @param tokens
 * @return
 */
int is_yellow_can_take(int tokens[4]) {
    if (tokens[2] > 0) {
        return 1;
    }
    return 0;

}

/**
 * can take brown?
 * @param tokens
 * @return
 */
int is_brown_can_take(int tokens[4]) {
    if (tokens[1] > 0) {
        return 1;
    }
    return 0;

}

/**
 * can take purple
 * @param tokens
 * @return
 */
int is_purple_can_take(int tokens[4]) {
    if (tokens[0] > 0) {
        return 1;
    }
    return 0;

}

/**
 * can take red
 * @param tokens
 * @return
 */
int is_red_can_take(int tokens[4]) {
    if (tokens[3] > 0) {
        return 1;
    }
    return 0;

}
