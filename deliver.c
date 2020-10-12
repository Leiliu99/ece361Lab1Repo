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
#include <sys/time.h>
#include <sys/resource.h>

#define MAXBUFLEN 1500
#define FRAG_SIZE 1000
struct packet{
    unsigned int total_frag; //total number of fragments
    unsigned int frag_no; //sequence number of the fragment
    unsigned int size; //size of the data
    char* filename; //file name
    char filedata[1000]; //actual data within the file
};

//read the required file, put content in an array for further analysis
void read_file(unsigned char **dataArray, int *fileSize, char *fileName);
//add required information to the packet
void add_info_to_packet(struct packet* filePacket, int total_frag, int frag_no, int size, char* filename);
//transfer the struct packet to string format
size_t structTostring(struct packet packetFile, char* messageString);

int main(int argc, char** argv){
    int socketfd;
    char msg[] = "ftp";
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

    //set timeout
    struct timeval tv;
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    if (setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO,(const char*)&tv ,sizeof(tv)) < 0) {
        printf("Unable to set time out for the socket\n");
        perror("Error");
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
        if(strcmp(userInputFTP, msg) == 0){//input start with ftp
            int* fileSize = (int *)malloc(sizeof(int));
            unsigned char *dataArray = (unsigned char *)malloc(sizeof(unsigned char));
            read_file(&dataArray, fileSize, userInputFile);
            printf("Total filesize: %d\n", *fileSize);
            //calculating the total package needed

            int totalPackage = *fileSize / FRAG_SIZE;
            int remainingSize = *fileSize - totalPackage * FRAG_SIZE;
            if(remainingSize != 0){
                totalPackage ++;
            }
            printf("Total packages: %d\n", totalPackage);
            int sequenceNumber = 1;
            for(int i = 0; i < totalPackage; i++, sequenceNumber++){
                int packetSize;
                if((i == totalPackage - 1)  && (remainingSize != 0)){//the only or the last package
                    packetSize = remainingSize;
                }else{
                    packetSize = FRAG_SIZE;
                }
                int dataPos = i * FRAG_SIZE;

                struct packet filePacket;//create a new package for this round
                for(int k = 0; k < packetSize; k++){
                    filePacket.filedata[k] = dataArray[dataPos + k];
                }
                add_info_to_packet(&filePacket, totalPackage, sequenceNumber, packetSize, userInputFile);
                char messageString[1500] = {};
                int messageSize = structTostring(filePacket, messageString);
                //send the message to deliver
                bool recievedAck = false;
                //printf("sending message: %s, and message size: %d\n", messageString, messageSize);
                printf("sending package: %d, package size: %d\n", sequenceNumber, messageSize);
                while(!recievedAck){
                    //calculating the RTT time
                    getrusage(RUSAGE_SELF, &usage);
                    start = usage.ru_utime;
                    timeStart = start.tv_usec;
                    if(sendto(socketfd, messageString, messageSize, 0, servinfo->ai_addr, servinfo->ai_addrlen) == -1){
                        printf("Unable to send packet to server. Quit the program\n");
                        return 0;
                    }
                    printf("Waiting for server's acknowledge...\n");

                    //receive a message from the server
                    char buf[MAXBUFLEN];
                    int numbytes;
                    if((numbytes = recvfrom(socketfd, buf, MAXBUFLEN - 1, 0,(struct sockaddr *)&their_addr, &addr_len)) < 0){//receive a message from server
                        recievedAck = false;
                        printf("Resending message: %s\n", messageString);
                        continue;
                    }
                    buf[numbytes] = '\0';
                    printf("buf, %s\n", buf);
                    if(strcmp(buf, "ACK") == 0){//packet sent is good
                        printf("Recieved ack from server\n");
                        getrusage(RUSAGE_SELF, &usage);
                        end = usage.ru_utime;
                        timeEnd = end.tv_usec;
                        printf("One round is %d microseconds\n", (int)(timeEnd-timeStart));
                        recievedAck = true;
                        break;
                    }
                }
            }
        }else{
              printf("Input must start with FTP. Quit the program\n");
              return 0;
        }
    }else{
        printf("the file name input does not exist\n");
        return 0;
    }

    return 0;
}

void read_file(unsigned char **dataArray, int *fileSize, char *fileName){
    // https://www.daniweb.com/programming/software-development/threads/188487/reading-binary-file-without-knowing-file-format
    // start read file
    FILE *fp;
    fp = fopen(fileName,"rb");

    //Get file length
    fseek(fp, 0, SEEK_END);
    *fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // Allocate memory
    *dataArray = (unsigned char *) malloc((*fileSize + 1) * sizeof(unsigned char));
    if (! *dataArray) {
        fprintf(stderr, "Memory error!");
        fclose(fp);
        exit(1);
    }

    // Read file contents into buffer
    fread(*dataArray, *fileSize, 1, fp);
    fclose(fp);
}

void add_info_to_packet(struct packet* filePacket, int total_frag, int frag_no, int size, char* filename){
    filePacket->total_frag = total_frag;
    filePacket->frag_no = frag_no;
    filePacket->size = size;
    filePacket->filename = filename;
}

size_t structTostring(struct packet packetFile, char* messageString){
    char totalPackageString[10];
    char fragNoString[10];
    char sizeString[10];
    char separateColon = ':';
    sprintf(totalPackageString, "%d", packetFile.total_frag);
    strcat(messageString, totalPackageString);
    strncat(messageString, &separateColon, 1);
    sprintf(fragNoString, "%d", packetFile.frag_no);
    strcat(messageString, fragNoString);
    strncat(messageString, &separateColon, 1);
    sprintf(sizeString, "%d", packetFile.size);
    strcat(messageString, sizeString);
    strncat(messageString, &separateColon, 1);

    strcat(messageString, packetFile.filename);
    strncat(messageString, &separateColon, 1);

    int partialSize = strlen(messageString);

    memmove(messageString+strlen(messageString), packetFile.filedata, packetFile.size);
    if(packetFile.frag_no == 816){
        printf("converting result, %s\n", messageString);
    }
     //printf("converting result, %s\n", messageString);
    return (partialSize + packetFile.size);
}


