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

#define MAXBUFLEN 100


int main(int argc, char** argv){
    int socketfd;
    struct addrinfo hints, *res;
    char buf[MAXBUFLEN];
    struct sockaddr_storage their_addr;
    socklen_t addr_len;


    // first, load up address structs with getaddrinfo():
    memset(&hints, 0, sizeof hints);//zero out the structure
    hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

    int rv;
    if ((rv = getaddrinfo(NULL, argv[1], &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
   // getaddrinfo(NULL, argv[1], &hints, &res);

    //create a UDP socket
    socketfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);//a socket descriptor
    if(socketfd == -1){
        printf("Unable to build the socket. Quit the program\n");
        return 0;
    }

    //associate socket with a port
    if(bind(socketfd, res->ai_addr, res->ai_addrlen) == -1){
        return 0;
    }
    printf("Waiting for deliver's message.\n");
    addr_len = sizeof(their_addr);
    int numbytes;
    if((numbytes = recvfrom(socketfd, buf, MAXBUFLEN - 1, 0,(struct sockaddr *)&their_addr, &addr_len)) == -1){//receive a message from client
        printf("Unable to receive from deliver. Quit the program\n");
        return 0;
    }

    printf("Recieve the message from deliver: %s.\n", buf);
    char desiredMessage[] = "ftp";
    if(strcmp(buf, desiredMessage) == 0){//if the message is "ftp" reply yes
        if(sendto(socketfd, "yes", 3, 0, (struct sockaddr *)&their_addr, addr_len) == -1){
            printf("Unable to send message back to deliver. Quit the program\n");
            return 0;
        }
        printf("Message contains ftp: send yes to user.\n");
    }else{//reply no otherwise
        if(sendto(socketfd, "no", 2, 0,(struct sockaddr *)&their_addr, addr_len) == -1){
            printf("Unable to send message back to deliver. Quit the program\n");
            return 0;
        }
        printf("Message does not contain ftp: send no to user.\n");
    }

    freeaddrinfo(res); //avoid memory leak
    close(socketfd);//close the socket
    return 0;

}
