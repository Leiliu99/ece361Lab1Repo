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
#include <sys/resource.h>

#define MAXBUFLEN 100
int main(int argc, char** argv){
    int socketfd;
    char msg[] = "ftp";
    char buf[MAXBUFLEN];
    struct addrinfo hints, *servinfo;
    struct sockaddr_storage their_addr;
    socklen_t addr_len;

    // first, load up address structs with getaddrinfo():
    memset(&hints, 0, sizeof hints);//zero out the structure
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    getaddrinfo(argv[1], argv[2], &hints, &servinfo);

    //create a UDP socket
    socketfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);//a socket descriptor
    if(socketfd == -1){
	printf("Unable to build the socket with given address and port number. Quit the program\n");
        return 0;
    }


    printf("Wait for user input:");
    //check if file exist sned message "ftp" to server
    char userInputFile[MAXBUFLEN];
    char userInputFTP[MAXBUFLEN];
    scanf("%s", userInputFTP);
    scanf("%s", userInputFile);
    //struct timeval stop, start;
    struct rusage usage;
    struct timeval start, end;
    double timeStart, timeEnd, totalTime;

    if(access(userInputFile, F_OK) != -1){
        if(strcmp(userInputFTP, msg) == 0){
            //gettimeofday(&start, NULL);
            getrusage(RUSAGE_SELF, &usage);
            start = usage.ru_utime;
            timeStart = start.tv_usec;
            if(sendto(socketfd, msg, MAXBUFLEN-1, 0, servinfo->ai_addr, servinfo->ai_addrlen) == -1){
                printf("Unable to send message to server. Quit the program\n");
                return 0;
            }
        }else{
            if(sendto(socketfd, userInputFTP, MAXBUFLEN-1, 0, servinfo->ai_addr, servinfo->ai_addrlen) == -1){
                printf("Unable to send message to server. Quit the program\n");
                return 0;
            }
        }
    }else{
        printf("the file name input does not exist\n");
        return 0;
    }
    printf("Waiting for server's response...\n");
    //receive a message from the server
    if(recvfrom(socketfd, buf, MAXBUFLEN - 1, 0,(struct sockaddr *)&their_addr, &addr_len) == -1){//receive a message from server
        printf("Unable to recieve from server. Quit the program\n");
        return 0;
    }

    char desiredMessage[] = "yes";
    if(strcmp(buf, desiredMessage) == 0){//if the message is "yes" print out "A file transfer can start"
        printf("Recieved yes from server\n");
        getrusage(RUSAGE_SELF, &usage);
        end = usage.ru_utime;
        timeEnd = end.tv_usec;

        printf("%d microseconds\n", (int)(timeEnd-timeStart));
        printf("A file transfer can start.\n");
    }else{//exit the program
        printf("Recieved no from server\n");
        return 0;
    }
    return 0;

}
