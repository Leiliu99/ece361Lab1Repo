#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define MAXBUFLEN 1500



struct packet{
    unsigned int total_frag; //total number of fragments
    unsigned int frag_no; //sequence number of the fragment
    unsigned int size; //size of the data
    char* filename; //file name
    char filedata[1000]; //actual data within the file
};

void stringTostruct(struct packet* packetFile, char* messageString){
     char *token = strtok(messageString, ":");
     int counter = 0;
    while (token != NULL)
    {
        if(counter == 0){
            packetFile->total_frag = atoi(token);
        }else if(counter == 1){
            packetFile->frag_no = atoi(token);
        }else if(counter == 2){
            packetFile->size = atoi(token);
        }else if(counter == 3){
            packetFile->filename = token;
        }else{
            memmove(packetFile->filedata, token, packetFile->size);
        }
        token = strtok(NULL, ":");
        counter++;
    }
    return;
}

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
    bool continueReceive = true;
    FILE* fp;
    while(continueReceive){
        if((numbytes = recvfrom(socketfd, buf, MAXBUFLEN - 1, 0,(struct sockaddr *)&their_addr, &addr_len)) == -1){//receive a message from client
            printf("Unable to receive from deliver. Quit the program\n");
            return 0;
        }
        buf[numbytes] = '\0';
        struct packet receivedpacketFile;
        stringTostruct(&receivedpacketFile, buf);
        printf("Recieve the message from deliver, seq#: %d\n", receivedpacketFile.frag_no);
        char filePrefix[100] = "serverCreated";
        char* fileNameBefore = receivedpacketFile.filename;
        //printf("check file name, %s\n", fileNameBefore);
        strcat(filePrefix, fileNameBefore);
        //printf("check, %s\n", filePrefix);
        fp = fopen(filePrefix, "w");
        //printf("check data: \n");
        for(int i = 0; i < receivedpacketFile.size; i++){
            fputc(receivedpacketFile.filedata[i], fp);
            //printf("%c", receivedpacketFile.filedata[i]);
        }
        //printf("\n");
        if(sendto(socketfd, "ACK", 3, 0,(struct sockaddr *)&their_addr, addr_len) == -1){
            printf("Unable to send ACK number back to deliver. Quit the program\n");
            return 0;
        }
        printf("sent ACK\n");
        if(receivedpacketFile.total_frag == receivedpacketFile.frag_no){//EOF the file reached
            continueReceive = false;
        }

    }
    fclose(fp);
    freeaddrinfo(res); //avoid memory leak
    close(socketfd);//close the socket
    return 0;

}
