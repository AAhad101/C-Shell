#include "sham.h"
#include <openssl/md5.h>

// Global Logging Variables (Definition)
FILE* log_fp = NULL;
int logging_enabled = 0;

// Function Prototypes
void handle_file_transfer(int sockfd, struct sockaddr_in *client_addr, float loss_rate, uint32_t start_seq, uint32_t start_ack);
void handle_chat_mode(int sockfd, struct sockaddr_in *client_addr, float loss_rate);
void calculate_md5(const char *filename);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port> [--chat] [loss_rate]\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    int chat_mode = 0;
    float loss_rate = 0.0;

    if (argc > 2) {
        if (strcmp(argv[2], "--chat") == 0) {
            chat_mode = 1;
            if (argc > 3) loss_rate = atof(argv[3]);
        } else {
            loss_rate = atof(argv[2]);
        }
    }
    
    init_logging("server");

    int sockfd = create_udp_socket(port);
    if (sockfd < 0) {
        close_logging();
        return 1;
    }

    printf("Server listening on port %d...\n", port);
    
    srand(time(NULL));
    
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    struct sham_packet syn_pkt;

    // Step 1: Wait for SYN to start handshake
    printf("Waiting for a client to connect...\n");
    ssize_t n = recvfrom(sockfd, &syn_pkt, sizeof(syn_pkt), 0, (struct sockaddr *)&client_addr, &client_len);

    if (n > 0 && (syn_pkt.header.flags & FLAG_SYN)) {
        log_event("RCV SYN SEQ=%u", syn_pkt.header.seq_num);

        // Step 2: Send SYN-ACK
        uint32_t server_isn = rand() % 50000;
        struct sham_packet syn_ack_pkt;
        memset(&syn_ack_pkt, 0, sizeof(syn_ack_pkt));
        syn_ack_pkt.header.seq_num = server_isn;
        syn_ack_pkt.header.ack_num = syn_pkt.header.seq_num + 1;
        syn_ack_pkt.header.flags = FLAG_SYN | FLAG_ACK;
        syn_ack_pkt.header.window_size = RECEIVER_BUFFER_SIZE;
        
        sendto(sockfd, &syn_ack_pkt, sizeof(struct sham_header), 0, (struct sockaddr *)&client_addr, client_len);
        log_event("SND SYN-ACK SEQ=%u ACK=%u", server_isn, syn_ack_pkt.header.ack_num);

        // Step 3: Receive final ACK
        struct sham_packet final_ack_pkt;
        recvfrom(sockfd, &final_ack_pkt, sizeof(final_ack_pkt), 0, (struct sockaddr *)&client_addr, &client_len);

        if ((final_ack_pkt.header.flags & FLAG_ACK) && final_ack_pkt.header.ack_num == server_isn + 1) {
             log_event("RCV ACK FOR SYN");
             printf("Connection established with %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

             if (chat_mode) {
                handle_chat_mode(sockfd, &client_addr, loss_rate);
             } else {
                handle_file_transfer(sockfd, &client_addr, loss_rate, final_ack_pkt.header.seq_num, final_ack_pkt.header.ack_num);
             }
             printf("Connection closed. Server shutting down.\n");
        }
    }

    close(sockfd);
    close_logging();
    return 0; 
}


void handle_file_transfer(int sockfd, struct sockaddr_in *client_addr, float loss_rate, uint32_t start_seq, uint32_t start_ack) {
    char output_filename[256] = "received_file.dat"; // Default
    
    // Receive the packet containing the filename
    struct sham_packet name_pkt;
    socklen_t client_len = sizeof(*client_addr);
    recvfrom(sockfd, &name_pkt, sizeof(name_pkt), 0, (struct sockaddr *)client_addr, &client_len);
    if(name_pkt.header.flags & FLAG_DATA) {
        strncpy(output_filename, name_pkt.data, sizeof(output_filename) - 1);
        log_event("RCV DATA SEQ=%u LEN=%zu (filename: %s)", name_pkt.header.seq_num, strlen(name_pkt.data) + 1, output_filename);
        
        // ACK the filename packet
        struct sham_packet ack_pkt;
        memset(&ack_pkt, 0, sizeof(ack_pkt));
        ack_pkt.header.flags = FLAG_ACK;
        ack_pkt.header.ack_num = name_pkt.header.seq_num + strlen(name_pkt.data) + 1;
        sendto(sockfd, &ack_pkt, sizeof(struct sham_header), 0, (struct sockaddr *)client_addr, client_len);
        log_event("SND ACK=%u WIN=%u", ack_pkt.header.ack_num, RECEIVER_BUFFER_SIZE);
    }
    
    FILE *fp = fopen(output_filename, "wb");
    if (!fp) {
        perror("fopen output_file failed");
        return;
    }

    uint32_t expected_seq_num = 20000; // Expected data sequence start
    struct sham_packet pkt;
    
    while(1) {
        ssize_t n = recvfrom(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)client_addr, &client_len);
        
        if (n <= 0) continue;

        // Simulate packet loss
        if (should_drop_packet(loss_rate)) {
            if(pkt.header.flags & FLAG_DATA)
                log_event("DROP DATA SEQ=%u", pkt.header.seq_num);
            continue;
        }

        if (pkt.header.flags & FLAG_FIN) {
            log_event("RCV FIN SEQ=%u", pkt.header.seq_num);
            // Start termination
            struct sham_packet ack_pkt, fin_pkt;

            // Send ACK for FIN
            memset(&ack_pkt, 0, sizeof(ack_pkt));
            ack_pkt.header.flags = FLAG_ACK;
            ack_pkt.header.ack_num = pkt.header.seq_num + 1;
            sendto(sockfd, &ack_pkt, sizeof(struct sham_header), 0, (struct sockaddr *)client_addr, client_len);
            log_event("SND ACK FOR FIN");
            
            // Send our FIN
            memset(&fin_pkt, 0, sizeof(fin_pkt));
            fin_pkt.header.flags = FLAG_FIN;
            fin_pkt.header.seq_num = 8500; // Arbitrary server FIN sequence
            sendto(sockfd, &fin_pkt, sizeof(struct sham_header), 0, (struct sockaddr *)client_addr, client_len);
            log_event("SND FIN SEQ=%u", fin_pkt.header.seq_num);

            // Receive final ACK
            recvfrom(sockfd, &ack_pkt, sizeof(ack_pkt), 0, (struct sockaddr *)client_addr, &client_len);
            log_event("RCV ACK=%u", ack_pkt.header.ack_num);
            break; // Exit loop
        }

        if (pkt.header.flags & FLAG_DATA) {
            log_event("RCV DATA SEQ=%u LEN=%ld", pkt.header.seq_num, n - sizeof(struct sham_header));
            
            // Only write if it's the expected packet
            if (pkt.header.seq_num == expected_seq_num) {
                size_t data_len = n - sizeof(struct sham_header);
                fwrite(pkt.data, 1, data_len, fp);
                expected_seq_num += data_len;
            }
            
            // Always send a cumulative ACK for what we have received in order
            struct sham_packet ack_pkt;
            memset(&ack_pkt, 0, sizeof(ack_pkt));
            ack_pkt.header.flags = FLAG_ACK;
            ack_pkt.header.ack_num = expected_seq_num;
            ack_pkt.header.window_size = RECEIVER_BUFFER_SIZE; // Simplified constant window
            sendto(sockfd, &ack_pkt, sizeof(struct sham_header), 0, (struct sockaddr *)client_addr, client_len);
            log_event("SND ACK=%u WIN=%u", ack_pkt.header.ack_num, ack_pkt.header.window_size);
        }
    }

    fclose(fp);
    printf("File reception complete.\n");
    calculate_md5(output_filename);
}

void handle_chat_mode(int sockfd, struct sockaddr_in *client_addr, float loss_rate) {
    printf("Chat mode activated with client. Type '/quit' to exit.\n");

    fd_set read_fds;
    socklen_t client_len = sizeof(*client_addr);
    uint32_t seq = 50000, peer_seq = 0; // Track our sequence number

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

        // Check for network input
        if (FD_ISSET(sockfd, &read_fds)) {
            struct sham_packet pkt;
            ssize_t n = recvfrom(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)client_addr, &client_len);
            
            if (n <= 0) {
                 printf("Client disconnected unexpectedly.\n");
                 break;
            }

            // Client initiated termination
            if (pkt.header.flags & FLAG_FIN) {
                log_event("RCV FIN SEQ=%u", pkt.header.seq_num);
                printf("Client initiated termination. Closing connection.\n");
                
                struct sham_packet ack_pkt;
                memset(&ack_pkt, 0, sizeof(ack_pkt));
                ack_pkt.header.flags = FLAG_ACK;
                ack_pkt.header.ack_num = pkt.header.seq_num + 1;
                sendto(sockfd, &ack_pkt, sizeof(struct sham_header), 0, (struct sockaddr *)client_addr, client_len);
                log_event("SND ACK FOR FIN");

                struct sham_packet fin_pkt;
                memset(&fin_pkt, 0, sizeof(fin_pkt));
                fin_pkt.header.flags = FLAG_FIN;
                fin_pkt.header.seq_num = seq;
                sendto(sockfd, &fin_pkt, sizeof(struct sham_header), 0, (struct sockaddr *)client_addr, client_len);
                log_event("SND FIN SEQ=%u", seq++);

                recvfrom(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)client_addr, &client_len);
                if (pkt.header.flags & FLAG_ACK) {
                    log_event("RCV ACK=%u", pkt.header.ack_num);
                }
                break; 
            }
            
            if (pkt.header.flags & FLAG_DATA) {
                // Calculate actual data length and null-terminate the buffer
                ssize_t data_len = n - sizeof(struct sham_header);
                if (data_len < MAX_DATA_SIZE) {
                    pkt.data[data_len] = '\0'; 
                }
                
                log_event("RCV DATA SEQ=%u LEN=%ld", pkt.header.seq_num, data_len);
                printf("Client: %s", pkt.data);
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
                    sendto(sockfd, &fin_pkt, sizeof(struct sham_header), 0, (struct sockaddr *)client_addr, client_len);
                    log_event("SND FIN SEQ=%u", seq++);

                    struct sham_packet rcv_pkt;
                    recvfrom(sockfd, &rcv_pkt, sizeof(rcv_pkt), 0, (struct sockaddr *)client_addr, &client_len);
                    log_event("RCV ACK FOR FIN");
                    
                    recvfrom(sockfd, &rcv_pkt, sizeof(rcv_pkt), 0, (struct sockaddr *)client_addr, &client_len);
                    log_event("RCV FIN SEQ=%u", rcv_pkt.header.seq_num);
                    peer_seq = rcv_pkt.header.seq_num;

                    struct sham_packet final_ack_pkt;
                    memset(&final_ack_pkt, 0, sizeof(final_ack_pkt));
                    final_ack_pkt.header.flags = FLAG_ACK;
                    final_ack_pkt.header.ack_num = peer_seq + 1;
                    sendto(sockfd, &final_ack_pkt, sizeof(struct sham_header), 0, (struct sockaddr *)client_addr, client_len);
                    log_event("SND ACK=%u", final_ack_pkt.header.ack_num);
                    
                    break;
                }
                
                struct sham_packet data_pkt;
                memset(&data_pkt, 0, sizeof(data_pkt));
                data_pkt.header.seq_num = seq;
                data_pkt.header.flags = FLAG_DATA;
                strncpy(data_pkt.data, buffer, MAX_DATA_SIZE);
                size_t len = strlen(buffer);
                sendto(sockfd, &data_pkt, sizeof(struct sham_header) + len, 0, (struct sockaddr *)client_addr, client_len);
                log_event("SND DATA SEQ=%u LEN=%zu", seq, len);
                seq += len;
            }
        }
    }
}

void calculate_md5(const char *filename) {
    unsigned char c[MD5_DIGEST_LENGTH];
    int i;
    FILE *inFile = fopen(filename, "rb");
    MD5_CTX mdContext;
    int bytes;
    unsigned char data[1024];

    if (inFile == NULL) {
        printf("%s can't be opened.\n", filename);
        return;
    }

    MD5_Init(&mdContext);
    while ((bytes = fread(data, 1, 1024, inFile)) != 0) {
        MD5_Update(&mdContext, data, bytes);
    }
    MD5_Final(c, &mdContext);
    
    printf("MD5: ");
    for(i = 0; i < MD5_DIGEST_LENGTH; i++) {
        printf("%02x", c[i]);
    }
    printf("\n");

    fclose(inFile);
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
    fflush(log_fp);
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
