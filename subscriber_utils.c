#include "subscriber_utils.h"

// Function used for receiving all the data from the server
void recv_all(int tcp_sock_fd, void *message) {
    int bytes_received = 0;
    int total_bytes_received = 0;

    while (1) {
        bytes_received = recv(tcp_sock_fd, message + total_bytes_received, sizeof(tcp_message) - total_bytes_received, 0);
        DIE(bytes_received < 0, "Failed receiving data from server\n");

        total_bytes_received += bytes_received;

        if (total_bytes_received == sizeof(tcp_message)) {
            break;
        }
    }
}

// Function used for sending all the data to the server
void send_all(int socket, tcp_message message) {
    size_t size_to_send = sizeof(tcp_message);
    uint8_t *buffer = (uint8_t *)&message;

    while (size_to_send > 0) {
        int bytes_sent = send(socket, buffer, size_to_send, 0);
        DIE(bytes_sent < 0, "Failed send\n");

        size_to_send -= bytes_sent;
        buffer += bytes_sent;
    }
}

// Function used to create a TCP socket
int create_tcp_socket() {
    int tcp_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(tcp_sock_fd < 0, "Failed creating TCP socket\n");

    return tcp_sock_fd;
}

// Function used to initialize the socket
void initialize_socket(int *tcp_sock_fd, struct sockaddr_in *tcp_sock, in_addr_t ip, in_port_t port) {
    // Create the socket
    *tcp_sock_fd = create_tcp_socket();

    memset((void *) tcp_sock, 0, sizeof(tcp_sock));

    tcp_sock->sin_family = AF_INET;
    tcp_sock->sin_port = htons(port);
    tcp_sock->sin_addr.s_addr = ip;
}

// Function used to connect to the server
void connect_to_server(int tcp_sock_fd, struct sockaddr_in *tcp_sock) {
    DIE(connect(tcp_sock_fd, (struct sockaddr *) tcp_sock, sizeof(*tcp_sock)) < 0, 
        "Failed connecting to server\n");
}

// Function used to send the client ID to the server
void send_id_to_server(int tcp_sock_fd, char *id) {
    DIE(send(tcp_sock_fd, id, strlen(id), 0) < 0, "Failed sending ID to server\n");
}

// Function used to receive the message from the server
tcp_message receive_message_from_server(int tcp_sock_fd) {
    tcp_message message;
    recv_all(tcp_sock_fd, (void *) &message);

    return message;
}

// Function used to initialize the poll fds
struct pollfd *initialize_poll_fds(int tcp_sock_fd) {
    // Allocate a fds array, initially with 2 elements (TCP socket and STDIN)
    struct pollfd *fds = calloc(2, sizeof(struct pollfd));

    // Add the TCP socket to the fds array
    fds[0].fd = tcp_sock_fd;
    fds[0].events = POLLIN;

    // Add the STDIN to the fds array
    fds[1].fd = STDIN_FILENO;
    fds[1].events = POLLIN;

    return fds;
}

// Function used to handle the STDIN message
void handle_stdin_message(int tcp_sock_fd) {
    char buffer[MAX_LEN + 1];
    memset(buffer, 0, MAX_LEN + 1);

    // Read the message from STDIN
    read(STDIN_FILENO, buffer, MAX_LEN);

    // Check if the message is "exit"
    if (strncmp(buffer, "exit", 4) == 0) {
        // Send exit message to the server
        tcp_message message;
        memset(&message, 0, sizeof(tcp_message));

        message.type = '0';
        send_all(tcp_sock_fd, message);

        // Close the TCP socket
        close(tcp_sock_fd);
        exit(0);
    } else if (strncmp(buffer, "subscribe", 9) == 0) {
        // Send subscribe message to the server
        tcp_message message;
        memset(&message, 0, sizeof(tcp_message));

        message.type = '+';
        strcpy(message.topic, buffer + 10);

        send_all(tcp_sock_fd, message);

        // Print the message
        printf("Subscribed to topic %s", message.topic);
    } else if (strncmp(buffer, "unsubscribe", 11) == 0) {
        // Send unsubscribe message to the server
        tcp_message message;
        memset(&message, 0, sizeof(tcp_message));

        message.type = '-';
        strcpy(message.topic, buffer + 12);

        send_all(tcp_sock_fd, message);

        // Print the message
        printf("Unsubscribed from topic %s", message.topic);
    } else {
        // Print the message
        printf("Invalid command\n");
    }
}

// Function used to handle the TCP message
void handle_tcp_message(int tcp_sock_fd) {
    tcp_message message = receive_message_from_server(tcp_sock_fd);

    switch (message.type) {
        case 0:
        {
            // Print the message
            printf("%s:%d - %s - INT - %s\n", message.ip, ntohs(message.port), message.topic, message.data);

            break;
        }
        case 1:
        {
            // Print the message
            printf("%s:%d - %s - SHORT_REAL - %s\n", message.ip, ntohs(message.port), message.topic, message.data);

            break;
        }
        case 2:
        {
            // Print the message
            printf("%s:%d - %s - FLOAT - %s\n", message.ip, ntohs(message.port), message.topic, message.data);

            break;
        }
        case 3:
        {
            // Print the message
            printf("%s:%d - %s - STRING - %s\n", message.ip, ntohs(message.port), message.topic, message.data);

            break;
        }
        case 'X':
        {
            // Close the TCP socket
            close(tcp_sock_fd);
            exit(0);
        }
        default:
            break;
    }
}
