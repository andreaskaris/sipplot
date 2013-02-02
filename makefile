all:
	gcc -Wall sipplot.c sipgraph.c sipdump.c -lcurses -lpthread -losip2 -losipparser2 -lpcap  -o sipplot
debug:
	gcc -Wall -g sipplot.c sipgraph.c sipdump.c -lcurses -lpthread -losip2 -losipparser2 -lpcap  -o sipplot
clean:
	rm sipplot
