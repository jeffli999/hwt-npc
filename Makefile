SRC=main.c common.c bitband.c rule.c trie.c hwt.c

all: $(SRC)
	gcc -g -O2 $(SRC) -o main
