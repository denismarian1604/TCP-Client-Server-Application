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

#include "structs.h"

int create_udp_socket();
int create_tcp_socket();

void send_all(int socket, tcp_message message);

void initialize_sockets(int *udp_sock_fd, int *tcp_sock_fd, in_port_t port);
struct pollfd *initialize_poll_fds(int *num_sockets, int udp_sock_fd, int tcp_sock_fd);

void handle_stdin_message(uint8_t *server_status);
void handle_udp_message(int udp_sock_fd, subscriber *subscribers, int num_subscribers);
void handle_tcp_message(int tcp_sock_fd, subscriber **subscribers, int *num_subscribers, struct pollfd **fds, int *num_sockets);
void handle_client_message(int socket, subscriber *subscribers, int num_subscribers, struct pollfd **fds, int *num_sockets);