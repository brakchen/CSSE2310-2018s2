#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdarg.h>

#ifndef _TOOLS_H_
#define _TOOLS_H_

#define STDERR_FD 2
#define BUF_SIZE 1024
#define TXT_SIZE 1024
#define READ 0
#define WRITE 1


int is_number(const char);

int is_capital_letter(const char);

int is_lowercase_letter(const char);

int is_letter(const char);

int is_all_numbers(char*);

int has_space(char*);

int has_comma(char* str);

int is_comma_or_colon(char);

int has_newline_char(char* str);

int is_comma(char);

int letter_to_id(char letter);

char id_to_letter(int id);

int is_colon(char);

void remove_newline_char(char* txt);

#endif
