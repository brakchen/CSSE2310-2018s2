#include "tools.h"

/**
 * check if is number
 * @param ch
 * @return
 */
int is_number(const char ch) {
    if (ch > 57 || ch < 48) {
        return 0;
    }

    return 1;
}


/**
 * check if is capital letter
 * @param ch
 * @return
 */
int is_capital_letter(const char ch) {
    if (ch > 90 || ch < 65) {
        return 0;
    }
    return 1;


}

/**
 * check if is lower case letter
 */
int is_lowercase_letter(const char ch) {
    if (ch > 122 || ch < 97) {
        return 0;
    }
    return 1;
}

/**
 * check is letter
 * @param ch
 * @return
 */
int is_letter(const char ch) {
    if (is_capital_letter(ch) || is_lowercase_letter(ch)) {
        return 1;
    }
    return 0;
}

/*
 * check if is all numbers
 */
int is_all_numbers(char* ch) {
    for (int i = 0; i < strlen(ch); i++) {
        if (!is_number(ch[i])) {
            return 0;

        }
    }

    return 1;
}

/**
 * check if is comma or colon
 * @param str
 * @return
 */
int is_comma_or_colon(char str) {
    if (str == ',' || str == ':') {
        return 1;
    }

    return 0;
}

/*
 * check if is comma
 */
int is_comma(char str) {
    if (str == ',') {
        return 1;
    }

    return 0;
}

/**
 * check if is colon
 * @param str
 * @return
 */
int is_colon(char str) {
    if (str == ':') {
        return 1;
    }

    return 0;
}

/* check if there is a space in str
 * @param str
 * @return 1 if yes,0 if no
 *
 *
 */
int has_space(char* str) {
    for (int i = 0; i < strlen(str); i++) {
        if (str[i] == ' ') {
            return 1;
        }
    }
    return 0;
}

/* check if there is a \n in str
 * @param str
 * @return 1 if yes,0 if no
 *
 *
 */
int has_newline_char(char* str) {

    for (int i = 0; i < strlen(str); i++) {
        if (str[i] == '\n') {
            return 1;
        }
    }
    return 0;
}

/* check if there is a comma in str
 * @param str
 * @return 1 if yes,0 if no
 *
 *
 */
int has_comma(char* str) {

    for (int i = 0; i < strlen(str); i++) {
        if (str[i] == ',') {
            return 1;
        }
    }
    return 0;
}

/* conver letter to id
 * @param letter
 * @return the letter's id otherwise error and return -1 
 *
 *
 */
int letter_to_id(char letter) {
    char lowCase[] = {"abcdefghijklmnopqrstuvwxyz"};
    char upperCase[] = {"ABCDEFGHIJKLMNOPQRSTUVWXYZ"};
    for (int i = 0; i < 26; i++) {
        if (letter == lowCase[i]) {
            return i;
        }
        if (letter == upperCase[i]) {
            return i;
        }
    }
    return -1;
}

/*convert id to letter*/
char id_to_letter(int id) {

    char upperCase[] = {"ABCDEFGHIJKLMNOPQRSTUVWXYZ"};
    return upperCase[id];
}

/* this is help function that
 * remove the last one new line char
 * replace with \0
 * no return value
 * */
void remove_newline_char(char* txt) {
    for (; ; ) {
        int len = strlen(txt);
        if (txt[len - 1] == '\n' || txt[len - 1] == '\r') {
            txt[len - 1] = '\0';
        } else {
            break;
        }
    }
}
