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
    char *server_ip = "10.24.4.204";
    if (argc > 1) {
        server_ip = argv[1];
    }
    if (argc > 2) {
        port = atoi(argv[2]);
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);

    int err_log = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (err_log) {
        perror("connect");
        close(sockfd);
        exit(1);
    }

    struct protocol pro;
    int num;
    while (1) {
        int i;
        printf("option:");
        scanf("%d", &i);
        getchar();
        //DOWNLOAD
        if (i == 1) {
            printf("filename:");
            scanf("%s", pro.buf);
            printf("%s", pro.buf);
            pro.command = DOWNLOAD;
            send(sockfd, (struct protocol *)&pro, sizeof(pro), 0);
            num = recv(sockfd, (struct protocol *)&pro, sizeof(pro), 0);
            if (pro.command == YES) {
                pro.command = START;
                send(sockfd, (struct protocol *)&pro, sizeof(pro), 0);
                FILE *fd;
                fd = fopen(pro.buf, "w+");
                int no = 0;
                if (fd == NULL) {
                    perror("create file error");
                    exit(1);
                }
                else {
                    bzero(&pro.buf, BUFLEN);
                    while (num = recv(sockfd, (struct protocol *)&pro, sizeof(pro), 0)) {
                        if (pro.command == CONTENT && pro.no == no) {
                            fwrite(pro.buf, 1, pro.len, fd);
                        }
                        else if (pro.command == END) {
                            printf("\ndownload over\n");
                            break;
                        }
                        no ++;
                    }
                }
                fclose(fd);
            }
        }
        //UPLOAD
        if (i == 2) {
            bzero(&pro.buf, BUFLEN);
            printf("filename:");
            scanf("%s", pro.buf);
            pro.command = UPLOAD;
            send(sockfd, (struct protocol *)&pro, sizeof(pro), 0);
            num = recv(sockfd, (struct protocol *)&pro, sizeof(pro), 0);
            
            if (pro.command == YES) {
                FILE *fd;
                pro.no = 0;
                fd = fopen(pro.buf, "r+");
                if (fd == NULL) {
                    perror("open failed");
                    exit(1);
                }
                else {
                    int f;
                    bzero(&pro.buf, BUFLEN);
                    while (f = fread(pro.buf, 1, BUFLEN, fd)) {
                        pro.command = CONTENT;
                        pro.len = f;
                        send(sockfd, (struct protocol *)&pro, sizeof(pro), 0);
                        pro.no ++;
                        bzero(&pro.buf, BUFLEN);
                    }
                    bzero(&pro.buf, BUFLEN);
                    pro.command = END;
                    send(sockfd, (struct protocol *)&pro, sizeof(pro), 0);
                }
                fclose(fd);
            }
            
        }
        //GETDIR
        if (i == 3) {
            bzero(&pro.buf, BUFLEN);
            pro.command = GETDIR;
            fgets(pro.buf, sizeof(pro.buf), stdin);
            send(sockfd, (struct protocol *)&pro, sizeof(pro), 0);
            num = recv(sockfd, (struct protocol *)&pro, sizeof(pro), 0);
            if (pro.command == START) {
                while(1) {
                    num = recv(sockfd, (struct protocol *)&pro, sizeof(pro), 0);
                    if (pro.command == CONTENT) printf("%s", pro.buf);
                    if (pro.command == END) break;
                }
            }
            bzero(&pro.buf, BUFLEN);
        }


    }
    close(sockfd);
}