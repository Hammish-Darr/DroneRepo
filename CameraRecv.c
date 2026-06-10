#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
int main(int argc, char * argv[]){

    int sockfd, newsockfd, charn;
    uint clilen, portno;

    FILE * fptr;


    struct sockaddr_in servaddr, cliaddr;

    if(argc != 2){
        printf("Please provide the correct arguments\n");
        return 1;
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        printf("Failed to create socket\n");
        return 1;
    }

    bzero((char *)&servaddr.sin_zero, sizeof(servaddr.sin_zero));

    portno = atoi(argv[1]);

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(portno);
    servaddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, ((struct sockaddr *) &servaddr), sizeof(servaddr)) > 0){
        printf("SOcket binding failed, port likely already bound\n");
        return 1;

    }
    listen(sockfd, 5);
    clilen = sizeof(cliaddr);

    newsockfd = accept(sockfd, (struct sockaddr *) &cliaddr, &clilen);

    if(newsockfd < 0){
        printf("error on accepting client\n");
        return 1;
    }


    printf("Printing F1");
    
    uint32_t jpgDataSize = 0;

    for(int i = 0; i < 100; i++){


        uint32_t len;

        read(newsockfd, &len, sizeof(len));
        //printf("Length (len) is : %d", len);
        //printf("Printing F");

        len = ntohl(len);

        //printf("Length (len) is : %d", len);

        void * jpgData = malloc(len);
        jpgDataSize = len;

        bzero(jpgData, jpgDataSize);
        charn = 0;

        while(len > 0){

            charn = read(newsockfd, jpgData+jpgDataSize-len, len); //Read from sock
            if(!(charn < 0)){
                len -= charn;
            }
            else{
                puts("Error occurred");
                printf("read failed: %s\n", strerror(errno));
                return 1;
            }
        }

        char filename[64];
        snprintf(filename, sizeof(filename), "image%d.jpg", i);

        fptr = fopen(filename, "w");

        fwrite(jpgData, 1, jpgDataSize, fptr);
        

        free(jpgData);

        fclose(fptr);

    }

    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);
    return 0;
}