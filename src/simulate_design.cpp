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

using namespace std;
const int ORDER=14;             //the size of chord ring will be 2^order
const int NUMBER=400;	        //the number of nodes inside chord ring
const int REPEAT=10000;         //the total number of lookups performed by all the nodes inside chord ring

struct finger
{
	int key;			   //finger key
	string ip;                         //finger ip
	double latency;                    //latency from node to finger
	double qvalue;                     //q value of the action Q(node, finger)
};

struct node
{
	int key;                      	    //node key
	string ip;                          //node ip
	vector<finger> fingerTable;         //succinct list(the node index has been stored)
};

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

int findClosestInclude(int arr[], int size, int key)  //find the closest available node on the chord ring including itself
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

// find the max Q value from the node's finger table.
double findMaxQ( node& n )
{
	double max = INT_MIN;

	for(int i = 0; i < n.fingerTable.size(); ++i)
	{
		if(n.fingerTable[i].qvalue >= max)
			max = n.fingerTable[i].qvalue;
	}

	return max;
}

// update the node's Q value of the finger table
void updateQ(node& n, int action, double propagateQ)
{
	double lr = 0.9;			//learning rate
	double lambda = 0.9;		//decaying rate
	n.fingerTable[action].qvalue += lr * ( -1.0 * n.fingerTable[action].latency + lambda * propagateQ - n.fingerTable[action].qvalue );
}

//find the whole routing of lookup, output the total latency
//flag==0: greedy approach; flag==1: server selection; flag==2: reinforcment learning; flag==3: reinforcement learning without updation
struct package
{
	string ip;
	double totalTime;
	double propagation;
};

package route(network& net, node& n, int targetKey, int flag)
{
	//node next;
	package result; // the package used to hold node's ip, propagation time, and total time.

	int fingerIndex = findNext( n, targetKey, flag ); // find the finger index of the next node.

	// if the finger index is not a node, return the result to the calling function
	if( fingerIndex == -1 )
	{
		result.ip = n.ip;
		result.totalTime = 0;
		result.propagation = findMaxQ( n );
		return result;
	// if the finger index is a node
	}else{

		string nextIp = n.fingerTable[fingerIndex].ip; // get the ip coresponding to the finger index.

		result.propagation = findMaxQ( n ); // calculate the propagated max Q value.

		//this is the only place need to refer to net. For the version with RPC, this should be done on another node
		package receive = route( net, net.request[nextIp], targetKey, flag );

		// if it is reinforcment learning (flag==2)
		if(flag == 2)
			updateQ( n, fingerIndex, receive.propagation ); // update the node's Q value

		result.ip = receive.ip; // get the called node's ip.
		result.totalTime = receive.totalTime+n.fingerTable[fingerIndex].latency; // accumulate the total time.

		return result;
	}
}

server.("lookup")
{
	targetKey = get("targetKey");
	flag = get("flag");

	int fingerIndex=findNext(n, targetKey, flag);

	if(fingerIndex==-1)
	{

		result.ip = n.ip;
		result.totalTime = 0;
		result.propagation = findMaxQ( n );

		string cmd1 = {"respond", result.ip, result.totalTime, result.propagation, initilizingNodeIp}
		node<local>.rpc_send(nodeIpInitializing, cmd1);

	}else{

		string nextIp = n.fingerTable[fingerIndex].ip;


		string cmd1 = {"lookup", targetKey, flag, result.totalTime, result.propagation, initilizingNodeIp}
		node<local>.rpc_send(nextIp, cmd1);
	}
}

server.("respond")
{
		if(node_this.networkIp != initilizingNodeIp)
		{
			string cmd1 = {"response", result.ip, result.totalTime, result.propagation}
			print(cmd1);
		}else{

			if(flag==2)
				updateQ(n, fingerIndex, receive.propagation);

			//result.propagation = findMaxQ(n);
			result.totalTime = receive.totalTime + n.fingerTable[fingerIndex].latency;

			string cmd1 = {"respond", result.ip, result.totalTime, result.propagation, initilizingNodeIp}
			node<local>.rpc_send(nodeIpInitializing, cmd1);
		}
}

/*
package node<n>.route(networkIp, string cmd1)
{
	//node next;
	//package result;

	int fingerIndex=findNext(n, targetKey, flag);

	if(fingerIndex==-1)
	{
		result.ip=n.ip;
		result.totalTime=0;
		result.propagation=findMaxQ(n);
		return result;
	}

	// string nextIp=n.fingerTable[fingerIndex].ip;
	//result.propagation=findMaxQ(n);

	//being recursive section:
	//cout<<net.request[nextIp].fingerTable[0].latency<<'\n';	

    //this is the only place need to refer to net. For the version with RPC, this should be done on another node
    string cmd1 = {targetKey, flag, result.totalTime, result.propagation}
	node<n>.route(n.fingerTable[fingerIndex].networkIp, cmd1);	

	//end recursive section
	
	//if(flag==2)
	//	updateQ(n, fingerIndex, receive.propagation);

	//result.ip=receive.ip;
	//result.totalTime=receive.totalTime+n.fingerTable[fingerIndex].latency;


	//return result;
}

node<n>.rpc_request(serverIp, cmd1)
{
	node<n>.send(serverIp, localIp, cmd1);
}

node<n>.rpc_respond(serverIp, cmd1)
{
	node<n>.send(serverIp, localIp, cmd1);
}

server.("lookup")
{
	int fingerIndex=findNext(n, targetKey, flag);

	if(fingerIndex==-1)
	{
		result.ip=n.ip;
		result.totalTime=0;
		result.propagation=findMaxQ(n);

		string cmd1 = {"respond", result.ip, result.totalTime, result.propagation}
		node<n>.rpc_respond(serverIp, cmd1);
		//return result;
	}else{
		string nextIp=n.fingerTable[fingerIndex].ip;
		result.propagation=findMaxQ(n);
		result.totalTime=receive.totalTime+n.fingerTable[fingerIndex].latency;
		string cmd1 = {"lookup", targetKey, flag, result.totalTime, result.propagation}
		node<n>.rpc_request(serverIp, cmd1);
	}
}

server.("rpc_respond")
{
		string cmd1 = {"response", result.ip, result.totalTime, result.propagation}
		node<n>.rpc_respond(serverIp, cmd1);
}
*/

int main()
{
	//randomly generate a nodelist which maps on the index of chord ring
	srand(time(NULL));
	network net;
	int ringSize=1<<(ORDER);
	//cout<<ringSize;
	map<int,int> recorder;
	int counter=0;
	int nodeKey[NUMBER];
	while(counter<NUMBER)
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
	net.size=NUMBER;
	for(int i=0; i<NUMBER; ++i)
	{
		node temp;
		temp.key=nodeKey[i];
		//generate ip based on the value of index
		stringstream ss;
		ss<<i;
		ss>>temp.ip;

		temp.fingerTable=findFinger(nodeKey, i, distMatrix);

		net.request[temp.ip]=temp;
	}

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
	double average=0, av;
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

		}while(id%2==0);
		stringstream ss;
		ss<<id;
		string randomIP;
		ss>>randomIP;
		//randStart=net[randomIP];

		int randEnd=rand()%ringSize;
		package ret=route(net, net.request[randomIP], randEnd, 2);
		int latency=ret.totalTime;
		//cout<<'\n';
		int ik=i%500;
		if(i%500==0)
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
				stringstream ss2;
				ss2<<id;
				ss2>>randomIP;
				ret=route(net, net.request[randomIP], randEnd, 3);
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


