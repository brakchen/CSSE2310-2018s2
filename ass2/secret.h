#ifndef SECRET_H
#define SECRET_H
#include <stdbool.h>

// Publically visible interfaces with hidden implmentations



typedef bool (*strfn)(const char*);

extern strfn* fnarray;				// how big is this array?

void bombSetup();

strfn getPhase(const char* name);


// reports marks and current attempts etc
// should only be called when not making a guess
void getStatus();

// When mute is on, append functions won't actually modify the string
void mute(bool enable);

// If mute is on, turn it off, if it is off, then turn it on
void muteFlip(void);

// sets secret string to the empty string
void clearString();

// append char to string
void appendChar(char c);

void appendInt(int z);

void appendString(const char* s);

// tests if the secret string matches s
bool stringMatches(const char* s);

// appendChar calls will (silently) fail after maxchars have been added [clearString resets the count] 
void stringCap(int maxchars);

// returns the next character in an unknown sequence
char blackBoxC();

// Expects a char 0..9 A..Z a..z
char guard0(char x0);
char guard1(char x1);
char guard2(char x2);
char guard3(char x3);
char guard4(char x4);
char guard5(char x5);
char guard6(char x6);
char guard7(char x7);


extern int bseed;

#endif
