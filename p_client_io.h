/* Sending and Receiving Functions from Client */

#define BUFSIZE 1024

int send_video(int sockfd, char *vidLoc); // Sends video specified by the filename in vidLoc to the specified socket

int receive_video(int sockfd); // Receives a video from the specified socket


void error(char *msg);

struct VIDPKT {
    char type; // type=8
    char filename[128];
}__attribute__((packed));  ; 
