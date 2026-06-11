#include <SDL2/SDL_render.h>
#include <SDL2/SDL_rwops.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_video.h>
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
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

int streamJpgData(int sockfd, FILE * fptr, int numFrames){

    SDL_Init(SDL_INIT_VIDEO); //Initialise image rendering library
    IMG_Init(IMG_INIT_JPG);

    SDL_RWops * rw = NULL;
    SDL_Surface * surface = NULL;
    SDL_Texture * texture = NULL;
    SDL_Renderer * renderer = NULL;
    
    SDL_Event event;

    SDL_Window * window = NULL;


    uint32_t jpgDataSize = 0;
    int charn;

    void * jpgData = malloc(200000);
    jpgDataSize = 200000; //200KB (0.2 MB)
    uint32_t jpgByteLength;
    uint32_t * jpgByteLengthPtr = &jpgByteLength;
    uint32_t remLength;

    for(int i = 0; i < numFrames; i++){


        

        read(sockfd, &remLength, sizeof(remLength));

        remLength = ntohl(remLength);
        jpgByteLength = remLength;
        if(jpgByteLength > jpgDataSize){
            free(jpgData);
            jpgData = malloc(jpgByteLength);
            jpgDataSize = jpgByteLength;
        }
        bzero(jpgData, jpgDataSize);

        while(remLength > 0){


            charn = read(sockfd, jpgData+jpgByteLength-remLength, remLength); //Read from sock
            if(!(charn < 0)){
                remLength -= charn;
            }
            else{
                puts("Error occurred");
                printf("read failed: %s\n", strerror(errno));
                return -1;
            }
        } //Now we have the proper JPG data and correct JPG data size loaded into memory :)
        rw = SDL_RWFromMem(jpgData, jpgByteLength);
        
        
        surface = IMG_Load_RW(rw, 1);//Value of 1 autofrees the old rw structure. We have to make one each time. Or do we? //Yes it seems like we do
        if(!surface){
            puts("Image loading failed");
            return -1;
        }

        if(window == NULL){
            window = SDL_CreateWindow("Camera", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, surface->w, surface->h, SDL_WINDOW_SHOWN);

        }
        if(renderer == NULL){
            renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        }
        if(texture == NULL){
        texture = SDL_CreateTextureFromSurface(renderer, surface);
        }
        else{
            SDL_UpdateTexture(texture, NULL, surface->pixels, surface->pitch);
        }
        SDL_FreeSurface(surface);

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);




        //char filename[64];
        //snprintf(filename, sizeof(filename), "image%d.jpg", i);

        //fptr = fopen(filename, "w");

        //fwrite(jpgData, 1, jpgByteLength, fptr);

        //fclose(fptr);

    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();


    free(jpgData);

    return 1;
}

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

    streamJpgData(newsockfd, fptr, 1000);


    printf("Printing F1");
    
    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);
    return 0;
}