#include "tools.h"

/**
 * check if is number
 * @param ch
 * @return
 */
int is_number(const char ch) {
    if (ch > 57 || ch < 48)
        return 0;

    return 1;
}

/**
 * check if is PBYR
 * @param ch
 * @return
 */
int is_PBYR(char ch) {
    switch (ch) {
        case 'P':
            return 1;
        case 'B':
            return 1;
        case 'Y':
            return 1;
        case 'R':
            return 1;
        default:
            return 0;
    }

}

/**
 * check if is capital letter
 * @param ch
 * @return
 */
int is_capital_letter(const char ch) {
    if (ch > 90 || ch < 65)
        return 0;
    return 1;


}

/**
 * check if is lower case letter
 */
int is_lowercase_letter(const char ch) {
    if (ch > 122 || ch < 97)
        return 0;
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
    for (int i = 0; i < strlen(ch); i++)
        if (!is_number(ch[i]))
            return 0;

    return 1;
}

/**
 * check if is comma or colon
 * @param str
 * @return
 */
int is_comma_or_colon(char str) {
    if (str == ',' || str == ':')
        return 1;

    return 0;
}

/*
 * check if is comma
 */
int is_comma(char str) {
    if (str == ',')
        return 1;

    return 0;
}

/**
 * check if is colon
 * @param str
 * @return
 */
int is_colon(char str) {
    if (str == ':')
        return 1;

    return 0;
}

int* count_comma_and_colon(char* str) {
    int* count = (int*) calloc(2, sizeof(int));
    int comma = 0;
    int colon = 0;
    for (int i = 0; i < strlen(str); i++) {
        if (is_comma(str[i])) {
            comma++;
        }
        if (is_colon(str[i])) {
            colon++;
        }
    }
    count[0] = comma;
    count[1] = colon;
    return count;


}

