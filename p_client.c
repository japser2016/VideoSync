/* 
 * usage for first client: p_client <port> <video file location>
 * usage for other clients: p_client <port> <des_hostname> <des_port> <need_file 0/1>
 */
 
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <errno.h>

#define BUFSIZE 1024
#define ARRAYSIZE 32
// each struct sockaddr_in has size 16 bytes -> 32*16 = 512 bytes
#define test_flag 0
#define REPEAT 2


/*
 * error - wrapper for perror
 */
void error(char *msg) {
  perror(msg);
  exit(1);
}

/* 
 * main functions 
 */
/* 
 * check the format of command line inputs
 * raise error if sth. wrong
 */
void check_inputs(int argc, char **argv);
void read_inputs(int argc, char **argv, int *portno, char *vidLoc, char *des_hostname, int *des_portno, int *need_file);
int creat_socket();
int set_reuse_address(int parentfd);
int set_timeout_address(int parentfd, int enable, int sec);
struct sockaddr_in my_server_address_build(int parentfd, int portno);
struct sockaddr_in socket_port_bind(int parentfd, int portno);
void update_maxSocket(int* maxSocket, int currentSocket);
void prepare_select(fd_set *main_set, int* maxSocket, int parentfd);
struct sockaddr_in target_server_address_build(char *des_hostname, int des_portno);
int store_one_address(struct sockaddr_in client_address, struct sockaddr_in *client_list, int *client_list_counter);
int store_address_list(struct sockaddr_in *out_client_address_list, struct sockaddr_in *client_list, int *client_list_counter);
void main_loop(fd_set *main_set, int *maxSocket, int parentfd, int *play_state, int *play_signal, int *time_stamp, struct sockaddr_in *client_list, int *client_list_counter);
int state_signal_is_diff(int play_state, int play_signal);
int check_msg_type(char *buf, int msg_len);

/* functions handle msg received */
void handle_initial_hello(struct sockaddr_in *client_list, int *client_list_counter);
void handle_userlist(struct sockaddr_in *client_list, int *client_list_counter);
void handle_pause(int *play_state, int *play_signal, int *time_stamp, struct sockaddr_in client);
void handle_play(int *play_state, int *play_signal, int *time_stamp, struct sockaddr_in client);

/* send functions */
int sends(int sockfd, char *buf, int buf_len, struct sockaddr_in *serveraddr, int repeat_time);
int construct_inital_hello(char *buf, struct sockaddr_in my_address, int need_file);
int construct_pause(char *buf, struct sockaddr_in my_address, int time_stamp);
int construct_pause_ack(char *buf, struct sockaddr_in my_address);
int construct_play(char *buf, struct sockaddr_in my_address, int time_stamp);
int construct_play_ack(char *buf, struct sockaddr_in my_address);
int construct_userlist(char *buf, struct sockaddr_in my_address, struct sockaddr_in *client_list, int client_list_counter);
int construct_userlist_ack(char *buf, struct sockaddr_in my_address);

/* receive functions */
int receives(int sockfd, char *buf, int buf_len, struct sockaddr_in *serveraddr, int repeat_time);

/*
 * helper functions
 */
void showS(char* name, char* arg);
void showD(char* name, int arg);
void showSep();
int address_is_same(struct sockaddr_in s1, struct sockaddr_in s2);
int client_is_new(struct sockaddr_in client_address, struct sockaddr_in *client_list);
 
/*
 * testing funcitons
 */
void _test_read_inputs(int portno, char *vidLoc, char *des_hostname, int des_portno, int need_file);

/*****************************************************************/
int main(int argc, char **argv){
	/* initial set */
	int portno, des_portno; /* port to listen on OR to connect to */
	int need_file = 0; /* flag to show if need file or not (0/1)*/
	char vidLoc[BUFSIZE]; /* video file location */ 
	char des_hostname[BUFSIZE]; /* hostname to connect to */ 
	int parentfd;  /* socket fd for my UDP socket */
	struct sockaddr_in serveraddr; /* server's addr */
	fd_set main_set;   /* , temp_set;  fd_set of all sockets */
	int maxSocket = 0; /* the max socket fd number */
	
	/* logic set */
	int play_state = 0; /* 0 for pause, 1 for play */
	int play_signal = 0; /* receive play/pause signal from browser */
	int time_stamp = 0; /* time stamp show watching progress */	
	struct sockaddr_in *client_list = malloc(ARRAYSIZE * sizeof(struct sockaddr_in)); /* array to store clients */
	int client_list_counter = 0; /* current client array counter */
	
	
	
	return 0;
}




/*****************************************************************/


/************************************************/
/**             main functions                  */
/************************************************/

/* 
 * check the format of command line inputs
 * raise error if sth. wrong
 */
void check_inputs(int argc, char **argv){
	
}

/*
 * store command line inputs to variables
 */
void read_inputs(int argc, char **argv, int *portno, char *vidLoc, char *des_hostname, int *des_portno, int *need_file){
	
}

/*
 * creat a new UDP socket
 */
int creat_socket(){
	return 0;
}

/*
 * set the socket to be reusable for debugging
 */
int set_reuse_address(int parentfd){
	return 0;
}

/*
 * enable(enable=1) or disable(enable=0) socket's time out
 * timeout duration is set by sec 
 */
int set_timeout_address(int parentfd, int enable, int sec){
	return 0;
}

/*
 * build server's internet address 
 * and return the address structure (sockaddr_in)
 */
//struct sockaddr_in my_server_address_build(int parentfd, int portno){}

/*
 * bind socket (parentfd) with the port (portno)
 */
//struct sockaddr_in socket_port_bind(int parentfd, int portno){}

/*
 * set maxSocket to be the larger one (compared to currentSocket)
 */
void update_maxSocket(int* maxSocket, int currentSocket){
	
}

/*
 * prepare fd_sets (main_set, temp_set) used by select()
 * and call updata_maxSocket to update maxSocket
 */
void prepare_select(fd_set *main_set, int* maxSocket, int parentfd){
	
}

/*
 * build target server's internet address (to send msg to)
 * and return the address structure (sockaddr_in)
 */
//struct sockaddr_in target_server_address_build(char *des_hostname, int des_portno){}

/*
 * check if client_address (sockaddr_in) existing in the client_list
 * by calling client_is_new()
 * if not, store it to the list
 */
int store_one_address(struct sockaddr_in client_address, struct sockaddr_in *client_list, int *client_list_counter){
	return 0;
}

/*
 * store the whole client list provided (out_client_address_list) 
 * to our local list (append)
 */
int store_address_list(struct sockaddr_in *out_client_address_list, struct sockaddr_in *client_list, int *client_list_counter){
	return 0;
}

/*
 * the main loop to handle 
 * user's commands (play/pause)
 * msg received from other peers 
 * eg. initial hello, user list, pause/play signal
 */
void main_loop(fd_set *main_set, int *maxSocket, int parentfd, int *play_state, int *play_signal, int *time_stamp, struct sockaddr_in *client_list, int *client_list_counter){
	
}

/*
 * check if play_state and play_signal are different
 */
int state_signal_is_diff(int play_state, int play_signal){
	return 0;
}

/*
 * return the msg type received
 */
int check_msg_type(char *buf, int msg_len){
	return 0;
}

/* functions handle msg received */

/*
 * send HELLO ACK
 * send client list
 * store new client
 */
void handle_initial_hello(struct sockaddr_in *client_list, int *client_list_counter){
	
}

/*
 * send USER LIST ACK
 * store user list if local list is new (with only one peer)
 */
void handle_userlist(struct sockaddr_in *client_list, int *client_list_counter){
	
}

/*
 * send PAUSE ACK
 * set the time stamp (according to the message)
 * change play state and signal if diff
 */
void handle_pause(int *play_state, int *play_signal, int *time_stamp, struct sockaddr_in client){
	
}

/*
 * send PLAY ACK
 * set the time stamp (according to the message)
 * change play state and signal if diff
 */
void handle_play(int *play_state, int *play_signal, int *time_stamp, struct sockaddr_in client){
	
}

/* send functions */
int sends(int sockfd, char *buf, int buf_len, struct sockaddr_in *serveraddr, int repeat_time){
	return 0;
}
int construct_inital_hello(char *buf, struct sockaddr_in my_address, int need_file){
	return 0;
}
int construct_pause(char *buf, struct sockaddr_in my_address, int time_stamp){
	return 0;
}
int construct_pause_ack(char *buf, struct sockaddr_in my_address){
	return 0;
}
int construct_play(char *buf, struct sockaddr_in my_address, int time_stamp){
	return 0;
}
int construct_play_ack(char *buf, struct sockaddr_in my_address){
	return 0;
}
int construct_userlist(char *buf, struct sockaddr_in my_address, struct sockaddr_in *client_list, int client_list_counter){
	return 0;
}
int construct_userlist_ack(char *buf, struct sockaddr_in my_address){
	return 0;
}

/* receive functions */
int receives(int sockfd, char *buf, int buf_len, struct sockaddr_in *serveraddr, int repeat_time){
	return 0;
}

/************************************************/
/**             helper functions                */
/************************************************/
void showS(char* name, char* arg){}
void showD(char* name, int arg){}
void showSep(){}
int address_is_same(struct sockaddr_in s1, struct sockaddr_in s2){
	return 0;
}
int client_is_new(struct sockaddr_in client_address, struct sockaddr_in *client_list){
	return 0;
}
 
/************************************************/
/**             testing functions               */
/************************************************/
void _test_read_inputs(int portno, char *vidLoc, char *des_hostname, int des_portno, int need_file){}
