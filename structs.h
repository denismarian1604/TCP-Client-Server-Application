#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <stdlib.h>

/*
 * Macro de verificare a erorilor
 * Exemplu:
 * 		int fd = open (file_name , O_RDONLY);
 * 		DIE( fd == -1, "open failed");
 */

// Gracefully copied from the lab
#define DIE(assertion, call_description)                                       \
  do {                                                                         \
    if (assertion) {                                                           \
      fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);                       \
      perror(call_description);                                                \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

#define MAX_LEN 1500
#define TOPIC_LEN 50

#define MAX_PACKET_LEN 1572
#define UDP_PACKET_SIZE 1552

#define IP_SIZE 16
#define ID_SIZE 10

typedef struct udp {
    char topic[TOPIC_LEN];
    uint8_t type;
    char data[MAX_LEN + 1];
} udp_message;

typedef struct tcp {
    char ip[IP_SIZE];
    uint16_t port;
    char topic[TOPIC_LEN + 2];
    uint8_t type;
    char data[MAX_LEN + 1];
} tcp_message;

typedef struct client {
    uint8_t connected;
    uint32_t socket;

    char id[ID_SIZE + 1];

    uint32_t n_topics;
    char **topics;
} subscriber;

typedef struct client_request {
    uint8_t type;
    char topic[TOPIC_LEN + 1];
} client_request;

#endif
