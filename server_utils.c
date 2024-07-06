#include "server_utils.h"

// Function used for sending all informations
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

// Function used for receiving all informations
int recv_all(int socket, void *message, size_t size_to_recv) {
    // If we wait for the client's ID, we know the size of the message
    if (size_to_recv == 11) {
        int bytes_received = recv(socket, message, size_to_recv, 0);
        DIE(bytes_received < 0, "Failed recv\n");

        return bytes_received;
    }

    size_t total_received = 0, remaining = size_to_recv;
    uint8_t *buffer = (uint8_t *)message;

    while (total_received < size_to_recv) {
        int bytes_received = recv(socket, buffer, remaining, 0);
        DIE(bytes_received < 0, "Failed recv\n");

        if (bytes_received == 0) {
            return 0;
        }

        remaining -= bytes_received;
        total_received += bytes_received;
        buffer += bytes_received;
    }

    return total_received;
}

// Function used to create an UDP socket
int create_udp_socket() {
    int udp_sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    DIE(udp_sock_fd < 0, "Failed creating UDP socket\n");

    return udp_sock_fd;
}

// Function used to create a TCP socket
int create_tcp_socket() {
    int tcp_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(tcp_sock_fd < 0, "Failed creating TCP socket\n");

    return tcp_sock_fd;
}

// Function used to initialize the sockets
void initialize_sockets(int *udp_sock_fd, int *tcp_sock_fd, in_port_t port) {
    // Create the sockets
    *udp_sock_fd = create_udp_socket();
    *tcp_sock_fd = create_tcp_socket();

    // Bind the UDP socket
    struct sockaddr_in udp_addr;
    memset((void *) &udp_addr, 0, sizeof(udp_addr));
    
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_port = htons(port);
    udp_addr.sin_addr.s_addr = INADDR_ANY;

    DIE(bind(*udp_sock_fd, (struct sockaddr *) &udp_addr, (socklen_t)sizeof(udp_addr)) < 0, "Failed bind on udp socket\n");

    // Bind the TCP socket
    struct sockaddr_in tcp_addr;
    memset((void *) &tcp_addr, 0, sizeof(tcp_addr));

    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_port = htons(port);
    tcp_addr.sin_addr.s_addr = INADDR_ANY;

    DIE(bind(*tcp_sock_fd, (struct sockaddr *) &tcp_addr, sizeof(tcp_addr)) < 0, "Failed bind on tcp socket\n");

    // Deactivate Nagle's algorithm
    int one = 1;

    DIE(setsockopt(*tcp_sock_fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one)) < 0, "Failed setsockopt\n");

    // Listen on the TCP socket
    DIE(listen(*tcp_sock_fd, __INT_MAX__) < 0, "Failed listen on tcp socket\n");
}

// Function used to initialize the poll fds
struct pollfd *initialize_poll_fds(int *num_sockets, int udp_sock_fd, int tcp_sock_fd) {
    // Allocate a fds array, initially with 3 elements (UDP and TCP sockets and STDIN)
    struct pollfd *fds = calloc(3, sizeof(struct pollfd));
    *num_sockets = 3;

    // Add the UDP socket to the fds array
    fds[0].fd = udp_sock_fd;
    fds[0].events = POLLIN;

    // Add the TCP socket to the fds array
    fds[1].fd = tcp_sock_fd;
    fds[1].events = POLLIN;

    // Add the STDIN to the fds array
    fds[2].fd = STDIN_FILENO;
    fds[2].events = POLLIN;

    return fds;
}

// Function used to handle the STDIN message
void handle_stdin_message(uint8_t *server_status) {
    char buffer[100];
    memset(buffer, 0, 100);

    // Read from STDIN
    DIE(read(STDIN_FILENO, buffer, 100) < 0, "Failed read from STDIN\n");

    // If the message is "exit", then exit the program
    if (strncmp(buffer, "exit", 4) == 0) {
        *server_status = 0;
    }
}

// Function to check if a topic matches another given topic
int match_topic(char *user_topic, char *searched_topic) {
    // As the user topic may be as a wild card, we have to check for a pattern match
    char *user_topic_copy = strdup(user_topic);
    char *searched_topic_copy = strdup(searched_topic);

    char *user_topic_tokens[1000] = {NULL};
    char *searched_topic_tokens[1000] = {NULL};

    // Get the user topic tokens
    int cnt = 0;
    char *c = strtok(user_topic_copy, "/");
    while (c != NULL) {
        user_topic_tokens[cnt++] = c;
        c = strtok(NULL, "/");
    }

    int user_tokens = cnt;

    // Get the searched topic tokens
    cnt = 0;
    c = strtok(searched_topic_copy, "/");
    while (c != NULL) {
        searched_topic_tokens[cnt++] = c;
        c = strtok(NULL, "/");
    }

    int searched_tokens = cnt;

    // Check if the topics match
    int cnt_user = 0, cnt_searched = 0;

    while (cnt_user < user_tokens && cnt_searched < searched_tokens) {
        if (strcmp(user_topic_tokens[cnt_user], searched_topic_tokens[cnt_searched]) == 0 ||
            strcmp(user_topic_tokens[cnt_user], "+") == 0) {
            cnt_user++, cnt_searched++;
        } else if (strcmp(user_topic_tokens[cnt_user], "*") == 0) {
            // Iterate through searched_topic_tokens until we found the next token
            // from the user_topic_tokens
            cnt_user++;

            // If '*' is the last token, the topics match
            if (cnt_user == user_tokens) {
                return 1;
            }

            while (cnt_searched < searched_tokens &&
                    cnt_user < user_tokens &&
                    strcmp(user_topic_tokens[cnt_user], searched_topic_tokens[cnt_searched]) != 0) {
                cnt_searched++;
            }
        } else {
            // If the tokens do not match, the topics do not match
            return 0;
        }
    }

    if (cnt_user == user_tokens && cnt_searched == searched_tokens) {
        return 1;
    } else {
        return 0;
    }
}

// Function used to check if a subscriber is subscribed to a topic
int is_subscribed_to_topic(subscriber subscriber, char *topic) {
    for (int i = 0; i < subscriber.n_topics; i++) {
        if (match_topic(subscriber.topics[i], topic)) {
            return 1;
        }
    }

    return 0;
}

// Function used to send the TCP message to the subscribed users
int send_to_users(tcp_message tcp_msg_to_send, subscriber *subscribers, int num_subscribers) {
    for (int i = 0; i < num_subscribers; i++) {
        if (is_subscribed_to_topic(subscribers[i], tcp_msg_to_send.topic)) {
            // Send the message to the subscriber
            if (subscribers[i].connected) {
                send_all(subscribers[i].socket, tcp_msg_to_send);
            }
        }
    }

    // If no user is subscribed to the topic, return success
    return 1;
}

// Function used to handle the UDP message
void handle_udp_message(int udp_sock_fd, subscriber *subscribers, int num_subscribers) {
    char buffer[MAX_PACKET_LEN + 1];

    // Receive the UDP message
    struct sockaddr_in udp_addr;
    memset((void *) &udp_addr, 0, sizeof(udp_addr));
    socklen_t udp_addr_len = sizeof(udp_addr);

    recvfrom(udp_sock_fd, buffer, MAX_PACKET_LEN, 0, (struct sockaddr *)&udp_addr, (socklen_t *)&udp_addr_len);

    udp_message *recvd_message = (udp_message *) buffer;
    tcp_message tcp_msg_to_send;

    // Fill in the TCP message struct
    memset(&tcp_msg_to_send, 0, sizeof(tcp_msg_to_send));
    memcpy(tcp_msg_to_send.ip, inet_ntoa(udp_addr.sin_addr), IP_SIZE);
    tcp_msg_to_send.port = ntohs(udp_addr.sin_port);
    memcpy(tcp_msg_to_send.topic, recvd_message->topic, TOPIC_LEN);
    tcp_msg_to_send.type = recvd_message->type;

    // Based on the type of the UDP message, fill in the TCP message struct
    switch (recvd_message->type) {
        // INT case
        case 0:
        {
            uint8_t negative = (uint8_t)recvd_message->data[0];
            uint32_t recvd_int = ntohl(*(uint32_t *)((uint8_t *)recvd_message->data + 1));

            (negative == 1) ? (recvd_int *= -1) : (recvd_int *= 1);
            
            sprintf(tcp_msg_to_send.data, "%d", recvd_int);
            break;
        }
        // SHORT_REAL case
        case 1:
        {
            uint16_t recvd_short_real = ntohs(*(uint16_t *)recvd_message->data);

            sprintf(tcp_msg_to_send.data, "%.2f", (recvd_short_real * 1.0) / 100);
            break;
        }
        // FLOAT case
        case 2:
        {
            uint8_t negative = recvd_message->data[0];
            // Store it on double to avoid eventual overflow
            double recvd_float = ntohl(*(uint32_t *)((void *)recvd_message->data + 1));
            uint8_t neg_exp = recvd_message->data[5];

            uint32_t power, i;
            for (i = 1, power = 1; i <= neg_exp; i++, power *= 10);

            recvd_float /= power;

            (negative == 1) ? (recvd_float *= -1) : (recvd_float *= 1);

            sprintf(tcp_msg_to_send.data, "%lf", recvd_float);
            break;
        }
        // STRING case
        case 3:
        {
            memcpy(tcp_msg_to_send.data, recvd_message->data, MAX_LEN);
            break;
        }
        // Shouldn't get here
        default:
        {
            write(STDERR_FILENO, "Invalid type of message\n", 25);
            break;
        }
    }

    // Send the TCP message to the subscribed users
    DIE(send_to_users(tcp_msg_to_send, subscribers, num_subscribers) < 0, "Failed send_to_users\n");
}

// Function used to find a subscriber in the list of subscribers
subscriber *find_subscriber_by_id(subscriber *subscribers, int num_subscribers, char *id) {
    for (int i = 0; i < num_subscribers; i++) {
        if (strcmp(subscribers[i].id, id) == 0) {
            return &subscribers[i];
        }
    }

    return NULL;
}

// Function used to find a subscriber by socket
subscriber *find_subscriber_by_socket(subscriber *subscribers, int num_subscribers, int socket) {
    for (int i = 0; i < num_subscribers; i++) {
        if (subscribers[i].socket == socket) {
            return &subscribers[i];
        }
    }

    return NULL;
}

// Function used to add a new fd in the poll fds
void add_new_fd_in_poll(struct pollfd **fds, int *num_sockets, int new_fd) {
    // Allocate memory for the new fds array
    *fds = realloc(*fds, (*num_sockets + 1) * sizeof(struct pollfd));
    DIE(*fds == NULL, "Failed realloc\n");

    // Increase the number of sockets
    (*num_sockets)++;

    // Fill in the new pollfd struct
    (*fds)[*num_sockets - 1].fd = new_fd;
    (*fds)[*num_sockets - 1].events = POLLIN;
}

// Function used to remove a fd from the poll fds
void remove_fd_from_poll(struct pollfd **fds, int *num_sockets, int fd) {
    for (int i = 0; i < *num_sockets; i++) {
        if ((*fds)[i].fd == fd) {
            for (int j = i + 1; j < *num_sockets; j++) {
                (*fds)[j - 1] = (*fds)[j];
            }

            (*num_sockets)--;
            *fds = realloc(*fds, *num_sockets * sizeof(struct pollfd));
            DIE(*fds == NULL, "Failed realloc\n");

            break;
        }
    }
}

// Function used to add a new client to the list of subscribers(clients)
void add_new_subscriber(subscriber **subscribers, int *num_subscribers, char *id, int socket) {
    // Allocate memory for the new subscriber
    if (*num_subscribers == 0) {
        *subscribers = calloc(1, sizeof(subscriber));
        DIE(*subscribers == NULL, "Failed calloc\n");
    } else {
        *subscribers = realloc(*subscribers, (*num_subscribers + 1) * sizeof(subscriber));
        DIE(*subscribers == NULL, "Failed realloc\n");
    }

    // Increase the number of subscribers
    (*num_subscribers)++;

    // Fill in the subscriber struct
    subscriber *new_subscriber = &(*subscribers)[*num_subscribers - 1];

    new_subscriber->connected = 1;
    new_subscriber->socket = socket;

    memcpy(new_subscriber->id, id, ID_SIZE + 1);

    new_subscriber->n_topics = 0;
    new_subscriber->topics = NULL;
}

// Function used to send client the confirmation of connection
void send_client_conn_reply(char type, int client_socket) {
    tcp_message ack_message;
    memset(&ack_message, 0, sizeof(tcp_message));

    ack_message.type = type;

    send_all(client_socket, ack_message);
}

// Function used to handle the TCP message
void handle_tcp_message(int tcp_sock_fd, subscriber **subscribers, int *num_subscribers, struct pollfd **fds, int *num_sockets) {
    struct sockaddr_in tcp_addr;
    socklen_t tcp_addr_len = sizeof(tcp_addr);
    int conn_fd = accept(tcp_sock_fd, (struct sockaddr *)&tcp_addr, &tcp_addr_len);

    // Receive the client's ID
    char buf[11];
    memset(buf, 0, 11);
    recv_all(conn_fd, buf, 11);

    // Look if the ID is already used
    subscriber *found_subscriber = find_subscriber_by_id(*subscribers, *num_subscribers, buf);
    if (found_subscriber) {
        if (found_subscriber->connected) {
            printf("Client %s already connected.\n", buf);

            // Send a message to the client to let him know the connection was unsuccessful
            send_client_conn_reply('N', conn_fd);

            close(conn_fd);
        } else {
            // If the subscriber existed disconnected and now he reconnected
            // update the connection status and the socket and update the poll fds

            // Log the reconnection
            printf("New client %s connected from %s:%d.\n", buf, inet_ntoa(tcp_addr.sin_addr), ntohs(tcp_addr.sin_port));
            found_subscriber->connected = 1;
            found_subscriber->socket = conn_fd;

            // Update the poll fds
            add_new_fd_in_poll(fds, num_sockets, conn_fd);

            // Send a message to the client to let him know the connection was successful
            send_client_conn_reply('A', conn_fd);
        }
    } else {
        // If the client is new, log it, add him to the list of subscribers
        // and update the poll fds
        printf("New client %s connected from %s:%d.\n", buf, inet_ntoa(tcp_addr.sin_addr), ntohs(tcp_addr.sin_port));

        add_new_subscriber(subscribers, num_subscribers, buf, conn_fd);

        // Update the poll fds
        add_new_fd_in_poll(fds, num_sockets, conn_fd);

        // Send a message the to client to let him know the connection was successful
        send_client_conn_reply('A', conn_fd);
    }
}

// Function used to subscribe a user to a topic
void subscribe_to_topic(subscriber *current_subscriber, char *topic) {
    // Allocate memory for the new topic
    if (current_subscriber->n_topics == 0) {
        current_subscriber->topics = calloc(1, sizeof(char *));
        DIE(current_subscriber->topics == NULL, "Failed calloc\n");
    } else {
        current_subscriber->topics = realloc(current_subscriber->topics, (current_subscriber->n_topics + 1) * sizeof(char *));
        DIE(current_subscriber->topics == NULL, "Failed realloc\n");
    }

    // Increase the number of topics
    current_subscriber->n_topics++;

    // Add the new topic to the list
    current_subscriber->topics[current_subscriber->n_topics - 1] = strdup(topic);
}

// Function used to unsubscribe a user from a topic
void unsubscribe_from_topic(subscriber *current_subscriber, char *topic) {
    int pos = -1;
    for (int i = 0; i < current_subscriber->n_topics; i++)
        if (match_topic(current_subscriber->topics[i], topic)) {
            pos = i;
            break;
        }
    
    // If the topic is found in the users' list, remove it
    // And decrement the number of topics the user is subscribed to
    if (pos != -1) {
        for (int i = pos + 1; i < current_subscriber->n_topics; i++)
            current_subscriber->topics[i - 1] = current_subscriber->topics[i];
        
        current_subscriber->n_topics--;

        // If the user is not subscribed to any topic, free the memory allocated for the topics
        if (current_subscriber->n_topics == 0) {
            free(current_subscriber->topics);
            current_subscriber->topics = NULL;
        } else {
            // If the user is still subscribed to some topics, reallocate the array
            // to free the memory allocated for the removed topic
            current_subscriber->topics = realloc(current_subscriber->topics, current_subscriber->n_topics * sizeof(char *));
            DIE(current_subscriber->topics == NULL, "Failed realloc\n");
        
        }
    }
}

// Function used to handle client requests
void handle_client_message(int socket, subscriber *subscribers, int num_subscribers, struct pollfd **fds, int *num_sockets) {
    char buf[MAX_PACKET_LEN + 1];
    memset(buf, 0, MAX_PACKET_LEN);

    int recvd_len = recv_all(socket, (void *)buf, sizeof(tcp_message));

    // Find the subscriber associated with the given socket
    subscriber *current_subscriber = find_subscriber_by_socket(subscribers, num_subscribers, socket);

    // Get the client request in tcp format
    tcp_message *client_request = (tcp_message *) buf;

    // A sudden disconnect occurred
    if (recvd_len <= 0 && current_subscriber->socket != -1) {
        printf("Client %s disconnected.\n", current_subscriber->id);

        current_subscriber->connected = 0;
        current_subscriber->socket = -1;

        close(socket);

        // Update the poll fds
        remove_fd_from_poll(fds, num_sockets, socket);

        return;
    } else if (current_subscriber->socket == -1) {
        printf("Client %s disconnected.\n", current_subscriber->id);
    }

    // Shouldn't get here
    if (current_subscriber == NULL) {
        write(STDERR_FILENO, "Client not found\n", 18);
        return;
    } else {
        switch(client_request->type) {
            case '+': // User wants to subscribe to a new topic
            {
                // Remove the newline character from the end of the topic
                client_request->topic[strlen(client_request->topic) - 1] = '\0';

                subscribe_to_topic(current_subscriber, client_request->topic);
                break;
            }
            case '-' : // User wants to unsubscribe from a topic
            {
                // Remove the newline character from the end of the topic
                client_request->topic[strlen(client_request->topic) - 1] = '\0';

                unsubscribe_from_topic(current_subscriber, client_request->topic);
                break;
            }
            case '0' : // User wants to disconnect
            {
                printf("Client %s disconnected.\n", current_subscriber->id);

                current_subscriber->connected = 0;
                current_subscriber->socket = -1;

                close(socket);

                // Update the poll fds
                remove_fd_from_poll(fds, num_sockets, socket);

                break;
            }
            default : // Shouldn't get here
            {
                write(STDERR_FILENO, "Invalid type of message\n", 25);
                break;
            }
        }
    }
}
