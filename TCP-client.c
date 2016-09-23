/*
* Using this as a demo tool. 
* To run this compile using cc TCP-client.c -o TCP-client then ./TCP-client 127.0.0.1
* TODO: Learn go... and use it to actually test the server.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT "10021" // the port client will be connecting to 

#define MAXDATASIZE 100 // max number of bytes we can get at once 

struct __attribute__((__packed__)) packet {
		char tml; // Message length in bytes
		char reqId;	// Request Id
		char opC; // Opcode 
		char numOp; // Number of opperands (This doesn't ever get used)
		unsigned short o1; // Operand 1
		unsigned short o2; // Operand 2
} my_packet;

struct __attribute__((__packed__)) returnPacket {
		char tml; // Message length in bytes
		char reqId; // Request Id
		char errorCode; // Error code (0 for no errors, 1 for errors)
		unsigned int finAnswer; // Returned answer
} return_packet;

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
	int sockfd, numbytes;  
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	if (argc != 2) {
	    fprintf(stderr,"usage: client hostname\n");
	    exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("client: connecting to %s\n", s);
	
	typedef struct packet packet_t;
	char requestCode[1];
	unsigned short fakeOperand1[1];
	unsigned short fakeOperand2[1];
	unsigned short Operand1;
	unsigned short Operand2;

	printf("Enter OpCode: ");
	scanf("%c", requestCode);

	char fake[1];
	scanf("%c", fake);

	printf("Operand1: ");
	scanf("%hu", fakeOperand1);

	scanf("%c", fake);

	printf("Operand2: ");
	scanf("%hu", fakeOperand2);

	packet_t my_packet;

	Operand1 = fakeOperand1[0];
	Operand2 = fakeOperand2[0];

	int tempSize = (int) sizeof(my_packet);
	char tempSizeChar = tempSize + '0';

	my_packet.tml = tempSizeChar;
	my_packet.reqId = '1';
	my_packet.opC = requestCode[0];
	my_packet.o1 = Operand1;
	my_packet.o2 = Operand2; 
	my_packet.numOp = '2';

	scanf("%c", fake);
	if(send(sockfd, (char*)&my_packet, my_packet.tml, 0) < 0)
    {
        puts("Send failed");
        return 1;
    }
	//--------------------
	freeaddrinfo(servinfo); // all done with this structure

	char ret[MAXDATASIZE];

	if ((numbytes = recv(sockfd, ret, MAXDATASIZE-1, 0)) == -1) {
	    perror("recv");
	    exit(1);
	}

	return_packet = *((struct returnPacket *)ret);

	char r_length = return_packet.tml;
	char r_id = return_packet.reqId;
	char r_error = return_packet.errorCode;
	unsigned int r_answ = return_packet.finAnswer;

	printf("TML: %c\n",r_length);
	printf("ReqId: %c\n",r_id);
	printf("ErrorCode: %c\n",r_error);
	printf("Answer: %d\n",r_answ);
	
	close(sockfd);

	return 0;
}

