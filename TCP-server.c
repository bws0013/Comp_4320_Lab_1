/*
* To run this compile it using cc TCP-server.c -o TCP-server then ./TCP-server
* TODO: remove print statements before submitting. 
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT "10021"  // Change this to 10021

#define BACKLOG 10	 // how many pending connections queue will hold

// The packet recieved from the client
struct __attribute__((__packed__)) packet {
		char tml; // Message length in bytes
		char reqId;	// Request Id
		char opC; // Opcode 
		char numOp; // Number of opperands (This doesn't ever get used)
		unsigned short o1; // Operand 1
		unsigned short o2; // Operand 2
} my_packet;

// The packed sent back to the client
struct __attribute__((__packed__)) returnPacket {
		char tml; // Message length in bytes
		char reqId; // Request Id
		char errorCode; // Error code (0 for no errors, 1 for errors)
		unsigned int finAnswer; // Returned answer
} return_packet;



void sigchld_handler(int s)
{
	// waitpid() might overwrite errno, so we save and restore it:
	int saved_errno = errno;

	while(waitpid(-1, NULL, WNOHANG) > 0);

	errno = saved_errno;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("server: waiting for connections...\n");

	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		printf("server: got connection from %s\n", s);

		int MAXDATASIZE = 100;
		char buf[MAXDATASIZE];
		int numbytes;  

		if ((numbytes = recv(new_fd, buf, MAXDATASIZE-1, 0)) == -1) {
	    	perror("recv");
	    	exit(1);
		}

		my_packet = *((struct packet *)buf);

		char error = '0';

 		char opcode = my_packet.opC; // The number of the operation to be done.
 		int operand1 = (int)my_packet.o1; // The first operand
 		int operand2 = (int)my_packet.o2; // The second operand
 		unsigned int answer = 0; // The final answer to be returned. 
 		if(opcode == '1') { 
 			answer = operand1 + operand2; // Addition
 		} else if(opcode == '2') {
 			answer = operand1 - operand2; // Subtractions
 		} else if(opcode == '3') {
 			answer = operand1 | operand2; // OR
 		} else if(opcode == '4') {
 			answer = operand1 & operand2; // AND
 		} else if(opcode == '5') {
 			answer = operand1 >> operand2; // Shift Right 
 		} else if(opcode == '6') {
 			answer = operand1 << operand2; // Shift Left
 		} else {
 			error = '1'; // Pass back an error. 
 		}
 
 		if(my_packet.tml != '8') { // Check that the message is the correct lenght
 			error = '1'; // Throw error if not 8 bytes. 
 		}

 		typedef struct return_packet packet_r; 
 		return_packet.tml = '7'; // Return packet lenght
 		return_packet.reqId = my_packet.reqId; // Return packet request id.
 		return_packet.errorCode = error; // Return packet error code. 
 		return_packet.finAnswer = answer; // Return packet answer. 

 		// The below can be removed in the final version. It is for testing purposes
 		char r_length = return_packet.tml; 
		char r_id = return_packet.reqId;
		char r_error = return_packet.errorCode;
		unsigned int r_answ = return_packet.finAnswer;

		printf("TML: %c\n",r_length);
		printf("ReqId: %c\n",r_id);
		printf("ErrorCode: %c\n",r_error);
		printf("Answer: %d\n",r_answ);
		// The above can be removed in the final version. It is for testing purposes

		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener
			if (send(new_fd, (char*)&return_packet, return_packet.tml, 0) == -1)
				perror("send");
			close(new_fd);
			exit(0);
		}

		close(new_fd);  // parent doesn't need this
	}

	return 0;
}

