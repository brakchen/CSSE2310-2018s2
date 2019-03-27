#include "shenzi.h"

static int shenzi = 0;

/**
 * main
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
    shenzi_game(player, desk, self);
    for (int i = 0; i < totalPlayer; i++) {
        free(player[i]);
    }
    free(player);
    free(desk->tokens);
    free(desk);
    //free_list(desk->board);
    return 0;
}

/**
 * shenzi main game
 * @param player
 * @param desk
 * @param self
 */
void shenzi_game(Player** player, Desk* desk, int self) {
    int rev = 0, doWhat = -1;
    char** buffer = NULL;
    while (1) {
        buffer = get_read(&rev);
        if (buffer == NULL) {
            sleep(1);
            /*blcking io*/
            /*TODO:NEED to CHECK*/
            continue;
        }
        if (rev > 0) {/*ok there are messages*/
            for (int i = 0; i < rev; i++) {
                shenzi++;
                char* code = buffer[i];
                doWhat = parsing_message(code);
                switch (doWhat) {
                    case HUB_DOWHAT:/*dowhat*/
                        do_what(desk, player[self], code);
                        break;
                    case HUB_EOG:/*game over*/
                        output_eog(desk, player, self);
                        exit(0);
                        return;
                    case -1:/*error message*/
                        free(desk);
                        free(player);
                        player_error(BAD_PIPE, NULL);
                        break;
                    default:/*invalid message handle on parent class*/
                        update_desk_info(desk, code);
                        update_player_info(player, desk, code);
                        output_board_status(desk);
                        output_player_status(player, desk->totalPlayer, self);
                        break;
                }
                free(code);/*finish*/
            }
        } else if (rev == 0) {/*empty buffer*/
            free(buffer);
            player_error(BAD_PIPE, NULL);/*conmmnication error*/
        }
    }
}

/**
 * response do what
 */

void do_what(Desk* desk, Player* player, char* code) {
    Cards* board = desk->board, * card = NULL;
    char* message = NULL;
    int flag = 0;/*is finish dowhat 1=yes 0=no*/
    int buyWhich;
    if ((buyWhich = buy_which_card(board, player)) != -1) {
        //print_chain(board,player->indicator);
        card = copy(find_element(board, buyWhich));
        int* tokensNeed = tokens_need(card, player);
        message = construct_purchase_message(card, tokensNeed);
        do_write(message);
        free(tokensNeed);
        free(card);
        refresh_id(board);
        flag = 1;
    }
    if (!flag) {
        if (can_take_token(desk->tokens)) {
            int* tmp = take_token(desk->tokens);
            message = construct_take_message(tmp);
            do_write(message);
            free(tmp);
        } else {
            message = construct_wild_message();
            do_write(message);
        }

    }
    free(message);
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
int can_take_token(int token[4]) {

    int chance[4] = {-1, -1, -1, -1};
    int count = 0;
    for (int i = 0; i < 4; i++) {
        if (token[i] <= 0) {
            chance[i] = 0;
        } else {
            chance[i] = 1;
        }
    }
    for (int i = 0; i < 3; i++) {
        if (chance[i] == 1) {
            count++;
        }
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
    int copy[4];
    for (int i = 0; i < 3; i++) {
        copy[i] = tokens[i];
    }
    int* tmp = (int*) calloc(4, sizeof(int));
    int count = 0;
    for (int i = 0; i < 4; i++) {
        if (tokens[i] != 0) {
            tmp[i]++;
            copy[i]--;
            count++;
        }
        if (count == 3) {
            break;
        }
    }


    return tmp;

}

/**
 * reorder worth
 * @param board
 * @return
 */
int* ordered_worth(Cards* board) {
    int worth[8] = {-1, -1, -1, -1, -1, -1, -1, -1,};
    int count = get_card_amount(board);;
    int* cardNum = (int*) calloc(count, sizeof(int));
    int swap;
    int swapCard;
    Cards* card = board;
    while (card != NULL) {
        worth[card->No] = card->worth;
        cardNum[card->No] = card->No;
        card = card->next;
    }
    for (int i = 0; i <= count; i++) {
        for (int j = i + 1; j < count; j++) {
            if (worth[i] < worth[j]) {
                swap = worth[i];
                swapCard = cardNum[i];
                worth[i] = worth[j];
                cardNum[i] = cardNum[j];
                worth[j] = swap;
                cardNum[j] = swapCard;
            }
        }

    }
    return cardNum;

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
int buy_which_card(Cards* board, Player* player) {
    Cards* card = NULL;
    int canBuyCount = 0;
    int* mostWorth = find_most_worth(board, player);
    for (int i = 0; i < 8; i++) {
        if (mostWorth[i] != -1) {
            canBuyCount++;
        }
    }
    if (canBuyCount == 1) {
        int tmp = mostWorth[0];
        free(mostWorth);
        return tmp;
    }
    /*more than one most worth*/
    int* mostCheapCard = find_cheapest(board, player, mostWorth);
    int mostCheapCount = 0;
    for (int i = 0; i < 8; i++) {
        if (mostCheapCard[i] != -1) {
            mostCheapCount++;
        }
    }
    if (mostCheapCount == 1) {
        int tmp = mostCheapCard[0];
        free(mostCheapCard);

        return tmp;
    }
    /*in most worth most cheap there still have more*/
    int recently = -1;
    for (int i = 0; i < mostCheapCount; i++) {
        card = find_element(board, mostCheapCard[i]);
        card = copy(card);
        discount_reduce(card->price, player);
        if (card->No > recently) {
            recently = card->No;
        }
    }

    free(mostWorth);
    free(mostCheapCard);
    return recently;

}

/**
 * find the most cheapest one
 * @param board
 * @param player
 * @param mostWorth
 * @return
 */
int* find_cheapest(Cards* board, Player* player, int* mostWorth) {

    int* mostCheapCard = (int*) calloc(8, sizeof(int));;
    int canBuyCount = 0;
    Cards* card = NULL;
    int index = 0;
    for (int i = 0; i < 8; i++) {
        mostCheapCard[i] = -1;
    }
    for (int i = 0; i < 8; i++) {
        if (mostWorth[i] != -1) {
            canBuyCount++;
        }
    }
    int mostCheapCount = 0;
    int price = 1000000;
    for (int i = 0; i < canBuyCount; i++) {
        card = find_element(board, mostWorth[i]);
        card = copy(card);
        if (cal_sum(card->price) < price) {
            price = cal_sum(card->price);
        }
        free(card);
    }
    for (int i = 0; i < canBuyCount; i++) {
        card = find_element(board, mostWorth[i]);
        card = copy(card);
        discount_reduce(card->price, player);
        if (cal_sum(card->price) <= price) {
            mostCheapCard[index++] = mostWorth[i];
            mostCheapCount++;
        }
        free(card);
    }
    return mostCheapCard;

}

/**
 * finr the most worth one
 * @param board
 * @param player
 * @return
 */
int* find_most_worth(Cards* board, Player* player) {
    int* orderWorth = ordered_worth(board);
    Cards* card = NULL;
    int canBuyCount = 0;
    int index = 0;
    int* mostWorth = (int*) calloc(8, sizeof(int));
    for (int i = 0; i < 8; i++) {
        mostWorth[i] = -1;
    }
    int maxWorth = 0;
    for (int i = 0; i < get_card_amount(board); i++) {
        card = find_element(board, orderWorth[i]);
        card = copy(card);
        if (can_buy_a_card(card, player)) {
            if (card->worth > maxWorth) {
                maxWorth = card->worth;
            }
        }
        free(card);
    }

    for (int i = 0; i < get_card_amount(board); i++) {
        card = find_element(board, orderWorth[i]);
        card = copy(card);
        discount_reduce(card->price, player);
        if (can_buy_a_card(card, player)) {
            if (card->worth >= maxWorth) {
                mostWorth[index++] = orderWorth[i];
                canBuyCount++;
            }
        }
        free(card);
    }
    return mostWorth;
}

void discount_reduce(int price[4], Player* player) {
    int discountcpy[4];

    for (int i = 0; i < 3; i++) {
        discountcpy[i] = player->discount[i];

    }
    for (int i = 0; i < 3; i++) {
        while (price[i] != 0 && discountcpy[i] != 0) {
            price[i]--;

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
