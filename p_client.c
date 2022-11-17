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
#define test_flag 1
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
void check_inputs(int argc, char **argv);
void read_inputs(int argc, char **argv, int *portno, char *vidLoc, char *des_hostname, int *des_portno, int *need_file);
int creat_socket();
int set_reuse_address(int parentfd);
int set_timeout_address(int parentfd, int enable, int sec);
void my_server_address_build(int portno, struct sockaddr_in *serveraddr);
void socket_port_bind(int parentfd, struct sockaddr_in *serveraddr);
void update_maxSocket(int* maxSocket, int currentSocket);
void prepare_select(fd_set *main_set, int* maxSocket, int parentfd);
struct sockaddr_in target_server_address_build(char *des_hostname, int des_portno, struct sockaddr_in *targetaddr);
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
int client_is_new(struct sockaddr_in client_address, struct sockaddr_in *client_list, int client_list_counter);
 
/*
 * testing funcitons
 */
void _test_read_inputs(int portno, char *vidLoc, char *des_hostname, int des_portno, int need_file);
void _test_parentfd_serverAddress_no_bind(int parentfd, struct sockaddr_in serveraddr);
void _test_prepare_select(fd_set main_set, int maxSocket, int parentfd);
void _test_target_server_address_build(struct sockaddr_in targetaddr);
void _test_address_is_same(struct sockaddr_in s1, struct sockaddr_in s2);
void _test_client_is_new(struct sockaddr_in s1, struct sockaddr_in s2);


/*****************************************************************/
int main(int argc, char **argv){
	/* initial set */
	int portno, des_portno; /* port to listen on OR to connect to */
	int need_file = 0; /* flag to show if need file or not (0/1)*/
	char vidLoc[BUFSIZE]; /* video file location */ 
	char des_hostname[BUFSIZE]; /* hostname to connect to */ 
	int parentfd;  /* socket fd for my UDP socket */
	struct sockaddr_in serveraddr; /* server's addr (myself) */
	struct sockaddr_in targetaddr; /* target server's addr (parent) */
	fd_set main_set;   /* , temp_set;  fd_set of all sockets */
	int maxSocket = 0; /* the max socket fd number */
	
	/* logic set */
	int play_state = 0; /* 0 for pause, 1 for play */
	int play_signal = 0; /* receive play/pause signal from browser */
	int time_stamp = 0; /* time stamp show watching progress */	
	struct sockaddr_in *client_list = malloc(ARRAYSIZE * sizeof(struct sockaddr_in)); /* array to store clients */
	bzero(client_list, ARRAYSIZE * sizeof(struct sockaddr_in));
	int client_list_counter = 0; /* current client array counter */
	
	/* load inputs */
	check_inputs(argc, argv);
	read_inputs(argc, argv, &portno, vidLoc, des_hostname, &des_portno, &need_file); 
	_test_read_inputs(portno, vidLoc, des_hostname, des_portno, need_file);	
	
	/* set socket & server address & bind port*/
	parentfd = creat_socket();	
	set_reuse_address(parentfd);
	my_server_address_build(portno, &serveraddr);
	socket_port_bind(parentfd, &serveraddr);
	_test_parentfd_serverAddress_no_bind(parentfd, serveraddr);
	
	/* prepare for select() */
	prepare_select(&main_set, &maxSocket, parentfd);
	_test_prepare_select(main_set, maxSocket, parentfd);
	
	/* if I am a new client (not the first main client) */
	if (argc == 5){
		/* setup target server + store target server */
		target_server_address_build(des_hostname, des_portno, &targetaddr);
		_test_target_server_address_build(targetaddr);
		_test_address_is_same(serveraddr, targetaddr);
		_test_client_is_new(serveraddr, targetaddr);
		
	}
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
	if (argc != 3 && argc != 5) {
		fprintf(stderr, "usage: %s <port> <video file location> OR %s <port> <des_hostname> <des_port> <need_file 0/1>\n", argv[0], argv[0]);
		exit(1);
	}	
	if (strlen(argv[2]) >= BUFSIZE){
		fprintf(stderr, "<video file location>/<des_hostname>: too long\n");
		exit(1);
	}
	if (argc == 5){
		char c1 = '0';
		char c2 = '1';
		if ((*(argv[4]) != c1 && *(argv[4]) != c2) || strlen(argv[4]) != 1){
			fprintf(stderr, "<need_file 0/1>: should be either 0 or 1\n");
			exit(1);
		}
	}
}

/*
 * store command line inputs to variables
 */
void read_inputs(int argc, char **argv, int *portno, char *vidLoc, char *des_hostname, int *des_portno, int *need_file){
	*portno = atoi(argv[1]);
	if (argc == 3){
		strcpy(vidLoc, argv[2]);
	} else {
		strcpy(des_hostname, argv[2]);
		*des_portno = atoi(argv[3]);	
		*need_file = atoi(argv[4]);	
	}
}

/*
 * creat a new UDP socket
 */
int creat_socket(){
	int new_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (new_socket < 0) 
		error("ERROR opening socket");
	return new_socket;
}

/*
 * set the socket to be reusable for debugging
 */
int set_reuse_address(int parentfd){
	int optval = 1;
	int result = setsockopt(parentfd, SOL_SOCKET, SO_REUSEADDR, 
				(const void *)&optval , sizeof(int));
	if (result != 0){
		printf("setsockopt for address reuse NOT success\n");
	}
	return result;
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
 * and store in the address structure (sockaddr_in) passed in
 */
void my_server_address_build(int portno, struct sockaddr_in *serveraddr){
	bzero((char *) serveraddr, sizeof(*serveraddr));
	serveraddr->sin_family = AF_INET;
	serveraddr->sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr->sin_port = htons((unsigned short)portno);
}

/*
 * bind socket (parentfd) with the port (portno)
 */
void socket_port_bind(int parentfd, struct sockaddr_in *serveraddr){
	if (bind(parentfd, (struct sockaddr *) serveraddr, sizeof(*serveraddr)) < 0) 
		error("ERROR on binding");	
}

/*
 * set maxSocket to be the larger one (compared to currentSocket)
 */
void update_maxSocket(int* maxSocket, int currentSocket){
	if (*maxSocket < currentSocket){
		*maxSocket = currentSocket;
	}
}

/*
 * prepare fd_sets (main_set) used by select():
 * set main_set to empty, and put parentfd into main_set
 * and call updata_maxSocket to update maxSocket
 */
void prepare_select(fd_set *main_set, int* maxSocket, int parentfd){
	FD_ZERO(main_set);
	//FD_ZERO(temp_set);
	update_maxSocket(maxSocket, parentfd);
	FD_SET(parentfd, main_set); 
}

/*
 * build target server's internet address (to send msg to)
 * and return the address structure (sockaddr_in)
 */
struct sockaddr_in target_server_address_build(char *des_hostname, int des_portno, struct sockaddr_in *targetaddr){
	struct hostent *server = gethostbyname(des_hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", des_hostname);
        exit(0);
    }
	
	bzero((char *) targetaddr, sizeof(*targetaddr));
	targetaddr->sin_family = AF_INET;
	bcopy((char *)server->h_addr, 
		  (char *)&targetaddr->sin_addr.s_addr, server->h_length);
	targetaddr->sin_port = htons(des_portno);
	return *targetaddr;
}

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
void showS(char* name, char* arg){
	printf("%s->%s\n", name, arg);
}
void showD(char* name, int arg){
	printf("%s->%d\n", name, arg);
}
void showSep(){
	printf("---------------------------------\n");
}
int address_is_same(struct sockaddr_in s1, struct sockaddr_in s2){
	int result_sin_addr = memcmp(&(s1.sin_addr), &(s2.sin_addr), sizeof(s1.sin_addr));
	int result_sin_port = memcmp(&(s1.sin_port), &(s2.sin_port), sizeof(s1.sin_port));
	if (result_sin_addr == 0 && result_sin_port == 0){
		return 1;
	}
	return 0;
}
int client_is_new(struct sockaddr_in client_address, struct sockaddr_in *client_list, int client_list_counter){
	for (int i = 0; i < client_list_counter; i++){
		if (address_is_same(client_address, client_list[i]) == 1){
			return 0;
		}
	}
	return 1;
}
 
/************************************************/
/**             testing functions               */
/************************************************/
void _test_read_inputs(int portno, char *vidLoc, char *des_hostname, int des_portno, int need_file){
	if (!test_flag)
		return;
	showSep();
	printf("_test_read_inputs:\n");	
	showD("portno",portno);
	showS("vidLoc",vidLoc);
	showS("des_hostname",des_hostname);
	showD("des_portno",des_portno);
	showD("need_file",need_file);
}
void _test_parentfd_serverAddress_no_bind(int parentfd, struct sockaddr_in serveraddr){
	if (!test_flag)
		return;
	showSep();
	printf("_test_parentfd_serverAddress_no_bind:\n");
	showD("parentfd",parentfd);
	showD("serveraddr.sin_port", ntohs(serveraddr.sin_port));
}
void _test_prepare_select(fd_set main_set, int maxSocket, int parentfd){
	if (!test_flag)
		return;
	showSep();
	printf("_test_prepare_select:\n");
	showD("parentfd",parentfd);
	showD("maxSocket",maxSocket);
	showD("FD_ISSET(parentfd, &main_set)",FD_ISSET(parentfd, &main_set));

}
void _test_target_server_address_build(struct sockaddr_in targetaddr){
	if (!test_flag)
		return;
	showSep();
	printf("_test_target_server_address_build:\n");
	showD("targetaddr.sin_port", ntohs(targetaddr.sin_port));
}
void _test_address_is_same(struct sockaddr_in s1, struct sockaddr_in s2){
	if (!test_flag)
		return;
	showSep();
	printf("_test_address_is_same:\n");
	showD("same_addresses", address_is_same(s1, s1));
	showD("same_addresses", address_is_same(s2, s2));
	showD("diff_addresses", address_is_same(s1, s2));
}
void _test_client_is_new(struct sockaddr_in s1, struct sockaddr_in s2){
	if (!test_flag)
		return;
	showSep();
	printf("_test_address_is_same:\n");
	
	struct sockaddr_in *client_list = malloc(ARRAYSIZE * sizeof(struct sockaddr_in));
	bzero(client_list, ARRAYSIZE * sizeof(struct sockaddr_in));
	int client_list_counter = 0;	
	client_list[client_list_counter++] = s1;
	showD("client_list_counter(1)", client_list_counter);
	int new = client_is_new(s2, client_list, client_list_counter);
	showD("is new", new);
	int old = client_is_new(s1, client_list, client_list_counter);
	showD("is NOT new", old);	
	client_list[client_list_counter++] = s2;
	showD("client_list_counter(2)", client_list_counter);
	int oldd = client_is_new(s2, client_list, client_list_counter);
	showD("is NOT new", oldd);	
	free(client_list);	
}






