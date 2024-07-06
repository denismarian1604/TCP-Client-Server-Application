#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "structs.h"

void initialize_socket(int *tcp_sock_fd, struct sockaddr_in *tcp_sock, in_addr_t ip, in_port_t port);
void connect_to_server(int tcp_sock_fd, struct sockaddr_in *tcp_sock);
void send_id_to_server(int tcp_sock_fd, char *id);
tcp_message receive_message_from_server(int tcp_sock_fd);

struct pollfd *initialize_poll_fds(int tcp_sock_fd);
void handle_stdin_message(int tcp_sock_fd);
void handle_tcp_message(int tcp_sock_fd);