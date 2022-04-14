#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "protocol.h"

void dg_echo(int);

int main(int argc, char *argv[]) {
    unsigned short port = 8000;
    if (argc > 1) port = atoi(argv[1]);
    
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(-1);
    }

    struct sockaddr_in my_addr;
    bzero(&my_addr, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    printf("binding server to port %d\n", port);
    int err_log;
    err_log = bind(sockfd, (struct sockaddr*)&my_addr, sizeof(my_addr));
    if (err_log) {
        perror("bind");
        close(sockfd);
        exit(-1);
    }

    struct protocol pro;

    printf("receive data...\n");

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int num = recvfrom(sockfd, (struct protocol *)&pro, sizeof(pro), 0, (struct sockaddr *)&client_addr, (struct socklen_t *)&client_addr_len);
        if (pro.command == DOWNLOAD) {
            int a = access(pro.buf, F_OK);
            if (a == -1) printf("No such file");
            else {
                pro.command = YES;
                sendto(sockfd, (struct protocol *)&pro, sizeof(pro), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
                num = recvfrom(sockfd, (struct protocol *)&pro, sizeof(pro), 0, (struct sockaddr *)&client_addr, (struct socklen_t *)&client_addr_len);
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
                            sendto(sockfd, (struct protocol *)&pro, sizeof(pro), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
                            pro.no ++;
                            bzero(&pro.buf, BUFLEN);
                        }
                        bzero(&pro.buf, BUFLEN);
                        pro.command = END;
                        sendto(sockfd, (struct protocol *)&pro, sizeof(pro), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
                    }
                }
            }
        }
        if (pro.command == UPLOAD) {
            
            FILE *fd;
            fd = fopen(pro.buf, "w+");

            pro.command = YES;
            int no = 0;
            sendto(sockfd, (struct protocol *)&pro, sizeof(pro), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));


            if (fd == NULL) {
                perror("create file error");
                exit(1);
            }
            else {
                bzero(&pro.buf, BUFLEN);
                while (num = recvfrom(sockfd, (struct protocol *)&pro, sizeof(pro), 0, (struct sockaddr *)&client_addr, (struct socklen_t *)&client_addr_len)) {
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
            // printf("[%s]\n", pro.buf);
            FILE *fstream = NULL;
            int error = 0;
            if ((fstream = popen(pro.buf, "r")) == NULL) {
                printf("error command");
                exit(1);
            }

            pro.command = START;
            sendto(sockfd, (struct protocol *)&pro, sizeof(pro), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
            while (fgets(pro.buf, sizeof(pro.buf), fstream)) {
                pro.command = CONTENT;
                printf("%s", pro.buf);
                sendto(sockfd, (struct protocol *)&pro, sizeof(pro), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
            }
            pro.command = END;
            sendto(sockfd, (struct protocol *)&pro, sizeof(pro), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));

            pclose(fstream);
        }
    }

    close(sockfd);
}

