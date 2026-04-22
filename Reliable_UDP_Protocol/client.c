#include "sham.h"
#include <errno.h>

// Global Logging Variables (Definition)
FILE* log_fp = NULL;
int logging_enabled = 0;

// Function Prototypes
void handle_file_transfer(int sockfd, struct sockaddr_in *server_addr, const char *input_file, const char *output_name);
void handle_chat_mode(int sockfd, struct sockaddr_in *server_addr);
void send_packet(int sockfd, struct sockaddr_in *addr, struct sham_packet *pkt, size_t size);

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  File Transfer: %s <server_ip> <server_port> <input_file> <output_file_name> [loss_rate]\n", argv[0]);
        fprintf(stderr, "  Chat Mode:     %s <server_ip> <server_port> --chat [loss_rate]\n", argv[0]);
        return 1;
    }

    const char *server_ip = argv[1];
    int server_port = atoi(argv[2]);
    int chat_mode = 0;
    float loss_rate = 0.0;
    const char *input_file = NULL;
    const char *output_file_name = NULL;

    // Argument parsing
    if (strcmp(argv[3], "--chat") == 0) {
        chat_mode = 1;
        if (argc > 4) loss_rate = atof(argv[4]);
    } else {
        if (argc < 5) {
            fprintf(stderr, "Error: Missing input/output file names for file transfer mode.\n");
            return 1;
        }
        input_file = argv[3];
        output_file_name = argv[4];
        if (argc > 5) loss_rate = atof(argv[5]);
    }

    // Initialize logging
    init_logging("client");

    // Create socket
    int sockfd = create_udp_socket(0); // 0 for ephemeral port
    if (sockfd < 0) {
        close_logging();
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("inet_pton failed");
        close(sockfd);
        close_logging();
        return 1;
    }

    srand(time(NULL)); // Seed for random numbers

    // Step 1: Three-Way Handshake (Establishment)
    uint32_t client_isn = rand() % 10000;
    struct sham_packet syn_pkt, syn_ack_pkt;

    // Step 2: Send SYN
    memset(&syn_pkt, 0, sizeof(syn_pkt));
    syn_pkt.header.seq_num = client_isn;
    syn_pkt.header.flags = FLAG_SYN;
    send_packet(sockfd, &server_addr, &syn_pkt, sizeof(struct sham_header));
    log_event("SND SYN SEQ=%u", client_isn);

    // Step 3: Receive SYN-ACK
    socklen_t addr_len = sizeof(server_addr);
    if (recvfrom(sockfd, &syn_ack_pkt, sizeof(syn_ack_pkt), 0, (struct sockaddr *)&server_addr, &addr_len) < 0) {
        perror("recvfrom SYN-ACK failed");
        close(sockfd);
        close_logging();
        return 1;
    }
    log_event("RCV SYN-ACK SEQ=%u ACK=%u", syn_ack_pkt.header.seq_num, syn_ack_pkt.header.ack_num);

    if ((syn_ack_pkt.header.flags & (FLAG_SYN | FLAG_ACK)) && (syn_ack_pkt.header.ack_num == client_isn + 1)) {
        // Send final ACK
        struct sham_packet ack_pkt;
        memset(&ack_pkt, 0, sizeof(ack_pkt));
        ack_pkt.header.seq_num = client_isn + 1;
        ack_pkt.header.ack_num = syn_ack_pkt.header.seq_num + 1;
        ack_pkt.header.flags = FLAG_ACK;
        send_packet(sockfd, &server_addr, &ack_pkt, sizeof(struct sham_header));
        log_event("SND ACK FOR SYN SEQ=%u ACK=%u", ack_pkt.header.seq_num, ack_pkt.header.ack_num);
        printf("Connection established.\n");
    } else {
        fprintf(stderr, "Handshake failed.\n");
        close(sockfd);
        close_logging();
        return 1;
    }
    
    // Dispatch to correct mode
    if (chat_mode) {
        handle_chat_mode(sockfd, &server_addr);
    } else {
        handle_file_transfer(sockfd, &server_addr, input_file, output_file_name);
    }

    close(sockfd);
    close_logging();
    return 0;
}

void handle_file_transfer(int sockfd, struct sockaddr_in *server_addr, const char *input_file, const char *output_name) {
    FILE *fp = fopen(input_file, "rb");
    if (!fp) {
        perror("fopen input_file failed");
        return;
    }

    // First, send the output filename to the server
    struct sham_packet name_pkt;
    memset(&name_pkt, 0, sizeof(name_pkt));
    name_pkt.header.seq_num = 10001; // Arbitrary starting sequence for file metadata
    name_pkt.header.flags = FLAG_DATA;
    strncpy(name_pkt.data, output_name, MAX_DATA_SIZE - 1);
    send_packet(sockfd, server_addr, &name_pkt, sizeof(struct sham_header) + strlen(output_name) + 1);
    log_event("SND DATA SEQ=%u LEN=%zu (filename)", name_pkt.header.seq_num, strlen(output_name) + 1);

    // Simple wait for ACK for filename
    recvfrom(sockfd, &name_pkt, sizeof(name_pkt), 0, NULL, NULL);
    log_event("RCV ACK=%u", name_pkt.header.ack_num);

    printf("Starting file transfer...\n");
    
    // File Sending Logic with Sliding Window
    
    uint32_t seq_num = 20000; // Start data sequence numbers
    char buffer[MAX_DATA_SIZE];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, MAX_DATA_SIZE, fp)) > 0) {
        struct sham_packet data_pkt;
        memset(&data_pkt, 0, sizeof(data_pkt));
        data_pkt.header.seq_num = seq_num;
        data_pkt.header.flags = FLAG_DATA;
        memcpy(data_pkt.data, buffer, bytes_read);

        int acked = 0;
        while (!acked) {
            send_packet(sockfd, server_addr, &data_pkt, sizeof(struct sham_header) + bytes_read);
            log_event("SND DATA SEQ=%u LEN=%zu", seq_num, bytes_read);

            // Set a timeout for receiving an ACK
            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = RTO_MS * 1000;
            setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
            
            struct sham_packet ack_pkt;
            ssize_t n = recvfrom(sockfd, &ack_pkt, sizeof(ack_pkt), 0, NULL, NULL);

            if (n > 0) {
                if ((ack_pkt.header.flags & FLAG_ACK) && ack_pkt.header.ack_num == seq_num + bytes_read) {
                    log_event("RCV ACK=%u", ack_pkt.header.ack_num);
                    acked = 1;
                    seq_num += bytes_read; // Move to next sequence number
                }
            } else {
                log_event("TIMEOUT SEQ=%u", seq_num);
                log_event("RETX DATA SEQ=%u LEN=%zu", seq_num, bytes_read);
            }
        }
    }
    
    // Four-Way Handshake (Termination)
    struct sham_packet fin_pkt, ack_pkt, peer_fin_pkt;
    
    // Send FIN
    memset(&fin_pkt, 0, sizeof(fin_pkt));
    fin_pkt.header.flags = FLAG_FIN;
    fin_pkt.header.seq_num = seq_num;
    send_packet(sockfd, server_addr, &fin_pkt, sizeof(struct sham_header));
    log_event("SND FIN SEQ=%u", fin_pkt.header.seq_num);

    // Receive ACK for FIN
    recvfrom(sockfd, &ack_pkt, sizeof(ack_pkt), 0, NULL, NULL);
    log_event("RCV ACK FOR FIN");
    
    // Receive FIN from peer
    recvfrom(sockfd, &peer_fin_pkt, sizeof(peer_fin_pkt), 0, NULL, NULL);
    log_event("RCV FIN SEQ=%u", peer_fin_pkt.header.seq_num);

    // Send final ACK
    memset(&ack_pkt, 0, sizeof(ack_pkt));
    ack_pkt.header.flags = FLAG_ACK;
    ack_pkt.header.ack_num = peer_fin_pkt.header.seq_num + 1;
    send_packet(sockfd, server_addr, &ack_pkt, sizeof(struct sham_header));
    log_event("SND ACK=%u", ack_pkt.header.ack_num);
    
    printf("File transfer complete. Connection closed.\n");
    fclose(fp);
}

void handle_chat_mode(int sockfd, struct sockaddr_in *server_addr) {
    printf("Chat mode activated. Type '/quit' to exit.\n");

    fd_set read_fds;
    uint32_t seq = 40000, peer_seq = 0; // Track our sequence number

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(sockfd, &read_fds);
        int max_fd = sockfd > STDIN_FILENO ? sockfd + 1 : STDIN_FILENO + 1;

        int activity = select(max_fd, &read_fds, NULL, NULL, NULL);
        if (activity < 0 && errno != EINTR) {
            perror("select error");
            break;
        }

        // --- Check for network input ---
        if (FD_ISSET(sockfd, &read_fds)) {
            struct sham_packet pkt;
            ssize_t n = recvfrom(sockfd, &pkt, sizeof(pkt), 0, NULL, NULL);
            
            if (n <= 0) {
                printf("Server disconnected unexpectedly.\n");
                break;
            }

            // Server initiated termination
            if (pkt.header.flags & FLAG_FIN) {
                log_event("RCV FIN SEQ=%u", pkt.header.seq_num);
                printf("Server initiated termination. Closing connection.\n");
                
                struct sham_packet ack_pkt;
                memset(&ack_pkt, 0, sizeof(ack_pkt));
                ack_pkt.header.flags = FLAG_ACK;
                ack_pkt.header.ack_num = pkt.header.seq_num + 1;
                send_packet(sockfd, server_addr, &ack_pkt, sizeof(struct sham_header));
                log_event("SND ACK FOR FIN");

                struct sham_packet fin_pkt;
                memset(&fin_pkt, 0, sizeof(fin_pkt));
                fin_pkt.header.flags = FLAG_FIN;
                fin_pkt.header.seq_num = seq;
                send_packet(sockfd, server_addr, &fin_pkt, sizeof(struct sham_header));
                log_event("SND FIN SEQ=%u", seq++);

                recvfrom(sockfd, &pkt, sizeof(pkt), 0, NULL, NULL);
                if (pkt.header.flags & FLAG_ACK) {
                    log_event("RCV ACK=%u", pkt.header.ack_num);
                }
                break; 
            }
            
            if (pkt.header.flags & FLAG_DATA) {
                // Calculate actual data length and null-terminate the buffer
                ssize_t data_len = n - sizeof(struct sham_header);
                if (data_len < MAX_DATA_SIZE) {
                    pkt.data[data_len] = '\0'; // Ensure it is null-terminated
                }
                
                log_event("RCV DATA SEQ=%u LEN=%ld", pkt.header.seq_num, data_len);
                printf("Server: %s\n", pkt.data);
                fflush(stdout);
            }
        }
        
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            char buffer[MAX_DATA_SIZE];
            if (fgets(buffer, sizeof(buffer), stdin)) {
                if (strncmp(buffer, "/quit", 5) == 0) {
                    printf("Disconnecting...\n");
                    
                    struct sham_packet fin_pkt;
                    memset(&fin_pkt, 0, sizeof(fin_pkt));
                    fin_pkt.header.flags = FLAG_FIN;
                    fin_pkt.header.seq_num = seq;
                    send_packet(sockfd, server_addr, &fin_pkt, sizeof(struct sham_header));
                    log_event("SND FIN SEQ=%u", seq++);

                    struct sham_packet rcv_pkt;
                    recvfrom(sockfd, &rcv_pkt, sizeof(rcv_pkt), 0, NULL, NULL);
                    if (rcv_pkt.header.flags & FLAG_ACK) {
                         log_event("RCV ACK FOR FIN");
                    }
                    
                    recvfrom(sockfd, &rcv_pkt, sizeof(rcv_pkt), 0, NULL, NULL);
                    if (rcv_pkt.header.flags & FLAG_FIN) {
                        log_event("RCV FIN SEQ=%u", rcv_pkt.header.seq_num);
                        peer_seq = rcv_pkt.header.seq_num;
                    }

                    struct sham_packet final_ack_pkt;
                    memset(&final_ack_pkt, 0, sizeof(final_ack_pkt));
                    final_ack_pkt.header.flags = FLAG_ACK;
                    final_ack_pkt.header.ack_num = peer_seq + 1;
                    send_packet(sockfd, server_addr, &final_ack_pkt, sizeof(struct sham_header));
                    log_event("SND ACK=%u", final_ack_pkt.header.ack_num);

                    break; 
                }

                struct sham_packet data_pkt;
                memset(&data_pkt, 0, sizeof(data_pkt));
                data_pkt.header.seq_num = seq;
                data_pkt.header.flags = FLAG_DATA;
                strncpy(data_pkt.data, buffer, MAX_DATA_SIZE);
                size_t len = strlen(buffer);
                send_packet(sockfd, server_addr, &data_pkt, sizeof(struct sham_header) + len);
                log_event("SND DATA SEQ=%u LEN=%zu", seq, len);
                seq += len;
            }
        }
    }
}

void send_packet(int sockfd, struct sockaddr_in *addr, struct sham_packet *pkt, size_t size) {
    sendto(sockfd, pkt, size, 0, (struct sockaddr*)addr, sizeof(*addr));
}

void init_logging(const char* role) {
    if (getenv("RUDP_LOG") && strcmp(getenv("RUDP_LOG"), "1") == 0) {
        logging_enabled = 1;
        char filename[20];
        snprintf(filename, sizeof(filename), "%s_log.txt", role);
        log_fp = fopen(filename, "w");
        if (!log_fp) {
            perror("fopen log file failed");
            logging_enabled = 0;
        }
    }
}

void close_logging() {
    if (log_fp) {
        fclose(log_fp);
    }
}

void log_event(const char *format, ...) {
    if (!logging_enabled || !log_fp) return;

    char time_buffer[30];
    struct timeval tv;
    time_t curtime;

    gettimeofday(&tv, NULL);
    curtime = tv.tv_sec;
    strftime(time_buffer, 30, "%Y-%m-%d %H:%M:%S", localtime(&curtime));

    fprintf(log_fp, "[%s.%06ld] [LOG] ", time_buffer, tv.tv_usec);
    
    va_list args;
    va_start(args, format);
    vfprintf(log_fp, format, args);
    va_end(args);
    
    fprintf(log_fp, "\n");
    fflush(log_fp); // Ensure log is written immediately
}

int create_udp_socket(int port) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        return -1;
    }

    if (port != 0) {
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            perror("bind failed");
            close(sockfd);
            return -1;
        }
    }
    return sockfd;
}

int should_drop_packet(float loss_rate) {
    if (loss_rate <= 0.0) return 0;
    double r = (double)rand() / (double)RAND_MAX;
    return r < loss_rate;
}
