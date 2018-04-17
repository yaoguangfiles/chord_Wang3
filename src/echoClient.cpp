/*
 * echoClient.cpp
 * 
 * 	The client will send message to the server via UDP. The client will
 * 	waits for receiving the message from server after it sends the
 * 	message. sends message to echo server.
 * 	      
 * 	command line arguments:
 * 		argv[1] IP number of server
 * 		argv[2] port number to send to
 * 		argv[3] message to send
 *  
 */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
using namespace std;

struct sockaddr_in configureClient(char *ip, char *port, int &sock, struct sockaddr_in echoserver, const char *localIP);
void sendMessage(int sock, struct sockaddr_in echoserver, char *message);
void closeSock(int sock);

int main(int argc, char *argv[])
{
	if (argc != 4) {
		cerr << "USAGE: echoClient server_ip port message\n";
		exit(EXIT_FAILURE);
	}
	
	struct sockaddr_in echoserver;  // structure for address of the server.

	char *port = argv[2]; // the port will be used for UDP messages
	                      // transmission.
    char *message = argv[3]; // the UDP message the client will send to the server.
    int sock;

    const char *localIP = "127.0.0.2"; // assign this ip to local client.
//    char *serverIP = "127.0.0.2"; // the server IP address that the client will
//    	                          // connect to.
    char *serverIP = argv[1]; // the server IP address that the client will
       	                          // connect to.

    // start to configure the client socket.
    struct sockaddr_in echoserverConf = configureClient(serverIP, port, sock, echoserver, localIP);

    // send the message to the server.
	sendMessage(sock, echoserverConf, message);

	// close the socket.
    closeSock(sock);
	
	return 0;
}

/**
 * Configure the client for the socket for UDP.
 * @para ip The ip address to which the message will be sent to.
 * @para port The port through which the message will be passed.
 * @para sock The socket descriptor.
 * @para echoserver The structure describing an Internet socket address.
 * @return sockaddr_in The structure describing an Internet socket address.
 *     I will be used for sending and receiving messages from the server.
 */
struct sockaddr_in configureClient(char *ip, char *port, int &sock, struct sockaddr_in echoserver, const char *localIP)
{
    // Create the UDP socket
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }



    // Bind to a specific network interface (and optionally a specific local port)
    struct sockaddr_in localaddr;
    localaddr.sin_family = AF_INET;
    localaddr.sin_addr.s_addr = inet_addr(localIP);
    localaddr.sin_port = 0;  // Any local port will do
    bind(sock, (struct sockaddr *)&localaddr, sizeof(localaddr));

    // Construct the server sockaddr_in structure
    memset(&echoserver, 0, sizeof(echoserver));        /* Clear struct */
    echoserver.sin_family = AF_INET;                   /* Internet/IP */
    echoserver.sin_addr.s_addr = inet_addr( ip );   /* IP address */
    echoserver.sin_port = htons(atoi(port));        /* server port */

    return echoserver;
}

/**
 * The client will send a message to the server.
 * @para sock The socket descriptor.
 * @para echoserver The structure describing an Internet socket address.
 * @para message The message will be sent to the server.
 * @return No return value.
 */
void sendMessage(int sock, struct sockaddr_in echoserver, char *message)
{
    char buffer[256];
    int echolen, received = 0;
    unsigned int addrlen;

    // Send the message to the server
    echolen = strlen(message);
    if (sendto(sock, message, strlen(message), 0, (struct sockaddr *) &echoserver, sizeof(echoserver)) != echolen) {
        perror("Mismatch in number of sent bytes");
        exit(EXIT_FAILURE);
    }

    // Receive the message back from the server
    addrlen = sizeof(echoserver);
//    if ((received = recvfrom(sock, buffer, 256, 0, (struct sockaddr *) &echoserver, &addrlen)) != echolen+6) {
//        perror("Mismatch in number of received bytes");
//        exit(EXIT_FAILURE);
//    }

    if( strncmp(message,"closed", 6) != 0 )
    {
        received = recvfrom(sock, buffer, 256, 0, (struct sockaddr *) &echoserver, &addrlen);

        buffer[received] = '\0';        /* Assure null-terminated string */
        cout << "Server (" << inet_ntoa(echoserver.sin_addr) << ") echoed: " << buffer << endl;

    }
//    else{
//        cout << "Try to close the server." << endl;
//    }

}

/**
 * Close the client UDP socket.
 * @para sock The socket descriptor.
 * @return No return value.
 */
void closeSock(int sock)
{
    close(sock);
}
           
