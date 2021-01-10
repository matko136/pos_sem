all: Klient Server
clean: rm -f Klient Server	
Klient: Klient.c
	gcc -o Klient -lpthread Klient.c
Server: Server.c
	gcc -o Server -lpthread Server.c	