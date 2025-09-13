#ifndef SHAM_H
#define SHAM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <stdarg.h>
#include <errno.h>

// --- Protocol Constants ---

// Maximum size of the data payload in a packet
#define MAX_DATA_SIZE 1024
// Retransmission Timeout in milliseconds
#define RTO_MS 500
// Sender's fixed sliding window size in packets (for congestion control simulation)
#define SENDER_WINDOW_SIZE 10
// Receiver's buffer size in bytes (for flow control)
#define RECEIVER_BUFFER_SIZE 65536 // 64 KB


// --- S.H.A.M. Header and Packet Flags ---
#define FLAG_SYN 0x1
#define FLAG_ACK 0x2
#define FLAG_FIN 0x4
#define FLAG_DATA 0x8 // Custom flag to differentiate pure ACKs from data packets

// S.H.A.M. Header Structure
// Using __attribute__((packed)) to prevent compiler padding
struct sham_header {
    uint32_t seq_num;     // Sequence Number
    uint32_t ack_num;     // Acknowledgment Number
    uint16_t flags;       // Control flags (SYN, ACK, FIN)
    uint16_t window_size; // Flow control window size
} __attribute__((packed));

// S.H.A.M. Packet Structure
struct sham_packet {
    struct sham_header header;
    char data[MAX_DATA_SIZE];
};


// --- Logging Globals and Functions ---

// Global variables for logging
extern FILE* log_fp;
extern int logging_enabled;

/**
 * @brief Initializes the logging system.
 * Checks for the RUDP_LOG=1 environment variable and opens the appropriate log file.
 * @param role "client" or "server" to determine the log file name.
 */
void init_logging(const char* role);

/**
 * @brief Closes the log file if it was opened.
 */
void close_logging();

/**
 * @brief Writes a formatted, timestamped message to the log file.
 * Uses a variadic function to support printf-style formatting.
 * @param format The format string.
 * @param ... The arguments for the format string.
 */
void log_event(const char *format, ...);

/**
 * @brief Creates and configures a UDP socket.
 * @param port The port to bind to. If 0, an ephemeral port is used (for client).
 * @return The socket file descriptor, or -1 on error.
 */
int create_udp_socket(int port);

/**
 * @brief Decides whether to drop a packet based on a given probability.
 * @param loss_rate The probability of dropping a packet (0.0 to 1.0).
 * @return 1 if the packet should be dropped, 0 otherwise.
 */
int should_drop_packet(float loss_rate);


#endif // SHAM_H
