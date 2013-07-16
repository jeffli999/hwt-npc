SRC=main.c common.c bitchunk.c

all: $(SRC)
	gcc -g $(SRC) -o h-npc
