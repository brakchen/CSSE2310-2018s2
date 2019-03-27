CC=gcc
CFLAGS=-Wall -Wextra -pedantic -std=gnu99 -g -Iinclude -fPIC

.PHONY: all clean

LIBRARY_SOURCES=\
				lib/game.c \
				lib/util.c \
				lib/protocol.c \
				lib/token.c \

AUSTERITY_SOURCES=\
				  austerity.c \

SHENZI_SOURCES=\
			   shenzi.c \
			   player.c \

BANZAI_SOURCES=\
			   banzai.c \
			   player.c \

ED_SOURCES=\
		   ed.c \
		   player.c \

all: \
	liba4.so \
	austerity \
	shenzi \
	banzai \
	ed \

clean:
	rm -f $(LIBRARY_SOURCES:.c=.o) liba4.so
	rm -f $(AUSTERITY_SOURCES:.c=.o) austerity
	rm -f $(SHENZI_SOURCES:.c=.o) shenzi
	rm -f $(BANZAI_SOURCES:.c=.o) banzai
	rm -f $(ED_SOURCES:.c=.o) ed

liba4.so: $(LIBRARY_SOURCES:.c=.o)
	$(CC) $(CFLAGS) -o liba4.so -shared $(LIBRARY_SOURCES:.c=.o)

austerity: $(AUSTERITY_SOURCES:.c=.o) $(LIBRARY_SOURCES:.c=.o)

shenzi: $(SHENZI_SOURCES:.c=.o) $(LIBRARY_SOURCES:.c=.o)

banzai: $(BANZAI_SOURCES:.c=.o) $(LIBRARY_SOURCES:.c=.o)

ed: $(ED_SOURCES:.c=.o) $(LIBRARY_SOURCES:.c=.o)
