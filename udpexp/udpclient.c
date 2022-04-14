#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "protocol.h"

int main(int argc, char *argv[]) {
    unsigned short port = 8000;
    char *server_ip = "192.168.43.216";

    if (argc > 1) server_ip = argv[1];
    if (argc > 2) port = atoi(argv[2]);

    int sockfd;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(-1);
    }

    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);
    socklen_t server_addr_len = sizeof(server_addr);

    int recv_len, num;

    printf("server is %s:%d\n", server_ip, port);

    printf("1:DOWNLOAD\n");
    printf("2:UPLOAD\n");
    printf("3:GETDIR\n");
    
    struct protocol pro;

    while (1) {
        int i;
        printf("option:");
        scanf("%d", &i);
        //DOWNLOAD
        if (i == 1) {
            printf("filename:");
            scanf("%s", pro.buf);
            printf("%s", pro.buf);
            pro.command = DOWNLOAD;
            sendto(sockfd, (struct protocol *)&pro, sizeof(struct protocol), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
            recv_len = recvfrom(sockfd, (struct protocol *)&pro, sizeof(pro), 0, (struct sockaddr *)&server_addr, (struct socklen_t *)&server_addr_len);
            if (pro.command == YES) {
                pro.command = START;
                sendto(sockfd, (struct protocol *)&pro, sizeof(pro), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
                FILE *fd;
                fd = fopen(pro.buf, "w+");
                int no = 0;
                if (fd == NULL) {
                    perror("create file error");
                    exit(1);
                }
                else {
                    bzero(&pro.buf, BUFLEN);
                    while (num = recvfrom(sockfd, (struct protocol *)&pro, sizeof(pro), 0, (struct sockaddr *)&server_addr, (struct socklen_t *)server_addr_len)) {
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
            sendto(sockfd, (struct protocol *)&pro, sizeof(pro), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
            num = recvfrom(sockfd, (struct protocol *)&pro, sizeof(pro), 0, (struct sockaddr *)&server_addr, (struct socklen_t *)&server_addr_len);
            
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
                        sendto(sockfd, (struct protocol *)&pro, sizeof(pro), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
                        pro.no ++;
                        bzero(&pro.buf, BUFLEN);
                    }
                    bzero(&pro.buf, BUFLEN);
                    pro.command = END;
                    sendto(sockfd, (struct protocol *)&pro, sizeof(pro), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
                }
                fclose(fd);
            }
            
        }
        //GETDIR
        if (i == 3) {
            bzero(&pro.buf, BUFLEN);
            pro.command = GETDIR;
            scanf("%s", pro.buf);
            sendto(sockfd, (struct protocol *)&pro, sizeof(pro), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
            num = recvfrom(sockfd, (struct protocol *)&pro, sizeof(pro), 0, (struct sockaddr *)&server_addr, (struct socklen_t *)&server_addr_len);
            if (pro.command == START) {
                while(1) {
                    num = recvfrom(sockfd, (struct protocol *)&pro, sizeof(pro), 0, (struct sockaddr *)&server_addr, (struct socklen_t *)&server_addr_len);
                    if (pro.command == CONTENT) printf("%s", pro.buf);
                    if (pro.command == END) break;
                }
            }
            bzero(&pro.buf, BUFLEN);
        }
    }

    close(sockfd);
    return 0;

}