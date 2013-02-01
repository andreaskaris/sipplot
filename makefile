all:
	gcc -Wall sipgrep.c sipgraph.c sipdump.c -lcurses -lpthread -losip2 -losipparser2 -lpcap  -o sipgrep
debug:
	gcc -Wall -g sipgrep.c sipgraph.c sipdump.c -lcurses -lpthread -losip2 -losipparser2 -lpcap  -o sipgrep
clean:
	rm sipgrep
