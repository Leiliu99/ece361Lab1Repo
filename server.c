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

//transfer the message string into the packet for further analysis
void stringTostruct(struct packet* packetFile, char* messageString, int messageSize);

int main(int argc, char** argv){
    int socketfd;
    struct addrinfo hints, *res;
    //char buf[MAXBUFLEN];
    struct sockaddr_storage their_addr;
    socklen_t addr_len;
    int receivedSeq = 0;

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
    bool firstTransmit = true;
    while(continueReceive){
        char buf[MAXBUFLEN];
        if((numbytes = recvfrom(socketfd, buf, MAXBUFLEN - 1, 0,(struct sockaddr *)&their_addr, &addr_len)) == -1){//receive a message from client
            printf("Unable to receive from deliver. Quit the program\n");
            return 0;
        }
        buf[numbytes] = '\0';
        struct packet receivedpacketFile;
        printf("numbytes size, %d\n", numbytes);
        stringTostruct(&receivedpacketFile, buf, numbytes);
        if(receivedSeq == receivedpacketFile.frag_no){
            printf("Recieve the message from deliver, seq#: %d, dupliacate packet, drop this packet\n", receivedpacketFile.frag_no);
        }else{
            printf("Recieve the message from deliver, seq#: %d, and package size is: %d\n", receivedpacketFile.frag_no, receivedpacketFile.size);
            receivedSeq = receivedpacketFile.frag_no;
            if(firstTransmit){
                char filePrefix[100] = "serverCreated";
                char* fileNameBefore = receivedpacketFile.filename;
                printf("check file name, %s\n", fileNameBefore);
                strcat(filePrefix, fileNameBefore);
                printf("check, %s\n", filePrefix);
                fp = fopen(filePrefix, "wb");
                firstTransmit = false;
            }
            //printf("package content, %s\n", receivedpacketFile.filedata);
            fwrite(receivedpacketFile.filedata , 1 , receivedpacketFile.size , fp);
            if(sendto(socketfd, "ACK", 3, 0,(struct sockaddr *)&their_addr, addr_len) == -1){
                printf("Unable to send ACK number back to deliver. Quit the program\n");
                return 0;
            }
            printf("sent ACK\n");
            if(receivedpacketFile.total_frag == receivedpacketFile.frag_no){//EOF the file reached
                continueReceive = false;
            }
        }

    }
    fclose(fp);

    freeaddrinfo(res); //avoid memory leak
    close(socketfd);//close the socket
    return 0;
}

void stringTostruct(struct packet* packetFile, char* messageString, int messageSize){
    int totalFragLength;
    int fragNoLength;
    int sizeLength;
    int fileNameLength=0;
    int counter = 0;
    int currentLength = 0;
    for(int i = 0; i < messageSize; i++){
        if(messageString[i] == ':'){
            if(counter == 0){
                totalFragLength = currentLength;
                counter++;
            }else if(counter == 1){
                fragNoLength = currentLength;
                counter++;
            }else if(counter == 2){
                sizeLength = currentLength;
                counter++;
            }else if(counter == 3){
                fileNameLength = currentLength;
                counter++;
                break;
            }else{
                break;
            }
            currentLength = 0;
        }else{
            currentLength++;
        }

    }
    char totalFrag[totalFragLength + 1];
    char fragNo[fragNoLength + 1];
    char size[sizeLength + 1];
    char fileName[fileNameLength + 1];
    int pointer;
    for(pointer = 0; pointer < totalFragLength; pointer++){
        totalFrag[pointer] = messageString[pointer];
    }
    totalFrag[totalFragLength] = '\0';
    packetFile->total_frag = atoi(totalFrag);
    printf("total frag: %d\n", packetFile->total_frag);
    for(pointer = 0; pointer < fragNoLength; pointer++){
        fragNo[pointer] = messageString[pointer + totalFragLength + 1];
    }
    fragNo[fragNoLength] = '\0';
    packetFile->frag_no = atoi(fragNo);
    printf("frag no: %d\n", packetFile->frag_no);
    for(pointer = 0; pointer < sizeLength; pointer++){
        size[pointer] = messageString[pointer + totalFragLength + fragNoLength + 2];
    }
    size[sizeLength] = '\0';
    packetFile->size = atoi(size);
    printf("size: %d\n", packetFile->size);

    for(pointer = 0; pointer < fileNameLength; pointer++){
        fileName[pointer] = messageString[pointer + totalFragLength + fragNoLength + sizeLength + 3];
    }
    fileName[fileNameLength] = '\0';
    packetFile->filename = (char *)malloc(sizeof(char) * fileNameLength);
    packetFile->filename[fileNameLength] = '\0';
    for(int i = 0; i < fileNameLength; i++){
        packetFile->filename[i] = fileName[i];
    }
    printf("file name: %s\n", packetFile->filename);

    int dataStart = totalFragLength + fragNoLength + sizeLength + fileNameLength + 4;
    printf("datasize: %d\n", messageSize-dataStart+1);
    for(int i = 0 ; i < packetFile->size; i++){
        packetFile->filedata[i] = messageString[i + dataStart];
    }
    return;
}
