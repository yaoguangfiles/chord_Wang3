/*
 This is the simulator for the Chord peer-to-peer lookup algorithm using reinforcement learning, only the basic functions related to the research are provided.

 */
#include <cmath>
#include <vector>
#include <iostream>
#include <map>
#include <cstdlib>
#include <time.h>
#include <algorithm>
#include <limits.h>
#include <string>
#include <sstream>

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
#define respond   "respond" // the RPC response command for indicating the request is a response of a request

const int ORDER = 14;             //the size of chord ring will be 2^order
const int NUMBER = 12; // 400;	        //the number of nodes inside chord ring
const int REPEAT = 10000; //the total number of lookups performed by all the nodes inside chord ring

//find the whole routing of lookup, output the total latency
//flag==0: greedy approach; flag==1: server selection; flag==2: reinforcment learning; flag==3: reinforcement learning without updation
struct package
{
	string ip;
	string networkIp;
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
	vector<finger> fingerTable;  //succinct list(the node index has been stored)
	RPCClient *client;           // the RPC client to send request to other node
};

void configureServer(const char *serverIP, node &n);
string getSubStr(const string &str, const string &subStr1,
		const string &subStr2);
string getField(const string &str, const string &field);
char *getSubStr(char str[], char char1, char char2);
package stringToPackage(const string &resultStr);
string counterIpToNetworkIp(const string &counterIp);

struct network
{
	map<string, node> request;
	int size;
};

int findClosest(int arr[], int size, int key) //find the closest available node on the chord ring
{
	for (int i = 0; i < size; ++i)
	{
		if (arr[i] > key)
			return i;
	}
	return 0;
}

//find the closest available node on the chord ring including itself
int findClosestInclude(int arr[], int size, int key)
{
	for (int i = 0; i < size; ++i)
	{
		if (arr[i] >= key)
			return i;
	}

	return 0;
}

vector<vector<double> > buildDist(int size)          //build the distance matrix
{
	vector<vector<double> > result;
	for (int i = 0; i < size; ++i)
	{
		vector<double> temp;
		for (int j = 0; j < size; ++j)
		{
			if (rand() % 10 > 5)
				temp.push_back(0.00);
			else
				temp.push_back(rand() % 20 + 1);
		}
		result.push_back(temp);
	}
	return result;
}

int distance(int a, int b, int size)//find the distance between two nodes on the chord ring
{
	if (a <= b)
		return b - a;
	return b - a + size;
}

vector<finger> findFinger(int arr[], int ind, vector<vector<double> > dist) //find the nodes in finger table
{
	vector<int> indexList;
	vector<finger> result;
	int ringSize = 1 << (ORDER);
	int key = arr[ind];

	for (int i = 0; i < ORDER; ++i)
	{
		int tempIndex = (key + (1 << i)) % ringSize;
		//cout<<tempIndex<<'\n';

		indexList.push_back(findClosestInclude(arr, NUMBER, tempIndex));
	}

	for (int i = 0; i < (int) indexList.size(); ++i)
	{
		finger temp;
		temp.key = arr[indexList[i]];
		stringstream ss;
		ss << indexList[i];
		ss >> temp.ip;
		temp.networkIp = counterIpToNetworkIp(temp.ip);
		temp.latency = dist[ind][indexList[i]];
		temp.qvalue = (double) rand() / INT_MAX;
		result.push_back(temp);
	}

	return result;
}

//find the next node for a hop. the candidate is selected from succinctList and finger table by different approaches. The return value is the index of the next node in finger table.
int findNext(node& n, int targetKey, int flag)
{
	int size = 1 << (ORDER);
	if (distance(n.key, targetKey, size)
			< distance(n.fingerTable[0].key, targetKey, size))
		return -1;

	int result;
	double min;

	//0: greedy approach: find the longest finger
	if (flag == 0)
	{
		min = INT_MAX;
		for (int i = 0; i < (int) n.fingerTable.size(); ++i)
		{
			if (distance(n.fingerTable[i].key, targetKey, size) < min)
			{
				result = i;
				min = distance(n.fingerTable[i].key, targetKey, size);
			}
		}
	}
	//1: server selection
	else if (flag == 1)
	{
		//double N=(double)distance(n.key, n.succinct.back(), size)/size*numberSuccinct;
		double N = NUMBER;
		min = INT_MAX;
		int cutoff = 0;
		for (int i = 0; i < (int) n.fingerTable.size(); ++i)
		{
			if (distance(n.key, targetKey, size)
					< distance(n.fingerTable[i].key, targetKey, size))
				break;
			++cutoff;
		}
		//cout<<cutoff<<'\n';
		for (int i = 0; i < cutoff; ++i)
		{
			double di = n.fingerTable[i].latency;
			double dmean = 10.5 / 2;
			double estimate = di
					+ dmean
							* log2(
									(double) distance(n.fingerTable[i].key,
											targetKey, size) / size * N);
			if (estimate < min)
			{
				result = i;
				min = estimate;
			}
		}
	}
	//2: reinforcement learning
	else if (flag == 2 || flag == 3)
	{

		double noize = 0.01;		//exploration rate
		int cutoff = 0;
		//deterimne feasiable actions
		for (int i = 0; i < (int) n.fingerTable.size(); ++i)
		{
			if (distance(n.key, targetKey, size)
					< distance(n.fingerTable[i].key, targetKey, size))
				break;
			++cutoff;
		}
		//selection
		double max = INT_MIN;
		if ((double) rand() / INT_MAX > noize)
		{
			for (int i = 0; i < cutoff; ++i)
			{
				double tempq = n.fingerTable[i].qvalue
						+ (double) rand() / INT_MAX;
				if (tempq >= max)
				{
					result = i;
					max = tempq;
				}
			}
			//}
		}
		else
			result = rand() % cutoff;
	}
	return result;
}

double findMaxQ(node& n)
{
	double max = INT_MIN;
	for (int i = 0; i < (int) n.fingerTable.size(); ++i)
	{
		if (n.fingerTable[i].qvalue >= max)
			max = n.fingerTable[i].qvalue;
	}
	return max;
}
void updateQ(node& n, int action, double propagateQ)
{
	double lr = 0.9;			//learning rate
	double lambda = 0.9;		//decaying rate
	n.fingerTable[action].qvalue += lr
			* (-1.0 * n.fingerTable[action].latency + lambda * propagateQ
					- n.fingerTable[action].qvalue);
}

// string will be formatted in "[ip]<propagation>{totalTime}" for RPC transfer.
string packageToString(package result)
{
//    string ipStr = result.ip;
	string propagationStr = to_string(result.propagation); // std::string varAsString = std::to_string(myDoubleVar);
	string totalTimeStr = to_string(result.totalTime);
	// string will be formatted in "[ip]<propagation>{totalTime}" for RPC transfer.
	string packageStr = "[" + result.ip + "]" + "<" + propagationStr + ">" + "{"
			+ totalTimeStr + "}";
	return packageStr;
}

package stringToPackage(const string &resultStr)
{
	string ip = getSubStr(resultStr, "[", "]");
	string propagationStr = getSubStr(resultStr, "<", ">");
	string totalTimeStr = getSubStr(resultStr, "{", "}");

	package result;

	result.ip = ip;
	result.propagation = std::stod(propagationStr);
	result.totalTime = std::stod(totalTimeStr);

	return result;
}

string counterIpToNetworkIp(const string &counterIp)
{
	string ip_prefix = "127.0.0.";
//	string ip_subfix = to_string( counterIp + 1);
	string ip_subfix = to_string(std::stoi(counterIp) + 1);

	string networkIp = ip_prefix + ip_subfix;

	return networkIp;
}

// get a substring between subStr1 and subStr2 in str.
string getField(const string &str, const string &field)
{
	return getSubStr(str, "{" + field + "=", "}");
}

// get a substring between subStr1 and subStr2 in str.
string getSubStr(const string &str, const string &subStr1, const string &subStr2)
{
	unsigned first = str.find(subStr1);
	unsigned first_position = first + subStr1.length();
	unsigned last = str.find(subStr2, first_position);

//	cout << "first:" << first << ", last:" << last << endl;

	if(first > str.length())
		return "";

	if(last < str.length())
		return str.substr(first_position, last - first_position);
	else
		return "";
}

// get a substring between subStr1 and subStr2 in str.
char *getSubStr(char str[], char char1, char char2)
{
	int first = 0;
//    int first_position = first + subStr1.length();
	int last = 0;

	for (int i = 0; i < (int) strlen(str); i++)
	{
		if (str[i] == char1)
		{
			first = i;
		}
		else if (str[i] == char2)
		{
			last = i;
		}
	}

//	cout << "first: " << first << ", last: " << last << endl;

	char *substr = new char[last - first + 1];

	for (int i = 0; i < last - first - 1; i++)
	{
		substr[i] = str[first + 1 + i];
	}

	substr[last - first - 1] = '\n';

//	cout << "substr: " << substr << endl;

	return substr;
}

int main()
{
	//randomly generate a nodelist which maps on the index of chord ring
	srand(time(NULL));
	network net;

	int ringSize = 1 << (ORDER);
	//cout<<ringSize;
	map<int, int> recorder;

	int counter = 0;
	int nodeKey[NUMBER];

	while (counter < NUMBER)
	{
		int tempKey = rand() % ringSize;
		if (recorder.find(tempKey) == recorder.end())
		{
			recorder[tempKey] = 1;
			nodeKey[counter] = tempKey;
			++counter;
		}
	}

	sort(nodeKey, nodeKey + NUMBER);

	//bulid distance matrx;
	vector<vector<double> > distMatrix = buildDist(NUMBER);

	//build network
	net.size = NUMBER;
	for (int i = 0; i < NUMBER; ++i)
	{
		node temp;
		temp.key = nodeKey[i];

		//generate ip based on the value of index
		stringstream ss;
		ss << i;
		ss >> temp.ip;

		temp.networkIp = counterIpToNetworkIp(temp.ip);

		temp.fingerTable = findFinger(nodeKey, i, distMatrix);

		net.request[temp.ip] = temp;

		temp.client = new RPCClient(); // configure RPC client

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
	}

	for (int i = 0; i < NUMBER; ++i)
	{
		string counterIp = to_string(i);
		cout << "(Info): node(counterIp: " << net.request[counterIp].ip
				<< "), networkIp: " << net.request[counterIp].networkIp
				<< ", key: " << net.request[counterIp].key << endl;
	}

	int TIME = 5;
	for (int i = 1; i <= TIME; i++)
	{
		sleep(1);
		cout << "(Info): Sleep counting: " << i << " of " << TIME << endl;
	}

	//Experiment
//	double average = 0, av;

	//training group
	cout << "moving average of latency for training group:\n";
	vector<double> record;

	int id;

//	do
//	{
//		id = rand() % NUMBER;
//
//	} while (id % 2 == 0);

	stringstream ss;
	ss << id;

	string randomIP;
	ss >> randomIP;
	//randStart=net[randomIP];
//	string randomNetworkI = counterIpToNetworkIp(randomIP);
	string randomNetworkI = "127.0.0.3";

//	string localIP = "127.0.0.1";
	//      string serverIP = "127.0.0.3";
	string localIP = "127.0.0.5";

	int randEnd = rand() % ringSize;

	// +++++++++++++++++++++
	// ++++++++++++++++++++ change to rpc request
	//      package ret=route(net, net.request[randomIP], randEnd, 2);

	// pass the lookup command to the node, "lookup(targetkey){flag}"
	string request_cmd1 = "{type=lookup}" +
			     (string) "{targetKey=" +to_string(randEnd) + "}" +
						  "{flag=" + "2" + "}"
				 	 	  "{callingNodeIp=" + localIP + "}" +
						  "{startNodeIp=" + localIP + "}";

	// have the node request the target key
	// pass the lookup command to the node, "lookup(targetkey){flag}"

	net.request[localIP].client->rpc_request(randomNetworkI, localIP, request_cmd1);

//	cout << "(Info): The node (ip: " << net.request[localIP].networkIp
//		 << ") is finding the node with message: |" << request_cmd1 << "|" << endl;

	// ------------- the end of main ------------
}

/**
 * Configure server for the socket for UDP. The server will wait for
 * clients for connection. It will wait for messages sent from clients.
 * Upon receiving messages from clients, the server will respond to them.
 * @para portNum The port number through which the message will be passed.
 * @return No return value.
 */
void configureServer(const char *serverIP, node &n)
{
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

	echoserver.sin_addr.s_addr = inet_addr(serverIP); /* the server IP address */
	echoserver.sin_port = htons(PORT_NUM); /* server port */

	// Bind the socket
	serverlen = sizeof(echoserver);
	if (bind(sock, (struct sockaddr *) &echoserver, serverlen) < 0)
	{
		perror("Failed to bind server socket");
		exit(EXIT_FAILURE);
	}

	cout << "(Info): Server (ip: " << n.networkIp
			<< ") is running. Waiting for message..." << endl;

	// Run until cancelled
	while (true)
	{
		// Receive a message from the client
		clientlen = sizeof(echoclient);

		if ((received = recvfrom(sock, buffer, 256, 0,
				(struct sockaddr *) &echoclient, &clientlen)) < 0)
		{
			perror("Failed to receive message");
			exit(EXIT_FAILURE);
		}

		string dataReceived = string(buffer);

		cout << "(Info): The Server (ip: " << serverIP << ") received data: |"
				<< dataReceived << "|." << endl;

		if (received > 0 && strncmp(buffer, "closed", 6) == 0)
		{
			break;
		}

		if (received > 0 && strncmp(buffer, "test:", 5) == 0)
		{
			sendto(sock, buffer, received, 0, (struct sockaddr *) &echoclient,
					clientlen);
			cout << "(Info): The Server (ip: " << serverIP
					<< ") sent back a message: |" << buffer << "|." << endl;
		}

		// =============================================
		// === begin: handle RPC lookup request here ===
		// =============================================

		// if this is a request for a lookup
		// the passed request command will be in the form of "lookup(targetkey){flag}"
//		if (received > 0 && strncmp(buffer, lookup, 6) == 0)
		if (received > 0 && getField(dataReceived, "type") == lookup )
		{
			string strRequest = string(buffer);

			int targetKey = std::stoi(getField(strRequest, "targetKey"));
			int flag      = std::stoi(getField(strRequest, "flag"));
//			int propagation      = std::stoi(getField(strRequest, "propagation"));
//			int totalTime      = std::stoi(getField(strRequest, "totalTime"));
//			int fingerIndex      = std::stoi(getField(strRequest, "fingerIndex"));
			string startNodeIp = getField(strRequest, "startNodeIp");
			string callingNodeIp = getField(strRequest, "callingNodeIp");

			package result;

			int fingerIndex = findNext(n, targetKey, flag);

//			cout << "(Info): fingerIndex: " << fingerIndex << endl;

			if (fingerIndex == -1)
			{
				result.ip = n.ip;
				result.networkIp = n.networkIp;
				result.totalTime = 0;
				result.propagation = findMaxQ(n);

//				string str1 = "{targetNetwortIp=";
				string request_cmd1 = "{type=respond}" +
							 (string) "{targetKey=" + to_string(targetKey) + "}" +
				                      "{flag="+to_string(flag) + "}" +
							          "{targetNetwortIp=" + result.networkIp + "}" +
									  "{totalTime=" + to_string(result.totalTime) + "}" +
									  "{propagation=" + to_string(result.propagation) + "}" +
									  "{fingerIndex=" + to_string(fingerIndex) + "}" +
									  "{callingNodeIp=" + n.networkIp + "}" +
									  "{startNodeIp=" + startNodeIp + "}";

				if(n.networkIp != startNodeIp)
					n.client->rpc_request(startNodeIp, n.networkIp, request_cmd1);

				else
				{
					string result =    "{targetNetwortIp=" + n.networkIp + "}";

					cout << "=======================================================" << endl;
					cout << "Result: The node found: " << result << endl;
					cout << "=======================================================" << endl;
				}
			}
			else
			{
//				if (flag == 2)
//					updateQ(n, fingerIndex, propagation);
//
//				double totalTimePassing = (double) totalTime + n.fingerTable[fingerIndex].latency;

				string nextNetworkIp = n.fingerTable[fingerIndex].networkIp;

				string request_cmd1 = "{type=lookup}" +
							 (string) "{targetKey=" + to_string(targetKey) + "}" +
						              "{flag="+to_string(flag) + "}" +
									  "{callingNodeIp=" + n.networkIp + "}" +
									  "{startNodeIp=" + startNodeIp + "}";

				n.client->rpc_request(nextNetworkIp, n.networkIp, request_cmd1);
			}

		} // end of "lookup" session

		// =============================================
		// === begin: handle RPC response here ===
		// =============================================

//		if (received > 0 && strncmp(buffer, respond, 7) == 0)
		if (received > 0 && getField(dataReceived, "type") == respond )
		{
			string strRequest = string(buffer);

//			cout << "(Info): {type=respond} The node server (ip: " << n.networkIp << ") received string :|" << strRequest << "|" << endl;

//			int targetKey = std::stoi(getField(strRequest, "targetKey"));
//			int flag      = std::stoi(getField(strRequest, "flag"));
//			int fingerIndex = std::stoi(getField(strRequest, "fingerIndex"));
//			int totalTime = std::stoi(getField(strRequest, "totalTime"));
//			int propagation = std::stoi(getField(strRequest, "propagation"));
//			string startNodeIp = getField(strRequest, "startNodeIp");
//			string callingNodeIp = getField(strRequest, "callingNodeIp");
			string targetNetwortIp = getField(strRequest, "targetNetwortIp");

			// if the receive node is the node start request a lookup,
			// stop responding and print the result.
//			if (n.networkIp == startNodeIp && targetKey == -1)
			{
//				string result =    "{targetNetwortIp=" + targetNetwortIp + "}" +
//								   "{totalTime=" + to_string(totalTime) + "}" +
//								   "{propagation=" + to_string(propagation) + "}";

//				string result =    "{targetNetwortIp=" + targetNetwortIp + "}";

				cout << "=======================================================" << endl;
				cout << "Result: The node found: " << targetNetwortIp << endl;
				cout << "=======================================================" << endl;
			}
			// if the receive node is the node start request a lookup,
			// respond to the previous node.
//			else
//			{
//
//				if (flag == 2)
//					updateQ(n, fingerIndex, propagation);
//
//				//result.propagation = findMaxQ(n);
//				double totalTimePassing = (double) totalTime + n.fingerTable[fingerIndex].latency;
//
//				string request_cmd1 = "{type=respond}" +
//							 (string) "{targetNetwortIp=" + targetNetwortIp + "}" +
//									  "{totalTime=" + to_string(totalTimePassing) + "}" +
//									  "{propagation=" + to_string(propagation) + "}" +
//									  "{fingerIndex=" + to_string(fingerIndex) + "}" +
//									  "{callingNodeIp=" + callingNodeIp + "}" +
//									  "{startNodeIp=" + startNodeIp + "}";
//
//				n.client->rpc_request(callingNodeIp, n.networkIp, request_cmd1);
//			}


		} // end of "respond" session

		// ------------------- handle test message ---------------------

		if (received > 0 && strncmp(buffer, "test:", 5) == 0)
		{
			sendto(sock, buffer, received, 0, (struct sockaddr *) &echoclient,
					clientlen);
			cout << "(Info): The Server (ip: " << serverIP
					<< ") sent back a message: |" << buffer << "|." << endl;
		}

		// ------------------- end: handle test message ---------------------

	}

	close(sock);
	cout << "(Info): Closed the Server (ip: " << serverIP << ").." << endl;
}

