#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>

#define MAXBUFLEN 100
int main(int argc, char** argv){
    int socketfd;
    char msg[] = "ftp";
    char buf[MAXBUFLEN];
    struct addrinfo hints, *servinfo;
    struct sockaddr_storage their_addr;
    socklen_t addr_len;

    // first, load up address structs with getaddrinfo():
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    getaddrinfo(argv[1], argv[2], &hints, &servinfo);

    //create a UDP socket
    socketfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);//a socket descriptor
    if(socketfd == -1){
        return 0;
    }

    //check if file exist sned message "ftp" to server
    char userInputFile[MAXBUFLEN];
    char userInputFTP[MAXBUFLEN];
    scanf("%s", userInputFTP);
    scanf("%s", userInputFile);
    struct timeval stop, start;
    if(access(userInputFile, F_OK) != -1){
        if(strcmp(userInputFTP, msg) == 0){
            gettimeofday(&start, NULL);
            if(sendto(socketfd, msg, MAXBUFLEN-1, 0, servinfo->ai_addr, servinfo->ai_addrlen) == -1){
                return 0;
            }
        }else{
            if(sendto(socketfd, userInputFTP, MAXBUFLEN-1, 0, servinfo->ai_addr, servinfo->ai_addrlen) == -1){
                return 0;
            }
        }
    }else{
        return 0;
    }

    //receive a message from the server
    if(recvfrom(socketfd, buf, MAXBUFLEN - 1, 0,(struct sockaddr *)&their_addr, &addr_len) == -1){//receive a message from server
        return 0;
    }
    //printf("[debug]%s.\n", buf);

    char desiredMessage[] = "yes";
    if(strcmp(buf, desiredMessage) == 0){//if the message is "yes" print out "A file transfer can start"
        gettimeofday(&stop, NULL);

        printf("took %lu us\n", (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec);
        printf("A file transfer can start.\n");
    }else{//exit the program
        return 0;
    }
    return 0;

}