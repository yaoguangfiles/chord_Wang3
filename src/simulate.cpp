/*
This is the simulator for the Chord peer-to-peer lookup algorithm using reinforcement learning, only the basic functions related to the research are provided.

*/
#include<cmath>
#include<vector>
#include<iostream>
#include<map>
#include<cstdlib>
#include<time.h>
#include<algorithm>
#include<limits.h>
#include<string>
#include<sstream>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include "RPCClient.h"

using namespace std;

#define PORT_NUM 12345 // the port the RPC server listens
#define lookup   "lookup" // the RPC request command for requesting a service for find the node
#define response   "response" // the RPC response command for indicating the request is a response of a request

const int ORDER=14;             //the size of chord ring will be 2^order
const int NUMBER=3; // 400;	        //the number of nodes inside chord ring
const int REPEAT=10000;         //the total number of lookups performed by all the nodes inside chord ring



//find the whole routing of lookup, output the total latency
//flag==0: greedy approach; flag==1: server selection; flag==2: reinforcment learning; flag==3: reinforcement learning without updation
struct package
{
    string ip;
    double totalTime;
    double propagation;
};

struct finger
{
	int key;			   			   //finger key
	string ip;                         //finger ip
	string networkIp;                  // the networkIP of the node
	double latency;                    //latency from node to finger
	double qvalue;                     //q value of the action Q(node, finger)
};

struct node
{
	int key;                      	    //node key
	string ip;                          //node ip
	string networkIp;                    // the networkIP of the node
	vector<finger> fingerTable;         //succinct list(the node index has been stored)
	RPCClient *client;                   // the RPC client to send request to other node
};

void configureServer(const char *serverIP, node &n);
string getSubStr(const string &str, const string &subStr1, const string &subStr2);
char *getSubStr(char str[], char char1, char char2);
package stringToPackage(const string &resultStr);
string counterIpToNetworkIp(const string &counterIp);

struct network
{
	map<string, node> request;
	int size;
};

int findClosest(int arr[], int size, int key)       //find the closest available node on the chord ring
{
	for(int i=0; i<size; ++i)
	{
		if(arr[i]>key)
			return i;
	}
	return 0;
}

//find the closest available node on the chord ring including itself
int findClosestInclude(int arr[], int size, int key)
{
	for(int i=0; i<size; ++i)
	{
		if(arr[i]>=key)
			return i;
	}

	return 0;
}



vector<vector<double> > buildDist(int size)                                 //build the distance matrix
{
	vector<vector<double> > result;
	for(int i=0; i<size; ++i)
	{
		vector<double> temp;
		for(int j=0; j<size; ++j)
		{
			if(rand()%10>5)
				temp.push_back(0.00);
			else
				temp.push_back(rand()%20+1);
		}
		result.push_back(temp);
	}
	return result;
}

int distance(int a, int b, int size)										//find the distance between two nodes on the chord ring
{
	if(a<=b)
		return b-a;
	return b-a+size;
}



vector<finger> findFinger(int arr[], int ind, vector<vector<double> > dist)        //find the nodes in finger table
{
	vector<int> indexList;
	vector<finger> result;
	int ringSize=1<<(ORDER);
	int key=arr[ind];

	for(int i=0; i<ORDER; ++i)
	{
		int tempIndex=(key+(1<<i))%ringSize;
		//cout<<tempIndex<<'\n';

		indexList.push_back(findClosestInclude(arr, NUMBER, tempIndex));
	}

	for(int i=0; i<indexList.size(); ++i)
	{
		finger temp;
		temp.key=arr[indexList[i]];
		stringstream ss;
		ss<<indexList[i];
		ss>>temp.ip;
		temp.networkIp = counterIpToNetworkIp(temp.ip);
		temp.latency=dist[ind][indexList[i]];
		temp.qvalue=(double)rand()/INT_MAX;
		result.push_back(temp);
	}

	return result;
}


//find the next node for a hop. the candidate is selected from succinctList and finger table by different approaches. The return value is the index of the next node in finger table.
int findNext(node& n, int targetKey, int flag)
{
	int size=1<<(ORDER);
	if(distance(n.key, targetKey, size)<distance(n.fingerTable[0].key, targetKey, size))
		return -1;

	int result;
	double min;

	//0: greedy approach: find the longest finger
	if(flag==0)
	{	
		min=INT_MAX;
		for(int i=0; i<n.fingerTable.size(); ++i)
		{
			if(distance(n.fingerTable[i].key, targetKey, size)<min)
			{
				result=i;
				min=distance(n.fingerTable[i].key, targetKey, size);
			}
		}
	}
	//1: server selection
	else if(flag==1)
	{
		//double N=(double)distance(n.key, n.succinct.back(), size)/size*numberSuccinct;
		double N=NUMBER;
		min=INT_MAX;
		int cutoff=0;
		for(int i=0; i<n.fingerTable.size(); ++i)
		{
			if(distance(n.key, targetKey, size)<distance(n.fingerTable[i].key, targetKey, size))
				break;
			++cutoff;
		}
		//cout<<cutoff<<'\n';
		for(int i=0; i<cutoff; ++i)
		{
			double di=n.fingerTable[i].latency;
			double dmean=10.5/2;
			double estimate=di+dmean*log2((double)distance(n.fingerTable[i].key, targetKey, size)/size*N);
			if(estimate<min)
			{
				result=i;
				min=estimate;
			}
		}
	}
	//2: reinforcement learning
	else if(flag==2 || flag==3)
	{

		double noize=0.01;		//exploration rate
		int cutoff=0;
		//deterimne feasiable actions
		for(int i=0; i<n.fingerTable.size(); ++i)
		{
			if(distance(n.key, targetKey, size)<distance(n.fingerTable[i].key, targetKey, size))
				break;
			++cutoff;
		}
		//selection
		double max=INT_MIN;
		if((double)rand()/INT_MAX>noize)
		{
			for(int i=0; i<cutoff; ++i)
			{
				double tempq=n.fingerTable[i].qvalue+(double)rand()/INT_MAX;
				if(tempq>=max)
				{
					result=i;
					max=tempq;
				}
			}
			//}
		}
		else
			result=rand()%cutoff;	
	}
	return result;
}

double findMaxQ(node& n)
{
	double max=INT_MIN;
	for(int i=0; i<n.fingerTable.size(); ++i)
	{
		if(n.fingerTable[i].qvalue>=max)
			max=n.fingerTable[i].qvalue;
	}
	return max;
}
void updateQ(node& n, int action, double propagateQ)
{
	double lr=0.9;			//learning rate 
	double lambda=0.9;		//decaying rate
	n.fingerTable[action].qvalue+=lr*(-1.0*n.fingerTable[action].latency+lambda*propagateQ-n.fingerTable[action].qvalue);
}



/*
package route(network& net, node& n, int targetKey, int flag)
{
	//node next;
	package result;

	int fingerIndex=findNext(n, targetKey, flag);
	if(fingerIndex==-1)
	{
		result.ip=n.ip;
		result.totalTime=0;
		result.propagation=findMaxQ(n);
		return result;
	}
	string nextIp=n.fingerTable[fingerIndex].ip;
	result.propagation=findMaxQ(n);

	//being recursive section:
	//cout<<net.request[nextIp].fingerTable[0].latency<<'\n';	
	package receive=route(net, net.request[nextIp], targetKey, flag);	//this is the only place need to refer to net. For the version with RPC, this should be done on another node

	//end recursive section
	
	if(flag==2)
		updateQ(n, fingerIndex, receive.propagation);

	result.ip=receive.ip;
	result.totalTime=receive.totalTime+n.fingerTable[fingerIndex].latency;


	return result;
}
*/

/*
// find the node containing targetKey using algorithm indicated by flag in
// Chord peer-to-peer network.
// targetKey: the key to be found.
// flag: a flag indicates what algorithm to be used for the search.
string route_rpc( network& net, node& n, int targetKey, int flag )
{
    //node next;
    package result;

    int fingerIndex = findNext( n, targetKey, flag );

    if( fingerIndex == -1 )
    {
        result.ip = n.ip;
        result.totalTime = 0;
        result.propagation = findMaxQ( n );
        return packageToString( result );
    }
    string nextIp = n.fingerTable[fingerIndex].ip;
    result.propagation = findMaxQ( n );

    //being recursive section:
    //cout<<net.request[nextIp].fingerTable[0].latency<<'\n';

    // this is the only place need to refer to net. For the version with RPC,
    // this should be done on another node
    package receive = route_rpc( net, net.request[nextIp], targetKey, flag );

    //end recursive section

    if( flag == 2 )
        updateQ( n, fingerIndex, receive.propagation );

    result.ip = receive.ip;
    result.totalTime = receive.totalTime + n.fingerTable[fingerIndex].latency;

//    return result;
    return packageToString( result );
}
*/

// string will be formatted in "[ip]<propagation>{totalTime}" for RPC transfer.
string packageToString(package result)
{
//    string ipStr = result.ip;
    string propagationStr = to_string(result.propagation); // std::string varAsString = std::to_string(myDoubleVar);
    string totalTimeStr = to_string(result.totalTime);
    // string will be formatted in "[ip]<propagation>{totalTime}" for RPC transfer.
    string packageStr = "["+result.ip + "]" + "<" + propagationStr + ">" + "{" + totalTimeStr + "}" ;
    return packageStr;
}

package stringToPackage(const string &resultStr)
{
    string ip = getSubStr(resultStr, "[", "]");
    string propagationStr = getSubStr(resultStr, "<", ">");
    string totalTimeStr = getSubStr(resultStr, "{", "}");

    package result;

    result.ip          = ip;
    result.propagation = std::stod(propagationStr);
    result.totalTime   = std::stod(totalTimeStr);

    return result;
}

string counterIpToNetworkIp(const string &counterIp)
{
	string ip_prefix = "127.0.0.";
//	string ip_subfix = to_string( counterIp + 1);
	string ip_subfix = to_string( std::stoi( counterIp ) + 1 );

	string networkIp = ip_prefix + ip_subfix;

    return networkIp;
}

// get a substring between subStr1 and subStr2 in str.
string getSubStr(const string &str, const string &subStr1, const string &subStr2)
{
    unsigned first = str.find(subStr1);
    unsigned first_position = first + subStr1.length();
    unsigned last = str.find(subStr2);

    return str.substr(first_position, last - first_position);
}

// get a substring between subStr1 and subStr2 in str.
char *getSubStr(char str[], char char1, char char2)
{
	int first = 0;
//    int first_position = first + subStr1.length();
	int last = 0;

	for(int i = 0; i < strlen(str); i++)
	{
		if(str[i] == char1)
		{
			first = i;
		}else if(str[i] == char2){
			last = i;
		}
	}

//	cout << "first: " << first << ", last: " << last << endl;

	char *substr = new char[last-first+1];

	for(int i = 0; i < last-first - 1; i++)
	{
		substr[i] = str[first+1+i];
	}

	substr[last - first - 1] = '\n';

	cout << "substr: " << substr << endl;

	return substr;
}


int main()
{
	//randomly generate a nodelist which maps on the index of chord ring
	srand(time(NULL));
	network net;

	int ringSize = 1 << (ORDER);
	//cout<<ringSize;
	map<int,int> recorder;

	int counter = 0;
	int nodeKey[NUMBER];

	while( counter < NUMBER )
	{
		int tempKey=rand()%ringSize;
		if(recorder.find(tempKey)==recorder.end())
		{
			recorder[tempKey]=1;
			nodeKey[counter]=tempKey;
			++counter;
		}
	}

	sort(nodeKey, nodeKey+NUMBER);

	//bulid distance matrx;
	vector<vector<double> > distMatrix=buildDist(NUMBER);

	//build network
	net.size = NUMBER;
	for( int i = 0; i < NUMBER; ++i )
	{
		node temp;
		temp.key = nodeKey[i];

		//generate ip based on the value of index
		stringstream ss;
		ss << i;
		ss >> temp.ip;

		temp.networkIp = counterIpToNetworkIp(temp.ip);

//		string ip_prefix = "127.0.0.";
//		string ip_subfix = to_string( i + 1);
//		temp.ip = ip_prefix + ip_subfix;



		temp.fingerTable = findFinger( nodeKey, i, distMatrix );

		net.request[temp.ip] = temp;

		temp.client = new RPCClient(); // configure RPC client
//		temp.client->sendMessage("127.0.0.1", "12345", "127.0.0.2", "test:yao_zhong"); // send a message

		// +++++++++++++++++++++++++++++++++++++++
		// +++++++++++++++++++++++
		// +++++++++++++++++++++++++++++++++++++++++

//		string networkIp = counterIpToNetworkIp(temp.ip);
		cout << "(Info): starting node server (ip: " << temp.networkIp << ")." << endl;

		pid_t pid;

		// fork will make 2 processes
		pid = fork();

		if (pid == -1)
		{
			perror("fork");
			exit(EXIT_FAILURE);
		}
		// if it is the child process
		if (pid == 0)
		{
			// start a node server for each node
			configureServer(temp.networkIp.c_str(), temp); // configure RPC server
		}

//		int period = 2;
//		for(int i = 1; i <= period; i++)
//		{
//			sleep(1);
//			cout << "(Info): Sleep counting: " << i << " of " << period << endl;
//		}

	}

	for( int i = 0; i < NUMBER; ++i )
	{
		string counterIp = to_string(i);
		cout << "(Info): node(counterIp: " << net.request[counterIp].ip << "), networkIp: "
			 << net.request[counterIp].networkIp << ", key: " << net.request[counterIp].key << endl;
	}

	int TIME = 5;
//	cout << "(Info): Sleep for "<<TIME<<" seconds for initializing all the node servers..." << endl;
	for(int i = 1; i <= TIME; i++)
	{
		sleep(1);
		cout << "(Info): Sleep counting: " << i << " of " << TIME << endl;
	}
//	cout << "(Info): Wake up after sleeping for " <<TIME<< endl;

	/*test build distance matrix
	for(int i=0; i<distMatrix.size(); ++i)
	{
		for(int j=0; j<distMatrix[0].size(); ++j)
		{
			cout<<distMatrix[i][j]<<' ';
		}
		cout<<'\n';
	}*/
	//test route
	//cout<<route(distMatrix, nodeList, number, order, nodeIndex[0], 0, 0);

	//Experiment
	double average = 0, av;
	//int randStart=nodeIndex[0];//nodeIndex[rand()%number];
	//int randEnd=100;//rand()%ringSize;

	//training group
	cout<<"moving average of latency for training group:\n";
	vector<double> record;

	for(int i=0; i<REPEAT; ++i)
	{
		//node randStart;
		int id;

		do
		{
			id=rand()%NUMBER;

		}while( id%2 == 0 );

		stringstream ss;
		ss << id;

		string randomIP;
		ss >> randomIP;
		//randStart=net[randomIP];

		string randomNetworkI = counterIpToNetworkIp(randomIP);

//		randomIP = "127.0.0.2";

		string localIP = "127.0.0.2";
//		string serverIP = "127.0.0.3";

		int randEnd = rand()%ringSize;

		// +++++++++++++++++++++
		// ++++++++++++++++++++ change to rpc request
//		package ret=route(net, net.request[randomIP], randEnd, 2);

		// pass the lookup command to the node, "lookup(targetkey){flag}"
		string request_cmd1 = "lookup("+to_string(randEnd)+"){2}";

		// have the node request the target key
		// pass the lookup command to the node, "lookup(targetkey){flag}"
//		string receivedStr = net.request[randomIP].client->rpc_request( net.request[randomIP].ip
//		        , net.request[randomIP].ip, request_cmd1 );
		string receivedStr = net.request[localIP].client->rpc_request( randomNetworkI, localIP, request_cmd1 );

		package ret = stringToPackage(receivedStr);

		int latency=ret.totalTime;
		//cout<<'\n';

		int ik = i % 500;

		if( i % 500 == 0 )
		{
			cout<<average<<'\n';
			average=0;
			double rec=0;
			for(int s=0; s<500; ++s)
			{
				//test group evaluation
				do
				{
					id=rand()%NUMBER;
					//randStart=nodeIndex[rand()%number];
				}while(id%2==1);
				randEnd=rand()%ringSize;

//				stringstream ss2;
//				ss2<<id;
//				ss2>>randomIP;

//				randomIP = counterIpToNetworkIp(id);

//				randomIP = "127.0.0.2";

				// +++++++++++++++++++++
                // ++++++++++++++++++++ change to rpc request
//				ret=route(net, net.request[randomIP], randEnd, 3);

				// pass the lookup command to the node, "lookup(targetkey){flag}"
				string request_cmd2 = "lookup(" + to_string(randEnd) + "){3}";


				// have the node request the target key
//                string receivedStr = net.request[randomIP].client->rpc_request( net.request[randomIP].ip
//                        , net.request[randomIP].ip, request_cmd2 );

				string receivedStr = net.request[localIP].client->rpc_request( randomNetworkI, localIP, request_cmd2 );

                package ret = stringToPackage(receivedStr);

				latency=ret.totalTime;
				
				rec+=latency;				
			}
			record.push_back(rec/500.0);
			
		}
		average=average*(ik/(ik+1.0))+latency/(ik+1.0);

	}
	cout<<average<<'\n';
	cout<<'\n'<<'\n';
	cout<<"latency for test group: \n";
	for(int i=0; i<record.size(); ++i)
	{
		cout<<record[i]<<'\n';
	}
}


/**
 * Configure server for the socket for UDP. The server will wait for
 * clients for connection. It will wait for messages sent from clients.
 * Upon receiving messages from clients, the server will respond to them.
 * @para portNum The port number through which the message will be passed.
 * @return No return value.
 */
void configureServer(const char *serverIP, node &n)
//void configureServer()
{
//    const char *serverIP = this->ip.c_str();

    char buffer[256];
    unsigned int clientlen, serverlen;
    int received = 0;

    int sock;
    struct sockaddr_in echoserver;  // structure for address of server
    struct sockaddr_in echoclient;  // structure for address of client

    // Create the UDP socket
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    // Construct the server sockaddr_in structure
    memset(&echoserver, 0, sizeof(echoserver)); /* Clear struct */
    echoserver.sin_family = AF_INET; /* Internet/IP */
//    echoserver.sin_addr.s_addr = INADDR_ANY; /* Any IP address */

    echoserver.sin_addr.s_addr = inet_addr( serverIP );   /* the server IP address */
    echoserver.sin_port = htons(PORT_NUM); /* server port */

    // Bind the socket
    serverlen = sizeof(echoserver);
    if (bind(sock, (struct sockaddr *) &echoserver, serverlen) < 0)
    {
        perror("Failed to bind server socket");
        exit(EXIT_FAILURE);
    }

    cout << "(Info): Server (ip: " << serverIP << ") is running. Waiting for message..." << endl;

    // Run until cancelled
    while (true)
    {
        // === begin: Receive message from clients, test, and close the connection ===
        // Receive a message from the client
        clientlen = sizeof(echoclient);

        if ((received = recvfrom(sock, buffer, 256, 0,
                (struct sockaddr *) &echoclient, &clientlen)) < 0)
        {
            perror("Failed to receive message");
            exit(EXIT_FAILURE);
        }

//        cerr << "Client connected: " << inet_ntoa(echoclient.sin_addr) << "\n";

        cout << "(Info): The Server (ip: " << serverIP << ") received data: |"<< buffer <<"|." << endl;

        if(received > 0 && strncmp(buffer,"closed", 6) == 0 )
        {
            break;
        }

        if(received > 0 && strncmp(buffer,"test:", 5) == 0 )
        {
            sendto(sock, buffer, received, 0, (struct sockaddr *) &echoclient, clientlen);
            cout << "(Info): The Server (ip: " << serverIP << ") sent back a message: |" << buffer << "|." << endl;
        }
        // === end: Receive message from clients, test, and close the connection ===

        // === begin: handle RPC lookup request here ===

        // if this is a request for a lookup
        // the passed request command will be in the form of "lookup(targetkey){flag}"
        if(received > 0 && strncmp(buffer,lookup, 6) == 0 )
        {
            cout << "(Info): Server (ip: " << n.networkIp
                    << ") received message 'lookup' command: |" << buffer << "|." << endl;

        	// the passed request command will be in the form of "lookup(targetkey){flag}"

        	// extract the key from the received buffer.
            int targetKey = std::stoi( getSubStr(buffer, '(', ')') );

            // extract from the received buffer.
            int flag      = std::stoi( getSubStr(buffer, '{', '}') );

            string resultStr = ""; // the result produced by this node.

            // ------------------- loop up the node ---------------------

			// string route_rpc( network& net, node& n, int targetKey, int flag )
			// {
			package result;

			int fingerIndex = findNext( n, targetKey, flag );

			if( fingerIndex == -1 )
			{
				result.ip = n.ip;
				result.totalTime = 0;
				result.propagation = findMaxQ( n );

				resultStr = packageToString( result );
			}

			string nextIp = n.fingerTable[fingerIndex].ip;
			result.propagation = findMaxQ( n );

			//being recursive section:
			//cout<<net.request[nextIp].fingerTable[0].latency<<'\n';

			// this is the only place need to refer to net. For the version with RPC,
			// this should be done on another node
			// package receive = route_rpc( net, net.request[nextIp], targetKey, flag );
			// +++++++++++++++++++++++++++++++++++++++++
			// ++ RPC request the service from the node with next ip
			// +++++++++++++++++++++++++++++++++++++++++

			string request_cmd1 = "lookup("+to_string(targetKey)+"){"+to_string(flag)+"}";

			// node_nextIP.request(targetKey, flag);
//			string receivedStr = n.client->rpc_request( n.fingerTable[0].networkIp, n.networkIp, request_cmd1 );
//			string receivedStr = n.client->rpc_request( n.fingerTable[0].networkIp, n.networkIp, request_cmd1,  fingerIndex, n);
			n.client->rpc_request( n.fingerTable[0].networkIp, n.networkIp, request_cmd1 );
//			n.client->rpc_request( n.fingerTable[0].networkIp, n.networkIp, request_cmd1,  fingerIndex, n);

			// ++++++++++++++++++++++++++++++
			// +++++++ The below part will be done in the response session.
			// ++++++++++++++++++++++++++++++
			cout << "(Info): The node server (ip: " << n.networkIp << ") received package string :|"
				  << receivedStr << "| from node server (ip: " << n.networkIp << ")." << endl;

			package receivedPkg = stringToPackage(receivedStr);
			//end recursive section

			if( flag == 2 )
				updateQ( n, fingerIndex, receivedPkg.propagation );

			result.ip = receivedPkg.ip;
			result.totalTime = receivedPkg.totalTime + n.fingerTable[fingerIndex].latency;

			// return result;
			resultStr = packageToString( result );

			// }

            // -------------------- end: loop up the node -----------------------

			// === begin: handle RPC response here ===

			if(received > 0 && strncmp(buffer,response, 8) == 0 )
			{
				// send info to the local node server to pass the information back to itself for the calculation below.
				// It will respond back the the previous node server at the end of the response.

				cout << "(Info): The node server (ip: " << n.networkIp << ") received package string :|"
					  << receivedStr << "| from node server (ip: " << n.networkIp << ")." << endl;

				package receivedPkg = stringToPackage(receivedStr);
				//end recursive section

				if( flag == 2 )
					updateQ( n, fingerIndex, receivedPkg.propagation );

				result.ip = receivedPkg.ip;
				result.totalTime = receivedPkg.totalTime + n.fingerTable[fingerIndex].latency;

				// return result;
				resultStr = packageToString( result );

				// ++++++++++++++
				// if the ip is the ip request, end and print the result,
				// otherwise, send the result string back to the previous node ip.

			}


//            char nodeIdChars[50];
//
//            sprintf(nodeIdChars, "FoundId:<%d>[%s]", n.key, n.ip.c_str());

            sendto(sock, resultStr.c_str(), resultStr.length() + 1, 0, (struct sockaddr *) &echoclient, clientlen);
        }

        // ------------------- handle test message ---------------------

        if(received > 0 && strncmp(buffer,"test:", 5) == 0 )
		{
			sendto(sock, buffer, received, 0, (struct sockaddr *) &echoclient, clientlen);
			cout << "(Info): The Server (ip: " << serverIP << ") sent back a message: |" << buffer << "|." << endl;
		}

        // ------------------- end: handle test message ---------------------

    }

    close(sock);
    cout << "(Info): Closed the Server (ip: " << serverIP << ").." << endl;


//    close(sock);
}



