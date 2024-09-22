#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdbool.h>

#define BUFSIZE 1024

// msg have to be less than 896

char buf[1024] = {0};


// if it is succeed return 0, otherwise return 1
int read_bytes(int fd, void * buf, size_t len){
    char * p = buf ;
    size_t acc = 0 ;

    while (acc < len) {
        size_t read_cnt ;
        read_cnt = read(fd, p, len - acc) ;
        if (read_cnt == 0)
            return 1 ;
        p += read_cnt ;
        acc += read_cnt ;
    }
    return 0 ;
}

void error_handling(char * message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

int main(int argc, char *argv[]) {
    int sock, i;
    int str_len, recv_len, recv_num;
    struct sockaddr_in serv_addr;

    fd_set reads, temps;
    int fd_max = 0;

    if (argc != 4) {
        printf("Usage : %s <IP> <port> <nickname>\n", argv[0]);
        exit(1);
    }

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if (connect(sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect() error");

    // send nickname
    sprintf(buf, "nickname: %s", argv[3]);
    write(sock, buf, sizeof(buf));

    printf("CURRENT USER: ");
    // recv nickname list and print
    for (i = 0; i < 4; i++) {
        memset(buf, 0x00, sizeof(buf));
        if (read_bytes(sock, buf, 128) == 1) {
            fprintf(stderr, "read nickname list failed\n");
            exit(EXIT_FAILURE);
        }
        printf("%s \t", buf);
    }
    // if this code doesn't exist, can't print current user list. WHY???
    printf("\n");

    FD_ZERO(&reads);
    FD_SET(0, &reads);
    FD_SET(sock, &reads);
    fd_max = sock;

    while(1) {
        int fd_num;

        temps = reads;

        if ((fd_num = select(fd_max+1, &temps, NULL, NULL, NULL)) == -1) {
            error_handling("select() error");
            exit(EXIT_FAILURE);
        }

        // printf("fd_num: %d\n", fd_num);

        if (FD_ISSET(0, &temps)) {
            memset(buf, 0x00, sizeof(buf));
            fgets(buf, BUFSIZE, stdin);

            write(sock, buf, sizeof(buf));

            if (!strcmp(buf, "q\n")) {
                sleep(3);
                break;
            }

            // printf("\n");
            FD_CLR(0, &temps);
        }

        if (FD_ISSET(sock, &temps)) {
            memset(buf, 0x00, sizeof(buf));
            if (read_bytes(sock, buf, sizeof(buf)) == 1) {
                fprintf(stderr, "read msg failed\n");
                exit(EXIT_FAILURE);
            }

            if (strncmp(buf, "ADD USER", 8) == 0 || strncmp(buf, "OUT USER", 8) == 0) {
                printf("\n<%s>\n\n", buf);
            }
            else {
                printf("%s", buf);
            }
        }
    }
    close(sock);
    return 0;
}