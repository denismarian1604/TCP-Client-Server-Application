#include "server_utils.h"

int main(int argc, char *argv[]) {
    DIE(argc < 2, "No port specified\n");

    // No buffering
    DIE(setvbuf(stdout, NULL, _IONBF, BUFSIZ) < 0, "Failed trying to set no buffering\n");

    int udp_sock_fd, tcp_sock_fd;
    initialize_sockets(&udp_sock_fd, &tcp_sock_fd, atoi(argv[1]));

    // Make fds reusable
    const int enable = 1;
    DIE(setsockopt(tcp_sock_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0, "Failed setting SO_REUSEADDR\n");
    DIE(setsockopt(udp_sock_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0, "Failed setting SO_REUSEADDR\n");

    int num_sockets = 0;
    struct pollfd *fds = initialize_poll_fds(&num_sockets, udp_sock_fd, tcp_sock_fd);

    int n_subscribers = 0;
    subscriber *subscribers = NULL;

    // The server is on unless it receives an exit command
    uint8_t server_status = 1;

    while (1 && server_status) {
        int rc = poll(fds, num_sockets, -1);
        DIE(rc < 0, "Failed poll\n");

        for (int i = 0; i < num_sockets; i++) {
            if (fds[i].revents & POLLIN) {
                // Check if the fd is the UDP socket
                if (fds[i].fd == udp_sock_fd) {
                    handle_udp_message(udp_sock_fd, subscribers, n_subscribers);
                    // Check if the fd is the TCP socket; only for new connections
                } else if(fds[i].fd == tcp_sock_fd) {
                    handle_tcp_message(tcp_sock_fd, &subscribers, &n_subscribers, &fds, &num_sockets);

                    fds[num_sockets - 1].revents = 0;

                    // Check if the fd is the STDIN
                } else if (fds[i].fd == STDIN_FILENO) {
                    handle_stdin_message(&server_status);

                    // If the server is no longer on, break the loop
                    if (!server_status) {
                        break;
                    }
                    // Check if the fd is a client socket
                } else {
                    handle_client_message(fds[i].fd, subscribers, n_subscribers, &fds, &num_sockets);
                }
            }
        }
    }

    // Close all the sockets
    for (int i = 0; i < num_sockets; i++) {
        // For all the clients, send a message that the server is closing
        if (i > 2) {
            tcp_message message;
            message.type = 'X';
            send_all(fds[i].fd, message);
        }

        close(fds[i].fd);
    }

    // Free the arrays
    free(fds);
    free(subscribers);

    return 0;
}
