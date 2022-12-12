/* Sending and Receiving Functions from Client */
#include <netinet/in.h>
#include <sys/socket.h>

#define BUFSIZE 1024

#define VIDBUFSIZE 262144

int send_video(int sockfd, char *vidLoc); // Sends video specified by the filename in vidLoc to the specified socket

int receive_video(int sockfd); // Receives a video from the specified socket

void serve_videofile(struct sockaddr_in serveraddr, struct sockaddr_in clientaddr, char *vidLoc);

void error(char *msg);

struct VIDPKT {
    char type; // type=8
    uint32_t size;
    char filename[128];
}__attribute__((packed));  ; 
