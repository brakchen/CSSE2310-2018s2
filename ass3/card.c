#include "card.h"

/**
 * read a card from file
 * @param deck desk obj
 * @param fileName  filename
 */
void read_cards(Cards** deck, const char* fileName) {
    const char* mode = "r";
    char ch, tmp[100] = {'\0'};
    int* cache = (int*) calloc(50, sizeof(int));
    int j = 0;
    FILE* fp = NULL;
    int flag = 0;
    fp = fopen(fileName, mode);
    if (fp == NULL) {/*error*/
        hub_error(BAD_FILE);
    }
    Cards* card = (Cards*) calloc(1, sizeof(Cards));
    while ((ch = fgetc(fp)) != EOF) {
        if (ch != '\n') {/*new line*/
            tmp[j++] = ch;
        } else {
            if (!is_card_valid(tmp)) {/*invalid should raise error*/
                hub_error(BAD_CONTENT);
            }
            split(&cache, tmp, 1);
            card->color = (char) cache[0];
            card->worth = cache[1];
            for (int i = 2, j = 0; i < 6; i++) {
                card->price[j++] = cache[i];
            }
            card->No = flag;
            memset(tmp, '\0', 100);
            add_element(deck, card);
            j = 0;
            card = (Cards*) calloc(1, sizeof(Cards));
            flag++;
        }
    }
    if (flag == 0) {/*error : empty file*/
        hub_error(BAD_CONTENT);
    }
    free(cache);
}

/**
 * get amount of card in board
 * @param deck
 * @return
 */
int get_card_amount(Cards* deck) {
    int count = 0;
    Cards* cards;
    cards = deck;
    while (cards != NULL) {
        cards = cards->next;
        count++;
    }

    return count;
}

/**
 * get amout of input cards
 * -formated- input eg:B:1:1,0,0,0|Y:0:0,3,0,2
 * @param cards
 * @return The restul of explame will return 2
 */
int get_amount_cards(char* cards) {
    int size = 0, cardsSize = strlen(cards), i = 0;
    while (cardsSize != i) {
        if (cards[i] == '|')
            size++;
        i++;
    }
    return size + 1;
}

/**
 * check is card valid
 * @param card
 * @return
 */
int is_card_valid(char* card) {
    if (strlen(card) <= 0) {
        return 0;
    }
    int size = strlen(card), element = 1, j = 0;
    char* cache = (char*) calloc(100, sizeof(char));
    for (int i = 0; i < size; i++) {/*unify the format*/
        if (is_comma_or_colon(card[i])) {
            card[i] = ',';
        }
        if (card[i] != ',') {/*update data in to cache*/
            cache[j++] = card[i];
        } else {
            cache[j++] = '\0';
            if (element == 1) {/*first element*/
                if (!is_PBYR(cache[0]) || strlen(cache) > 1)
                    /*check is PBYR or len >1*/
                    return 0;

            } else {
                if (!is_all_numbers(cache))
                    return 0;

            }
            j = 0;
            element++;
            memset(cache, '\0', 100);
        }
    }

    if (element != 6) {
        return 0;
    } else {
        if (!is_all_numbers(cache))/*check last one items*/
            return 0;

    }
    free(cache);
    return 1;
}

/**
 * using to split card to a int array
 * @param array
 * @param data
 * @param mode 0 return all int 1 will return 1 char with other int
 */
void split(int** array, char* data, int mode) {
    char tmp[100] = {'\0'};
    int index = 0, j = 0;
    for (int i = 0; i < strlen(data); i++) {
        if (is_comma_or_colon(data[i])) {
            j = 0;
            if (index == 0 && mode) {/*mode 1*/
                (*array)[index++] = (char) tmp[0];
            } else {/*mode 0*/
                if (!is_all_numbers(tmp)) {
                    player_error(BAD_PIPE, NULL);
                }
                (*array)[index++] = atoi(tmp);
            }
            memset(tmp, '\0', 100);
            continue;
        } else {

            if (!is_all_numbers(tmp)) {
                player_error(BAD_PIPE, NULL);
            }
            tmp[j++] = data[i];
        }
    }
    (*array)[index++] = atoi(tmp);
    (*array)[index++] = -1;
}

/**
 * add element in linked list
 * @param node
 * @param element
 */
void add_element(Cards** node, Cards* element) {
    Cards* tmp;
    if (*node != NULL) {
        tmp = *node;
        while (tmp->next != NULL) {
            tmp = tmp->next;
        }
        tmp->next = element;
        element->next = NULL;
    } else {
        *node = element;
        element->next = NULL;
    }

}

/**
 * del a linked list element
 * @param cards the linked list
 * @param position the position of element
 */
void del_element(Cards** cards, int position) {
    Cards* current = *cards;
    Cards* pre = NULL;
    Cards* tmp = NULL;
    while (current != NULL) {
        if (current->No == position) {
            break;
        } else {
            pre = current;
            current = current->next;
        }
    }
    if (pre == NULL) {/*del head*/
        tmp = current;
        current = current->next;
        free(tmp);
        tmp = NULL;
        *cards = current;
    } else {
        tmp = current;
        current = current->next;
        free(tmp);
        tmp = NULL;
        pre->next = current;
    }

}

/**
 * if position is -1 then insert at the tail.
 * otherwise,insert in position.
 * @param cards
 * @param position
 * @param element
 */
void insert_element(Cards* cards, int position, Cards* element) {
    Cards* current = cards;
    Cards* pre = NULL;
    while (1) {
        if (position == -1) {/*insert in to tail*/
            if (current == NULL) {
                pre->next = element;
                break;
            } else {
                pre = current;
                current = current->next;
            }
        } else {/*insert into certain positon*/
            if (current->No != position) {
                pre = current;
                current = current->next;
                continue;
            } else {
                pre->next = element;
                element->next = current;
                break;
            }
        }
    }

    refresh_id(cards);
}

/**
 * find a element that in the linked list
 * @param cards the linked list
 * @param position the card position
 * @return position==-1 return the last one
 *         postiion==? return the other that on ?
 */
Cards* find_element(Cards* cards, int position) {
    Cards* current = cards;
    Cards* pre = NULL;
    while (1) {
        if (position == -1) {
            pre = current;
            if ((current = current->next) == NULL) {
                pre->next = NULL;
                return pre;
            }
        } else {
            if (current->No == position) {
                return current;
            }
            current = current->next;

        }
    }

}

/**
 * get last one elment from cards
 * the function will remove the last one from original cards
 * @param cards
 * @return
 */
Cards* pop_last_one(Cards* cards) {
    Cards* current = cards;
    Cards* pre = NULL;
    while (1) {
        pre = current;
        current = current->next;
        if (current->next == NULL) {
            pre->next = NULL;
            return current;
        }
    }
}

/**
 * pop the first one
 * @param cards
 * @return
 */
Cards* pop_first_one(Cards** cards) {
    Cards* current = *cards;
    Cards* pre = NULL;
    while (1) {
        pre = current;
        current = current->next;
        pre->next = NULL;
        *cards = current;
        refresh_id(*cards);
        return pre;
    }
}

/**
 * free list
 * @param cards
 */
void free_list(Cards* cards) {
    Cards* current = cards;
    Cards* pre = NULL;
    while (current != NULL) {
        pre = current;
        current = pre->next;
        free(pre);
    }

}

/**
 * refresh id
 * @param cards
 */
void refresh_id(Cards* cards) {
    int flag = 0;
    while (cards != NULL) {
        cards->No = flag;
        cards = cards->next;
        flag++;
    }
}
