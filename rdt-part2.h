/**************************************************************
rdt-part2.h
Student name:Aakansha Parmar	
Student No. :2012622238
Date and version:02/04/2015 2
Development platform:Ubuntu 14.04
Development language:C
Compilation:Can be compiled with GCC
	
*****************************************************************/

#ifndef RDT2_H
#define RDT2_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>

#define PAYLOAD 1000		//size of data payload of the RDT layer
#define TIMEOUT 50000		//50 milliseconds
#define TWAIT 10*TIMEOUT	//Each peer keeps an eye on the receiving  
							//end for TWAIT time units before closing
							//For retransmission of missing last ACK

//----- Type defines ----------------------------------------------------------
typedef unsigned char		u8b_t;    	// a char
typedef unsigned short		u16b_t;  	// 16-bit word
typedef unsigned int		u32b_t;		// 32-bit word 

extern float LOSS_RATE, ERR_RATE;


//---------Define Packet Structure----------------------------------------------
//ACK structure 
struct AckPack
{
	u8b_t pcktType; 	// Type of Packet A for Acknowledgement D for Data Packet
	u8b_t seqNo; 		// 0 or 1 depending on sequence of packet 
	u16b_t checksumValue; 	//Checksum value of packet
	u16b_t payLoadLen; 	//Lenght of Payload
	
} *ACKpkt; 

//Packet Structure
struct Packet 
{
	u8b_t pcktType; 	// Type of Packet A for Acknowledgement D for Data Packet
	u8b_t seqNo; 		// 0 or 1 depending on sequence of packet 
	u16b_t checksumValue; 	//Checksum value of packet
	u16b_t payLoadLen; 	//Lenght of Payload
	char payLoad[1000];		//Payload of data Packet
} *sndpkt;


	
/* this function is for simulating packet loss or corruption in an unreliable channel */
/***
Assume we have registered the target peer address with the UDP socket by the connect()
function, udt_send() uses send() function (instead of sendto() function) to send 
a UDP datagram.
***/
int udt_send(int fd, void * pkt, int pktLen, unsigned int flags) {
	double randomNum = 0.0;
	
	/* simulate packet loss */
	//randomly generate a number between 0 and 1
	randomNum = (double)rand() / RAND_MAX;
	if (randomNum < LOSS_RATE){
		//simulate packet loss of unreliable send
		printf("WARNING: udt_send: Packet lost in unreliable layer!!!!!!\n");
		return pktLen;
	}

	/* simulate packet corruption */
	//randomly generate a number between 0 and 1
	randomNum = (double)rand() / RAND_MAX;
	if (randomNum < ERR_RATE){
		//clone the packet
		u8b_t errmsg[pktLen];
		memcpy(errmsg, pkt, pktLen);
		//change a char of the packet
		int position = rand() % pktLen;
		if (errmsg[position] > 1) errmsg[position] -= 2;
		else errmsg[position] = 254;
		printf("WARNING: udt_send: Packet corrupted in unreliable layer!!!!!!\n");
		return send(fd, errmsg, pktLen, 0);
	} else 	// transmit original packet
		return send(fd, pkt, pktLen, 0);
}

/* this function is for calculating the 16-bit checksum of a message */
/***
Source: UNIX Network Programming, Vol 1 (by W.R. Stevens et. al)
***/
u16b_t checksum(u8b_t *msg, u16b_t bytecount)
{
	
	u32b_t sum = 0;
	u16b_t * addr = (u16b_t *)msg;
	u16b_t word = 0;
	
	// add 16-bit by 16-bit
	while(bytecount > 1)
	{
		sum += *addr++;
		bytecount -= 2;
	}
	
	// Add left-over byte, if any
	if (bytecount > 0) {
		*(u8b_t *)(&word) = *(u8b_t *)addr;
		sum += word;
	}
	
	// Fold 32-bit sum to 16 bits
	while (sum>>16) 
		sum = (sum & 0xFFFF) + (sum >> 16);
	
	word = ~sum;
	
	return word;
}

//Funtion to make a packet 
void make_pkt(u8b_t seq, char* pcktData, u16b_t check, u8b_t type, int len)
{	
	//Set Values
	sndpkt= (Packet*)malloc (sizeof(struct Packet));
	sndpkt->pcktType= type;	
	sndpkt->seqNo=seq;
	sndpkt->checksumValue = check;
	memcpy(sndpkt->payLoad, pcktData, len);
	sndpkt->payLoadLen=len;
	sndpkt->checksumValue = checksum((u8b_t*) sndpkt,(u16b_t)(sizeof(struct AckPack)+len));	
}

//Function to make ACK Packet 
void make_ACKpkt(u8b_t seq, u16b_t check, u8b_t type, int len)
{

	//Set Values
	ACKpkt= (AckPack*)malloc (sizeof(struct AckPack));
	ACKpkt->pcktType= type;	
	ACKpkt->seqNo=seq;
	ACKpkt->payLoadLen=0;
	ACKpkt->checksumValue= check;
	ACKpkt->checksumValue= checksum((u8b_t*)ACKpkt,(u16b_t)(sizeof(struct AckPack)));
}


//----- Type defines ----------------------------------------------------------

// define your data structures and global variables in here

u8b_t prevSeqNo=1; //Sequence number of packet already sent 
u8b_t expectedSeqNo=0; //Sequence number of expected packet
int packetSize;//Size of packet being sent 

struct sockaddr_in peer_addr;
struct sockaddr_in my_addr;
struct hostent *host;
int sentByte;
socklen_t sin_size;

//Function to check if packet received is ACK 
int isACK(void* ACK)
{
	if(((AckPack *)ACK)->pcktType=='A')
		return 1;
	else 
		return 0;
}

//Function to check if packet received is Data Packet
int isData(void* Data)
{
	if(((Packet *)Data)->pcktType=='D')
		return 1;
	else 
		return 0;
}

//Function to check if correct Sequence Number for ACK
int chkSeqNo(void* ACK)
{
	if(((AckPack *)ACK)->seqNo == prevSeqNo)
		return 1;
	else 
		return 0;
}

//Function to check if correct Sequence Number for DATA
int chkSeqNoData(void* ACK)
{
	if(((AckPack *)ACK)->seqNo == expectedSeqNo)
		return 1;
	else 
		return 0;
}

//Function to check if corrupt
int isCorrupt(void* Pck, int len)
{
	
	return checksum((u8b_t*)Pck,(u16b_t)len);
}

int rdt_socket();
int rdt_bind(int fd, u16b_t port);
int rdt_target(int fd, char * peer_name, u16b_t peer_port);
int rdt_send(int fd, char * msg, int length);
int rdt_recv(int fd, char * msg, int length);
int rdt_close(int fd);

/* Application process calls this function to create the RDT socket.
   return	-> the socket descriptor on success, -1 on error 
*/
int rdt_socket()
{
	
	int sockfd = socket (AF_INET, SOCK_DGRAM, 0);
	
	if(sockfd==-1)
	{
		perror("Unsuccessful\n");
		return -1;
	}
	else 
		return sockfd;
}

/* Application process calls this function to specify the IP address
   and port number used by itself and assigns them to the RDT socket.
   return	-> 0 on success, -1 on error
*/
int rdt_bind(int fd, u16b_t port)
{
	
	my_addr.sin_family = AF_INET;         // host byte order	
    	my_addr.sin_port = htons(port);     // short, network byte order
    	my_addr.sin_addr.s_addr = htonl(INADDR_ANY); // automatically fill with my IP address
    	memset(&(my_addr.sin_zero), '\0', 8); // zero the rest of the struct

	/* Associate my address info to the socket */
    	if (bind(fd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) 
	{
        	perror("Unsuccessful\n");
        	return -1;
        }
	else 
		return 0;
}

/* Application process calls this function to specify the IP address
   and port number used by remote process and associates them to the 
   RDT socket.
   return	-> 0 on success, -1 on error
*/
int rdt_target(int fd, char * peer_name, u16b_t peer_port)
{
	
	host = gethostbyname(peer_name);   
 
      	peer_addr.sin_family = AF_INET;    // host byte order 
      	peer_addr.sin_port = htons(peer_port);  // short, network byte order 
      	peer_addr.sin_addr = *((struct in_addr *)host->h_addr); // already in network byte order
      	memset(&(peer_addr.sin_zero), '\0', 8);  // zero the rest of the struct 

      	if (connect(fd, (struct sockaddr *)&peer_addr,  sizeof(struct sockaddr_in)) == -1)
	{
        	perror("Unsuccessful\n");
       		 return -1;
      	}
        else
        	return 0;
}

/* Application process calls this function to transmit a message to
   target (rdt_target) remote process through RDT socket.
   msg		-> pointer to the application's send buffer
   length	-> length of application message
   return	-> size of data sent on success, -1 on error
*/
int rdt_send(int fd, char * msg, int length)
{
	//implement the Stop-and-Wait ARQ (rdt3.0) logic
	//must use the udt_send() function to send data via the unreliable layer
	
	AckPack recvPkt; //ACK Packet to be sent
	u8b_t currentSeqNo;//Current sequence number of packet being sent
	fd_set readfds; 
	
	//Check Sequence Number 
	if(prevSeqNo==1)
	{
		currentSeqNo=0;
		prevSeqNo=0;
	}
	else
	{
		currentSeqNo=1; 
		prevSeqNo=1;
	}

	//Make Packet 
	make_pkt (currentSeqNo, msg, 0, 'D', length);
	printf("Packet Made with sequence number%d\n", currentSeqNo);
	
	//Calculate packet size 
	packetSize = sizeof(struct AckPack) + sndpkt->payLoadLen ; 

	//Send the message
	int sendResult = udt_send(fd, sndpkt, packetSize, 0);
	printf("Packet of size - %d sent\n",sizeof(sendResult)-sizeof(AckPack));
 
	//Wait for ACK or timeout
	int result;
	
	while(1)
	{
		
		//Set timeout values
		struct timeval timeout;
		timeout.tv_sec = 0;
  		timeout.tv_usec = TIMEOUT;

		//Reset file descriptors
		FD_ZERO(&readfds);
		FD_SET(fd,&readfds); 
  		
  		//Call Select function() and Wait
  		result = select(fd+1,&readfds , NULL, NULL, &timeout);
  		
  		//In case of timeout 
		if(result == 0)
  		{
  			
			printf("Timeout has occurred\n");
			
			//Resend Packet
			sendResult = udt_send(fd, sndpkt, packetSize, 0);
			printf("Packet resent\n");
			timeout.tv_sec = 0;
  			timeout.tv_usec = TIMEOUT;
			
			
		}

		//If Packet arrives
		else if(result == 1)
		{
			if(FD_ISSET(fd,&readfds))
			{
				
				//Receive Packet
				int recvResult=recv(fd,(void*) &recvPkt, sizeof(struct AckPack), 0);
				
				//Check if acknowledgement/corrupt/correct sequence number
				int chkACK = isACK( &recvPkt);
				int chkCor = isCorrupt ( &recvPkt, recvResult);
				int chkSeq = chkSeqNo( &recvPkt);

				//If it is an acknowledgement packet
				if(chkACK==1)
				{
					//If sequence number and checksum are correct
					if((chkCor==0) && (chkSeq==1))
					{
						printf("ACK for sequence number-%d received\n", recvPkt.seqNo);  
					 	break; 
					}
					//If either sequence number or checksum is wrong
					else if ((chkCor != 0) || (chkSeq == 0 ))
					{
						timeout.tv_sec = 0;
  						timeout.tv_usec = TIMEOUT;
					}
				}
				//If Data Packet arrives
				else if (chkACK==0)
				{
					int b;
					if(expectedSeqNo==0) b=1;
					else b=0;
					//Check if it is a retransmitted message 
					if(recvPkt.seqNo==b)
					{
						printf("Sender got old retransmitted message sending ACK\n");
						
						//Make ACK Packet
						AckPack recvACK; 
						recvACK.seqNo=recvPkt.seqNo;
						recvACK.pcktType='A';
						recvACK.checksumValue= 0;
						recvACK.checksumValue= checksum((u8b_t*) &recvACK,(u16b_t)(sizeof(struct AckPack)));

						//Calculate packet size 
						int ACKpacketSize = sizeof(struct AckPack); 

						//Send acknowledgement
						udt_send(fd, &recvACK, ACKpacketSize, 0);
					}
					else
					{	printf("Sender got new data packet ignoring it\n");}

				}

			}

			
		}

		//If error occurs
		else if(result == -1)
		{
			perror("Error in select() fxn\n");
			exit(0);
		}
		

	} 


	return (sendResult - sizeof(struct AckPack));
	
}

/* Application process calls this function to wait for a message 
   from the remote process; the caller will be blocked waiting for
   the arrival of the message.
   msg		-> pointer to the receiving buffer
   length	-> length of receiving buffer
   return	-> size of data received on success, -1 on error
*/
int rdt_recv(int fd, char * msg, int length)
{
	//implement the Stop-and-Wait ARQ (rdt3.0) logic
	
	int recvResult;
	Packet recvPkt;
	
	
	while(1)
	{	
		printf("Waiting for Packet\n");
		//Wait for packet to arrive 
		recvResult=recv(fd, (void *) &recvPkt, sizeof(Packet) , 0);
		printf("Packet Arrived\n");
		//Check if data packet , has correct sequence, is not corrupt. 
		int chkData = isData( &recvPkt);
		int chkCor = isCorrupt ( &recvPkt, recvResult);
		int chkSeq = chkSeqNoData( &recvPkt);
		
		//If packet received is a data packet 
		if(chkData ==1)
		{
			//If data packet is not corrupt and sequence number is correct
			if((chkCor==0)&&(chkSeq==1))
			{
				printf("Packet of Sequence Number- %d is not corrupt and has correct sequence\n", expectedSeqNo);
				
				//Make Acknowledgement Packet
				make_ACKpkt (expectedSeqNo, 0, 'A', length);

				//Calculate ACK size 
				int ACKpacketSize = sizeof(struct AckPack);

				//Send ACK
				int sendResult = udt_send(fd, ACKpkt, ACKpacketSize, 0);
				printf("Acknowledgement for sequence number- %d is sent of size- %d\n", expectedSeqNo, sendResult);
				if(expectedSeqNo==0) expectedSeqNo=1;
				else if(expectedSeqNo==1) expectedSeqNo=0;
				
				//Return and Extract data from Packet
				memcpy(msg,recvPkt.payLoad,sizeof(recvPkt.payLoad));

				break;

			}
			// If Data packet is corrupt or wrong sequence number 
			else if ((chkCor!=0) || (chkSeq==0))
			{
				printf("Corrupted / Wrong sequence packet received");

				//Resend acknowledgement of past 
				int a;
				if(expectedSeqNo==0) a=1;
				else a=0;

				
				//Make Acknowledgement Packet of older packet
				make_ACKpkt (a, 0, 'A', length);

				//Calculate ACK size 
				int ACKpacketSize = sizeof(struct AckPack);

				//Send ACK
				int sendResult = udt_send(fd, ACKpkt, ACKpacketSize, 0);
				printf("Previous acknowledgement for sequence number- %d is sent of size- %d\n", a, sendResult);


			}
		}
		//If ACK Packet Arrives
		else if(chkData==0)
		{ 	printf("Receiver got ACK packet ignoring it\n");}

	}
	
	return (recvResult-sizeof(AckPack)); 

}



/* Application process calls this function to close the RDT socket.
*/
int rdt_close(int fd)
{
//implement the Stop-and-Wait ARQ (rdt3.0) logic

	int result;
	fd_set readfds; 
	Packet recvPkt;

	while(1)
	{
		
  		struct timeval timeout;
		timeout.tv_sec = 0;
  		timeout.tv_usec = TWAIT;
		FD_ZERO(&readfds);//reset 
		FD_SET(fd,&readfds);

		//Wait for last packet to come 
		result = select(fd+1,&readfds , NULL, NULL, &timeout);
  		
		//If timeout happens end program
		if(result==0)
  		{
  			break;
  		}
  		
		//If Packet arrives 
		else if(result==1)
  		{
  			if(FD_ISSET(fd,&readfds))
			{
				
				//In case packet is received.
				int recvResult=recv(fd,(void*) &recvPkt, sizeof(struct Packet), 0);
				printf("Received a packet\n");
				int chkData = isData( &recvPkt);
				int chkCor = isCorrupt ( &recvPkt, recvResult);
				
				
				if((chkData==1) && (chkCor==0))
				{	
						printf("Last packet received is Data packet and is not corrupt\n");
						int ACKpacketSize = sizeof(struct AckPack); 
						make_ACKpkt (recvPkt.seqNo, 0, 'A', ACKpacketSize);

						//Send acknowledgement
						int sendResult = udt_send(fd, ACKpkt, ACKpacketSize, 0);
						printf("Last ACK size- %d sent\n", sendResult);
						timeout.tv_sec = 0;
  						timeout.tv_usec = TWAIT;

				}
			    
			 }                                                     
				
				
			
		}
		else if(result == -1)
		{
			perror("Error in select() fxn\n");
			exit(0);
		}

	}
	
	printf("Socket Closed\n");
	return close(fd);

}

#endif
