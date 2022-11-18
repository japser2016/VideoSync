/* Sending and Receiving Functions from Client */

#define BUFSIZE 1024

int send_video(int sockfd, char *vidLoc); // Sends video specified by the filename in vidLoc to the specified socket
