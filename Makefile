SRC=main.c common.c bitband.c rule.c

all: $(SRC)
	gcc -g $(SRC) -o hwt-npc
