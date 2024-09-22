#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdbool.h>

#define BUFSIZE 1024

char buf[1024] = {0};
bool buf_flag = false, send_current_client_flag = false;
int sent_msg_fd;    // fd which makes buf_flag true

struct client_info {
    int fd;
    char nickname[128];
};

struct client_info client_list[4] = {0};

// check fd is in client_list
// if it succeed return 0, otherwise 1
int check_fd (int fd) {
    int i;
    for (i = 0; i < 4; i++) {
        if (client_list[i].fd == fd) {
            return 0;
        }
    }
    return 1;
}

// if it failed return -1
int find_nickname (int fd) {
    int i;
    for (i = 0; i < 4; i++) {
        if (client_list[i].fd == fd) {
            return i;   // i is index of client_list which fd is fd
        }
    }
    return -1;
}

int insert_fd (int fd, char * nickname) {
    int i;
    for (i = 0; i < 4; i++) {
        if (client_list[i].fd == 0) {
            client_list[i].fd = fd;
            strcpy(client_list[i].nickname, nickname);

            memset(buf, 0x00, sizeof(buf));
            sprintf(buf, "ADD USER: %s", client_list[find_nickname(fd)].nickname);
            buf_flag = true;
            send_current_client_flag = true;
            sent_msg_fd = fd;

            return 0;
        }
    }
    return 1;
}

int remove_fd (int fd) {
    int i;
    for (i = 0; i < 4; i++) {
        if (client_list[i].fd == fd) {
            memset(buf, 0x00, sizeof(buf));
            sprintf(buf, "OUT USER: %s", client_list[find_nickname(fd)].nickname);
            buf_flag = true;
            sent_msg_fd = fd;

            client_list[i].fd = 0;
            strcpy(client_list[i].nickname, "");

            return 0;
        }
    }
    return 1;
}

// send the info of current client to fd
int send_current_client(int fd) {
    int i;
    for (i = 0; i < 4; i++) {
        write(fd, client_list[i].nickname, 128);
    }
    send_current_client_flag = false;
    return 0;
}


void error_handling(char * message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

int main(int argc, char ** argv) {

    int serv_sock;
    struct sockaddr_in serv_addr;

    fd_set reads, temps;
    int fd_max = 0;
    char copy[896];
    char * name_ptr;

    if (argc != 2) {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) == -1)
        error_handling("bind() error");

    if (listen(serv_sock, 5) == -1)
        error_handling("listen() error");

    FD_ZERO(&reads);
    FD_SET(serv_sock, &reads);
    fd_max = serv_sock;

    while(1) {
        int fd, str_len, fd_num, i;
        int clnt_sock;
        socklen_t clnt_len;
        struct sockaddr_in clnt_addr;

        temps = reads;

        // send message for all client
        if (buf_flag) {
            for (i = 4; i < fd_max+1; i++) {
                if (FD_ISSET(i, &temps) && i != sent_msg_fd && check_fd(i) == 0) {
                    write(i, buf, sizeof(buf));
                }
            }
            if (send_current_client_flag) {
                send_current_client(sent_msg_fd);
            }
            buf_flag = false;
        }

        if ((fd_num = select(fd_max+1, &temps, NULL, NULL, NULL)) == -1) {
            error_handling("select() error");
            exit(EXIT_FAILURE);
        }

        for (fd = 0; fd < fd_max+1; fd++) {
            if (FD_ISSET(fd, &temps)) {
                if (fd == serv_sock) {
                    clnt_len = sizeof(clnt_addr);
                    clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_len);
                    FD_SET(clnt_sock, &reads);
                    if (fd_max < clnt_sock)
                        fd_max = clnt_sock;
                    printf("connected client: %d\n", clnt_sock);
                }
                else {
                    memset(buf, 0x00, sizeof(buf));

                    str_len = read(fd, buf, sizeof(buf));
            
                    if (str_len <= 0) {
                        FD_CLR(fd, &reads);
                        close(fd);
                        remove_fd(fd);
                        printf("closed client: %d\n", fd);
                    }
                    else {
                        if (strncmp(buf, "q\n", 2) == 0) {
                            FD_CLR(fd, &reads);
                            close(fd);
                            remove_fd(fd);
                            printf("closed client: %d\n", fd);
                        }
                        // receive nickname from user
                        else if (strncmp(buf, "nickname: ", 10) == 0) {   
                            name_ptr = strchr(buf, ' ') + 1;  
                            insert_fd(fd, name_ptr);
                            continue;
                        }
                        else {
                            sent_msg_fd = fd;
                            buf_flag = true;
                            memset(copy, 0x00, sizeof(copy));
                            strncpy(copy, buf, sizeof(copy));
                            memset(buf, 0x00, sizeof(buf));
                            sprintf(buf, "[%s] %s", client_list[find_nickname(fd)].nickname, copy);
                            printf("fd: %d  msg: %s\n", fd, buf);
                        }
                    }
                }
            }
        }
    }
}
