#include "banzai.h"


/**
 *  main
 * @param args
 * @param argv
 * @return
 */
int main(int args, char** argv) {
    //pipe_message_handle(argv);
    /*TODO:check vaild*/
    check_player_args(args, argv, __FILE__);
    int totalPlayer = atoi(argv[1]);
    int self = atoi(argv[2]);
    char indicator = argv[2][0];
    Player** player = get_player_objects(indicator);
    Desk* desk = get_desk_object(totalPlayer);
    banzai_game(player, desk, self);
    for (int i = 0; i < totalPlayer; i++) {

        free(player[i]->name);
        free(player[i]);
    }
    free(player);
    //free_list(desk->board);
    free(desk->board);
    free(desk);
    return 0;
}

/**
 * main start
 * @param player
 * @param desk
 * @param self
 */
void banzai_game(Player** player, Desk* desk, int self) {
    int rev = 0, doWhat = -1;
    char** buffer = NULL;
    while (1) {
        buffer = get_read(&rev);
        if (buffer == NULL) {
            /*blcking io*/
            /*TODO:NEED to CHECK*/
            continue;
        }
        if (rev > 0) {/*ok there are messages*/
            for (int i = 0; i < rev; i++) {
                char* code = buffer[i];
                doWhat = parsing_message(code);
                switch (doWhat) {
                    case HUB_DOWHAT:/*dowhat*/
                        do_what(desk, player[self]);
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
                        //output_board_status(desk);
                        output_player_status(player, desk->totalPlayer, self);
                        break;
                }
                free(buffer[i]);/*finish*/
            }
        } else if (rev == 0) {/*empty buffer*/
            continue;
            player_error(BAD_PIPE, NULL);/*conmmnication error*/
        }
    }
    free(buffer);
}

/**
 * response dowhat
 * @param desk
 * @param player
 */
void do_what(Desk* desk, Player* player) {
    char* message = NULL;
    int buyWhich = 0;
    Cards* card = NULL;
    Cards* board = desk->board;
    if (is_need_tokens(player)) {/*take tokens*/
        if (can_take_token(desk->tokens)) {/*take tokens*/
            int* tmp = take_token(desk->tokens);
            message = construct_take_message(tmp);
            do_write(message);
            free(tmp);
            return;
        }

    } else {
        if ((buyWhich = buy_which_card(board, player)) != -1) {
            //print_chain(board,player->indicator);
            card = copy(find_element(board, buyWhich));
            int* tokensNeed = tokens_need(card, player);
            message = construct_purchase_message(card, tokensNeed);
            do_write(message);
            free(tokensNeed);
            free(card);
            refresh_id(board);
        } else {
            message = construct_wild_message();
            do_write(message);

        }
    }


    free(message);
}

/**
 * is need take token
 * @param player
 * @return 1 -> to get a tokens,0 -> no dont go
 */
/*

 * */
int is_need_tokens(Player* player) {
    int sum = 0;
    for (int i = 0; i < 4; i++) {
        sum += player->tokens[i];
    }
    if (sum >= 3) {
        return 0;
    }

    if (player->tokens[4] >= 3) {
        return 0;
    }
    return 1;
}

/**
 * can take token?
 * @param tokens tokens[4] -> tokens from board
 * @return 1 -> yes,0->no
 */

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
 * can take bron?
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
 * can take red?
 * @param tokens
 * @return
 */
int is_red_can_take(int tokens[4]) {
    if (tokens[3] > 0) {
        return 1;
    }
    return 0;

}

/**
 *  take tokens
 * @param tokens tokens -> tokens in board
 * @return int*:the tokens needed to take
 */

int* take_token(int tokens[4]) {
    int order[4] = {2, 1, 0, 3};
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
 * reorder price
 * @param board
 * @return
 */
int* ordered_price(Cards* board) {
    int count = get_card_amount(board);;
    int price[8] = {-1, -1, -1, -1, -1, -1, -1, -1,};
    int* cardNum = (int*) calloc(count, sizeof(int));
    int swap = 0;
    int swapCard = 0;
    Cards* card = board;
    while (card != NULL) {
        price[card->No] = cal_sum(card->price);
        cardNum[card->No] = card->No;
        card = card->next;
    }
    for (int i = 0; i <= count; i++) {
        for (int j = i + 1; j < count; j++) {
            if (price[i] < price[j]) {
                swap = price[i];
                swapCard = cardNum[i];
                price[i] = price[j];
                cardNum[i] = cardNum[j];
                price[j] = swap;
                cardNum[j] = swapCard;
            }
        }

    }
    return cardNum;

}

/**
 *from cardList decise buy which card
 *Parameters:
 *  board -> all cards
 *  canBuyList->cardList ,a list of card could be buy
 *Return:
 * -1 -> there is not card can buy.It is impossible value bug check needed
 *  ? -> card number
 */
int buy_which_card(Cards* board, Player* player) {
    int* orderPrice = ordered_price(board);
    Cards* card = NULL;
    int canBuyCount = 0;
    int index = 0;
    int mostExpensive[8] = {-1, -1, -1, -1, -1, -1, -1, -1,};

    /*can buy any most expensive?*/
    for (int i = 0; i < get_card_amount(board); i++) {
        card = find_element(board, orderPrice[i]);
        card = copy(card);
        if (can_buy_a_card(card, player)) {
            if (get_card_amount(board) > 0 && card->worth != 0) {
                mostExpensive[index++] = orderPrice[i];
                canBuyCount++;
            }
        }
        free(card);
    }
    if (canBuyCount == 1) {
        free(orderPrice);
        return mostExpensive[0];
    } else if (canBuyCount == 0) {
        return -1;
    }
    /*which waste most wildtokend*/
    int mostWildNeeded[8] = {-1, -1, -1, -1, -1, -1, -1, -1,};
    int mostWildCount = 0;
    int wildNeed = 0;
    int* tokensNeed = NULL;
    for (int i = 0; i < canBuyCount; i++) {
        card = find_element(board, mostExpensive[i]);
        card = copy(card);
        tokensNeed = tokens_need(card, player);
        if (tokensNeed[4] > wildNeed) {
            wildNeed = tokensNeed[4];
        }
        free(card);
        free(tokensNeed);
    }
    for (int i = 0; i < canBuyCount; i++) {
        card = find_element(board, mostExpensive[i]);
        card = copy(card);
        tokensNeed = tokens_need(card, player);
        if (tokensNeed[4] == wildNeed) {
            mostWildNeeded[mostWildCount++] = mostExpensive[i];
        }
        free(card);
        free(tokensNeed);
    }

    if (mostWildCount == 1) {
        free(orderPrice);
        return mostWildNeeded[0];
    }
    /*in most worth most cheap there still have more*/
    int recently = 7;
    for (int i = 0; i < mostWildCount; i++) {
        card = find_element(board, mostWildNeeded[i]);
        card = copy(card);
        if (card->No < recently) {
            recently = card->No;
        }
    }

    free(orderPrice);
    return recently;

}

/**
 *bug checked? no
 *check if shenzi can buy this card
 * @param card
 * @param player
 * @return 1 -> yes,0 -> no
 */
int can_buy_a_card(Cards* card, Player* player) {
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

/**
 * how many wild need buy a card
 * @param card he card wants to buy
 * @param player who buy this card
 * @return ? -> number of wilds
 */

int* tokens_need(Cards* card, Player* player) {
    int* need = (int*) calloc(5, sizeof(int));
    int cardPrice[4];
    int tokens[5];
    int discount[4];
    for (int i = 0; i < 4; i++) {

        cardPrice[i] = card->price[i];
        tokens[i] = player->tokens[i];
        discount[i] = player->discount[i];
    }
    tokens[4] = player->tokens[4];
    for (int i = 0; i < 4; i++) {
        while (cardPrice[i] != 0 && (tokens[i] != 0 || discount[i] != 0)) {
            if (discount[i] != 0) {
                cardPrice[i]--;
                discount[i]--;
            } else {
                cardPrice[i]--;
                tokens[i]--;
                need[i]++;
            }
        }
    }
    for (int i = 0; i < 4; i++) {
        while (cardPrice[i] != 0) {
            cardPrice[i]--;
            tokens[4]--;
            need[4]++;
        }
    }
    return need;
}

/**
 * cal sum of array which size is 4
 * @param price price[4] -> the array
 * @return ? -> sum
 */
int cal_sum(int price[4]) {
    int tmp = 0;
    for (int i = 0; i < 4; i++) {
        tmp += price[i];
    }
    return tmp;
}
