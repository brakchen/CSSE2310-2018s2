
OBJ=-Wall -pedantic -g -std=gnu99
#====================test===========================#
#player:player.h player.c error.o tools.o player.o card.o
#	gcc $(OBJ) -o player error.o tools.o card.o  player.o 
#shenzi:shenzi.c shenzi.h tools.o error.o player.o shenzi.o
#	gcc $(OBJ) -o shenzi card.o error.o tools.o player.o shenzi.o
#===================================================#



#################exectuable file################



austerity:card.o error.o tools.o austerity.o player.o banzai.o banzai  ed.o ed shenzi.o shenzi
#austerity:card.o error.o tools.o austerity.o player.o  ed.o ed shenzi.o shenzi
	gcc $(OBJ) -o austerity card.o error.o tools.o player.o austerity.o

#shenzi
#shenzi.i:shenzi.c shenzi.h error.o tools.o player.o
#	gcc $(OBJ) -E -c shenzi.c

player.i:player.c player.h tools.o error.o card.o 
	gcc $(OBJ) -E -c player.c
#####################obj########################
#austerity obj
austerity.o:austerity.c austerity.h error.o tools.o player.o card.o shenzi.o ed.o banzai.o
#austerity.o:austerity.c austerity.h error.o tools.o player.o card.o shenzi.o ed.o
	gcc $(OBJ) -c austerity.c

#error obj
error.o:error.c error.h
	gcc $(OBJ) -c error.c 

#player obj
player.o:player.c player.h tools.o error.o card.o 
	gcc $(OBJ) -c player.c
#card obj
card.o:card.c card.h error.o tools.o
	gcc $(OBJ) -c card.c

#tools obj
tools.o:tools.c tools.h error.o
	gcc $(OBJ) -c tools.c

#shenzi
shenzi.o:shenzi.c shenzi.h  card.o error.o tools.o player.o
	gcc $(OBJ) -c shenzi.c

shenzi:shenzi.c shenzi.h tools.o error.o card.o player.o shenzi.o
	gcc $(OBJ) -o shenzi card.o error.o tools.o player.o shenzi.o

#player banzai
banzai.o:banzai.c banzai.h error.o tools.o player.o
	gcc $(OBJ) -c banzai.c
banzai:banzai.c banzai.o error.o player.o tools.o error.o
	gcc $(OBJ) -o banzai card.o player.o error.o tools.o banzai.o
#player ed
ed.o:ed.c ed.h error.o tools.o player.o
	gcc $(OBJ) -c ed.c
ed:ed.c ed.o error.o player.o tools.o error.o
	gcc $(OBJ) -o ed card.o player.o error.o tools.o ed.o
##############################################
# cleanup
clean:
	rm -rf *.o

