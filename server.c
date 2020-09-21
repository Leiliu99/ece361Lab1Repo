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
        return 0;
    }

    //associate socket with a port
    if(bind(socketfd, res->ai_addr, res->ai_addrlen) == -1){
        return 0;
    }

    addr_len = sizeof(their_addr);
    int numbytes;
    if((numbytes = recvfrom(socketfd, buf, MAXBUFLEN - 1, 0,(struct sockaddr *)&their_addr, &addr_len)) == -1){//receive a message from client
        return 0;
    }


//    char s[INET6_ADDRSTRLEN];
//    printf("listener: got packet from %s\n",
//        inet_ntop(their_addr. ss_family,
//            get_in_addr((struct sockaddr *)&their_addr),
//            s, sizeof s));
//    printf("listener: packet is %d bytes long\n", numbytes);
//    buf[numbytes] = '\0';
//    printf("listener: packet contains \"%s\"\n", buf);

    //printf("[debug]%s.\n", buf);

    char desiredMessage[] = "ftp";
    if(strcmp(buf, desiredMessage) == 0){//if the message is "ftp" reply yes
        if(sendto(socketfd, "yes", 3, 0, (struct sockaddr *)&their_addr, addr_len) == -1){
            printf("hereeee");
            return 0;
        }
    }else{//reply no otherwise
        if(sendto(socketfd, "no", 2, 0,(struct sockaddr *)&their_addr, addr_len) == -1){
            return 0;
        }
    }

    freeaddrinfo(res); //avoid memory leak
    close(socketfd);//close the socket
    return 0;
    //ug180.eecg.toronto.edu,128.100.13.180 ecdsa-sha2-nistp256 AAAAE2VjZHNhLXNoYTItbmlzdHAyNTYAAAAIbmlzdHAyNTYAAABBBJgtxmKZcFTKbPgxsUqE0bWGO7o3XQfmOz023KvotBGznGE2IXJpgxeAIW7vKWtzVfw6O5D3oxR2cAgUS6ualYA=
}
