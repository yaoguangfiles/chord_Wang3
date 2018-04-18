#ifndef RPC_CLIENT_H
#define RPC_CLIENT_H

/*
 * echoClient.cpp
 *
 *  The client will send message to the server via UDP. The client will
 *  waits for receiving the message from server after it sends the
 *  message. sends message to echo server.
 *
 *  command line arguments:
 *      argv[1] IP number of server
 *      argv[2] port number to send to
 *      argv[3] message to send
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

class RPCClient
{
    public:
        RPCClient();
        void rpc_request(const string &serverIP, const string &localIP, const string &message);
        string sendMessage(const char *serverIP, const char *port, const char *localIP, char *message);
        struct sockaddr_in configureClient(const char *ip, const char *port, int &sock
                , struct sockaddr_in echoserver, const char *localIP);
//        void sendMessage(int sock, struct sockaddr_in echoserver, char *message);
        string sendMessage(int sock, struct sockaddr_in echoserver, char *message);
        void closeSock(int sock);
};



#endif
