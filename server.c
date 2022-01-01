// Server side C/C++ program to demonstrate Socket programming
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>


#define PORT 8080
#define BUFFER_SIZE 500
#define WINDOW_SIZE 5


struct Packet {
	int seqNum;
	int size;
	char data[BUFFER_SIZE];
	bool sent;
	bool received;
	bool ack;
	bool eof;
};

struct ackPacket
{
   int seqNum;
   int size;
   bool eof;
};


int num_of_pkt_wait_for = 0;
int num_of_acked_pkts = 0;
int n;
int udpSocket;
struct sockaddr_in address;
int addrlen = sizeof(address);
char buffer[BUFFER_SIZE];
FILE *output_file;



int seq_num = 0;
struct ackPacket ack;
int last_acked_in_sequence; 
bool eof_reached;
struct Packet window[WINDOW_SIZE];

bool eof_reached = false;


void printWindow();


void addPacket() {
   /* shifting array elements */
   n = WINDOW_SIZE;
   for(int i=0; i < n-1; i++){
      window[i] = window[i+1];
   }
	// puts("testing");

   struct Packet nullPkt;
   nullPkt.seqNum = -2;
   window[n-1] = nullPkt;
}


void printAckWindow() {
	printf("[");
	for (int i = 0; i < WINDOW_SIZE; i++)
	{
		printf("%d, ", window[i].ack);
	}
	printf("]\n");
	
}

void slideWindow(int steps) {

	// printf("steps: %d\n", steps);
   for (int i = 0; i < steps; i++)
   {
		struct Packet packet = window[i];
		printf("writting: %d\n", packet.seqNum);
		
		size_t num_write = fwrite(&packet.data, 1, packet.size, output_file);

		
		if (num_write == 0) {
			packet.eof = true;
			break;
		}
	}

	for (int i = 0; i < WINDOW_SIZE; i++)
	{
		addPacket();
	}
}


void socketConnection() {
	
	udpSocket = socket(AF_INET, SOCK_DGRAM, 0);

	if (udpSocket < 0) {
		perror("[-]socket failed");
		exit(EXIT_FAILURE);
	}
	else {
		puts("[+]Socket created");
	}

	// Client config
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons( PORT );
	

	// Binding

	if (bind(udpSocket, (struct sockaddr *)&address, sizeof(address))<0) {
		perror("[-]bind failed");
		exit(EXIT_FAILURE);
	}
	
}

/**
 * Reading segments till 0 bytes received from client
*/


void initWindowSeq();
void recvOverUDP();
void threadSendAck();
void checkAckWindow();
// void *send_ack(struct Packet *packet);


void rescOverR_Udp() {

	num_of_pkt_wait_for = WINDOW_SIZE;
	initWindowSeq();
	
	rescAgain:
	recvOverUDP();

	// if all pkts acked else send acks
	if (num_of_acked_pkts < WINDOW_SIZE-1) {
		num_of_pkt_wait_for = WINDOW_SIZE - num_of_acked_pkts;
		goto rescAgain;
	}

	if (eof_reached == false) {
		slideWindow(WINDOW_SIZE);
		rescOverR_Udp();
	} 

	// eof reached  
	else {
		slideWindow(num_of_pkt_wait_for+1);
	}

}

/**
 * init a pkt array to be received 
*/

void initWindowSeq() {
	seq_num = 0;

	for (int i = 0; i < WINDOW_SIZE; i++)
	{
			struct Packet newPkt;
			newPkt.seqNum = seq_num;
			newPkt.received = false;
			seq_num++;
			window[i] = newPkt;
	}
}


void printWindow() {
	puts("Accepted seq nums: ");
	printf("[");
	for (int i = 0; i < WINDOW_SIZE; i++)
	{
		printf("%d, ", window[i].seqNum);
	}
	printf("]\n");
	
}

void recvOverUDP() {
   bzero(buffer, sizeof(buffer));
//    printWindow();

   for (int i = 0; i < num_of_pkt_wait_for; i++) {

		struct Packet newPacket;
		newPacket.eof = false;

		n = recvfrom(udpSocket, &newPacket, sizeof(newPacket), MSG_WAITALL, ( struct sockaddr *) &address, &addrlen);

		int num = newPacket.seqNum;

		// if the pkt is already recieved then ignore it
		if (!window[num].received) 
		{
			printf("packet received: %d\n", num);
			window[num] = newPacket;
			window[num].received = true;
			struct ackPacket ack;
			ack.seqNum = num;
			ack.eof = newPacket.eof;

			int n = sendto(udpSocket, &ack, sizeof(ack), MSG_CONFIRM, (const struct sockaddr *) &address, addrlen);
			window[num].ack = true;
			printf("Ack sent: %d\n",num);
			num_of_acked_pkts++;
		}

		if (newPacket.eof == true) {
			printf("eof seq num %d\n", newPacket.seqNum);
			eof_reached = true;
			num_of_pkt_wait_for = newPacket.seqNum;
			break;
		}

		if (newPacket.size < 0) // Error
		perror("[-]ERROR writing to socket");
      
   }
}



void checkAckWindow() {
   for (int i = 0; i < WINDOW_SIZE; i++)
   {
		// printf("pkt ackted: %d", window[i].ack);
		if (window[i].ack == true)
			last_acked_in_sequence = i;
		else {
			break;
		}
   }
}



int main(int argc, char const *argv[])
{


/**
 * 
 * TRANSFER FILE OVER UDP
 * 
*/
	puts("\n");
	puts("USING UDP CONNECTION");
	socketConnection();
	puts("[+]Client found");
	puts("[+]Receiving file from the clinet.");
	// file = open(recVideoFile, O_RDWR | O_CREAT, 0755);

    output_file = fopen("outUDP.mp4", "wb");

	rescOverR_Udp();
  	
	  
	puts("[+]File is received and saved in outUDP.mp3.");
	if (output_file != NULL)  
		fclose(output_file);
    close(udpSocket);
	puts("[+]Closing Connection.");

	return 0;
}
