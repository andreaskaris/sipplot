all:
	gcc -Wall sipgrep.c sipgraph.c -lcurses -lpthread -o sipgrep
clean:
	rm sipgrep
