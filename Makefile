SRC=main.c common.c bitband.c rule.c trie.c hwt.c stats.c

all: $(SRC)
	gcc -g $(SRC) -o main
