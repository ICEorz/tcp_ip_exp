#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "protocol.h"

int main(int argc, char **argv) {
    unsigned short port = 8000;
    if (argc > 1) {
        port = atoi(argv[1]);
    }
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in server_iddr;
    bzero(&server_iddr, sizeof(server_iddr));
    server_iddr.sin_family = AF_INET;
    server_iddr.sin_port = htons(port);
    server_iddr.sin_addr.s_addr = htonl(INADDR_ANY);

    int err_log = bind(sockfd, (struct sockaddr *)&server_iddr, sizeof(server_iddr));
    if (err_log) {
        perror("bingding");
        close(sockfd);
        exit(1);
    }
    err_log = listen(sockfd, 10);
    if (err_log) {
        perror("listen");
        close(sockfd);
        exit(1);
    }

    struct protocol pro;

    while (1) {
        struct sockaddr_in client_addr;
        char cli_ip[INET_ADDRSTRLEN] = "";
        socklen_t client_addr_len = sizeof(client_addr);
        int connfd;

        connfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (connfd < 0) {
            perror("accept");
            continue;
        }
        inet_ntop(AF_INET, &client_addr.sin_addr, cli_ip, INET_ADDRSTRLEN);

        char recv_buf[512] = "";
        while (1) {
            struct sockaddr_in client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            int num = recv(connfd, (struct protocol *)&pro, sizeof(pro), 0);
            if (pro.command == DOWNLOAD) {
                int a = access(pro.buf, F_OK);
                if (a == -1) printf("No such file");
                else {
                    pro.command = YES;
                    send(connfd, (struct protocol *)&pro, sizeof(pro), 0);
                    num = recv(connfd, (struct protocol *)&pro, sizeof(pro), 0);
                    if (pro.command == START) {
                        FILE *fd;
                        pro.no = 0;
                        fd = fopen(pro.buf, "r+");
                        if (fd == NULL) {
                            perror("open file failed");
                            exit(1);
                        }
                        else {
                            int f;
                            while (f = fread(pro.buf, 1, BUFLEN, fd)) {
                                pro.command = CONTENT;
                                pro.len = f;
                                send(connfd, (struct protocol *)&pro, sizeof(pro), 0);
                                pro.no ++;
                                bzero(&pro.buf, BUFLEN);
                            }
                            bzero(&pro.buf, BUFLEN);
                            pro.command = END;
                            send(connfd, (struct protocol *)&pro, sizeof(pro), 0);
                        }
                    }
                }
            }
            if (pro.command == UPLOAD) {
                
                FILE *fd;
                fd = fopen(pro.buf, "w+");

                pro.command = YES;
                int no = 0;
                send(connfd, (struct protocol *)&pro, sizeof(pro), 0);


                if (fd == NULL) {
                    perror("create file error");
                    exit(1);
                }
                else {
                    bzero(&pro.buf, BUFLEN);
                    while (num = recv(connfd, (struct protocol *)&pro, sizeof(pro), 0)) {
                        if (pro.command == CONTENT && pro.no == no) {
                            fwrite(pro.buf, 1, pro.len, fd);
                        }
                        else if (pro.command == END) {
                            printf("update succeeded\n");
                            break;
                        }
                        no ++;
                    }
                }
                fclose(fd);
            }
            if (pro.command == GETDIR) {
                FILE *fstream = NULL;
                int error = 0;
                if ((fstream = popen(pro.buf, "r")) == NULL) {
                    printf("error command");
                    exit(1);
                }

                pro.command = START;
                send(connfd, (struct protocol *)&pro, sizeof(pro), 0);
                while (fgets(pro.buf, sizeof(pro.buf), fstream)) {
                    pro.command = CONTENT;
                    printf("%s", pro.buf);
                    send(connfd, (struct protocol *)&pro, sizeof(pro), 0);
                }
                pro.command = END;
                send(connfd, (struct protocol *)&pro, sizeof(pro), 0);

                pclose(fstream);
            }
        }
        close(connfd);
    }
    close(sockfd);
}