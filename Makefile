CC = gcc
CFLAGS = -Wall -Werror
OBJ_OPTION = -c

build: server subscriber

server: server_utils.o server.o
	$(CC) $(CFLAGS) $^ -o server

server_utils.o: server_utils.c
	$(CC) $(CFLAGS) $(OBJ_OPTION) $< -o $@

server.o: server.c
	$(CC) $(CFLAGS) $(OBJ_OPTION) $< -o $@

subscriber: subscriber_utils.o subscriber.o
	$(CC) $(CFLAGS) $^ -o $@

subscriber_utils.o: subscriber_utils.c
	$(CC) $(CFLAGS) $(OBJ_OPTION) $< -o $@

subscriber.o: subscriber.c
	$(CC) $(CFLAGS) $(OBJ_OPTION) $< -o $@

make clean:
	rm -f server subscriber server_utils.o subscriber_utils.o server.o subscriber.o

zip:
	zip -r 324CB_Denis-Marian_Vladulescu_Tema2.zip Makefile structs.h server.c server_utils.c server_utils.h subscriber.c subscriber_utils.c subscriber_utils.h readme.txt