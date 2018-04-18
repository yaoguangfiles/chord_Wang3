#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <stdio.h>

#include "RPCClient.h"

RPCClient::RPCClient()
{
}

void RPCClient::rpc_request(const string &serverIP, const string &localIP, const string &message)
{
    const char *serverIPChars = serverIP.c_str();
//    const char *messageChars = message.c_str();
    const char *localIPChars = localIP.c_str();
    const char *port = "12345";

    char *messageChars2 = new char[message.length() + 1];
    strcpy(messageChars2, message.c_str());

    cout << "(Info): send a message: '" << messageChars2
    	 << "' to node (ip: " << serverIPChars << ") from node (ip: " << localIPChars << ")." << endl;


	cout << "(Info): Delay 2 second before send the message." << endl;
	sleep(2);

    this->sendMessage(serverIPChars, port, localIPChars, messageChars2);
}

//void  RPCClient::rpc_request( const string &serverIP, const string &localIP, const string &message,  int fingerIndex, node &n)
//{
//
//	const char *serverIPChars = serverIP.c_str();
//	const char *messageChars = message.c_str();
//	const char *localIPChars = localIP.c_str();
//	char *port = "12345";
//
//	char *messageChars2 = new char[message.length() + 1];
//	strcpy(messageChars2, message.c_str());
//
//	cout << "[RPCClient.cpp][rpc_request()]: send a message: '" << messageChars2
//		 << "' to node (ip: " << serverIPChars << ") from node (ip: " << localIPChars << ")." << endl;
//
//
////	cout << "(Info): The node server (ip: " << n.networkIp << ") received package string :|"
////					  << receivedStr << "| from node server (ip: " << n.networkIp << ")." << endl;
////
////	package receivedPkg = stringToPackage(receivedStr);
//	//end recursive section
//
//	// the passed request command will be in the form of "lookup(targetkey){flag}"
//
//	// extract the key from the received buffer.
//	int targetKey = std::stoi( getSubStr(message, '(', ')') );
//
//	// extract from the received buffer.
//	int flag      = std::stoi( getSubStr(message, '{', '}') );
//
//	string resultStr = ""; // the result produced by this node.
//
//	if( flag == 2 )
//		updateQ( n, fingerIndex, receivedPkg.propagation );
//
//	result.ip = receivedPkg.ip;
//	result.totalTime = receivedPkg.totalTime + n.fingerTable[fingerIndex].latency;
//
//	// return result;
//	resultStr = packageToString( result );
//
//	this->sendMessage(serverIPChars, port, localIPChars, messageChars2);
//}

string RPCClient::sendMessage(const char *serverIP, const char *port, const char *localIP, char *message)
{
    struct sockaddr_in echoserver;  // structure for address of the server.

    int sock;

    // start to configure the client socket.
    struct sockaddr_in echoserverConf = configureClient(serverIP, port, sock, echoserver, localIP);

    // send the message to the server.
    string str = sendMessage(sock, echoserverConf, message);

    // close the socket.
    closeSock(sock);

    return str;
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
struct sockaddr_in RPCClient::configureClient(const char *ip, const char *port, int &sock, struct sockaddr_in echoserver, const char *localIP)
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
string RPCClient::sendMessage(int sock, struct sockaddr_in echoserver, char *message)
{
    char buffer[256];
    int echolen, received = 0;
    unsigned int addrlen;

    // Send the message to the server
    echolen = strlen(message);

    int byteSend = sendto(sock, message, strlen(message), 0, (struct sockaddr *) &echoserver, sizeof(echoserver));
    if ( byteSend != echolen )
    {
    	cout << "[RPCClient.cpp][rpc_request()]: May cause a 'Mismatch error'. byteSent: "
    		 << byteSend << ", echolen: " << echolen << endl;
        perror("Mismatch in number of sent bytes");
        exit(EXIT_FAILURE);
    }

    // Receive the message back from the server
    addrlen = sizeof(echoserver);

    received = recvfrom(sock, buffer, 256, 0, (struct sockaddr *) &echoserver, &addrlen);

    buffer[received] = '\0';        /* Assure null-terminated string */
    cout << "Server (" << inet_ntoa(echoserver.sin_addr) << ") echoed: " << buffer << endl;

    string str(buffer);

    return str;
}

/**
 * Close the client UDP socket.
 * @para sock The socket descriptor.
 * @return No return value.
 */
void RPCClient::closeSock(int sock)
{
    close(sock);
}
