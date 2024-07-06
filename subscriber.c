#include "subscriber_utils.h"

int main(int argc, char *argv[]) {

    DIE(argc < 4, "Not enough arguments provided. Usage : ./subscriber <ID_CLIENT> <IP_SERVER> <PORT_SERVER>\n");

    // No buffering
    DIE(setvbuf(stdout, NULL, _IONBF, BUFSIZ) < 0, "Failed trying to set no buffering\n");

    struct sockaddr_in tcp_sock;
    int tcp_sock_fd;

    // Initialize the socket
    initialize_socket(&tcp_sock_fd, &tcp_sock, inet_addr(argv[2]), atoi(argv[3]));

    // Connect to the server
    connect_to_server(tcp_sock_fd, &tcp_sock);

    // Send the client ID to the server
    send_id_to_server(tcp_sock_fd, argv[1]);

    // Receive the message from the server
    tcp_message message = receive_message_from_server(tcp_sock_fd);

    switch (message.type) {
        case 'A':
        {
            // Connection was successful
            break;
        }
        case 'N':
        {
            // Connection was unsuccessful
            close(tcp_sock_fd);
            return 0;
        }
        default:
            break;
    }

    int num_sockets = 2;
    // Initialize the poll fds
    struct pollfd *fds = initialize_poll_fds(tcp_sock_fd);

    while(1) {
        int rc = poll(fds, num_sockets, -1);
        DIE(rc < 0, "Failed poll\n");

        if (fds[0].revents & POLLIN) {
            // Check if the fd is the STDIN
            handle_tcp_message(tcp_sock_fd);
        }

        if (fds[1].revents & POLLIN) {
            // Check if the fd is the TCP socket
            handle_stdin_message(tcp_sock_fd);
        }
    }

    // Close the socket
    close(tcp_sock_fd);

    return 0;
}
