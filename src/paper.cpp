
// the main program
main()
{
   initialize all the nodes.

   string request = "{type=lookup}"   +  // the request is for lookup
			        "{targetKey=18}"  +  // the key to be searched
			        "{flag=2}"        +  // use reinforcement algorithm
			        "{previousNodeIp=127.0.0.10}" + // the previous node, i.e. the node sending this request
			        "{initializingNodeIp=}";        // the node initialize the lookup

   initializingNode.client->rpc_request( helperNode, initializingNodeIp, request );
}


// call back when the node server receives requests
void configureNodeServer(const char *serverIP, node &n)
{
    // Run until cancelled
	while (true)
	{
         // Receive a message from the client
         clientlen = sizeof(echoclient);

         //  handle RPC lookup request here
         if (received > 0 && getField(dataReceived, "type") == lookup )
         {
        	 receiveLookup(buffer);
         }

         if (received > 0 && getField(dataReceived, "type") == respond )
         {
        	 receiveResponse(buffer);
         }

    }
}

void receiveLookup(buffer)
{
	string strRequest = string(buffer);
	int targetKey = getField(strRequest, "targetKey");
	int flag      = getField(strRequest, "flag");
	int targetIp  = getField(strRequest, "targetIp");

	int fingerIndex = findNext(n, targetKey, flag);

	if (fingerIndex == -1)
	{
	   print("Found: The node of the key is: " + targetIp );
	}else
	{
		string nextIp = n.fingerTable[fingerIndex].networkIp;

		string request = "{type=lookup}" +
					{targetKey=" + to_string(targetKey) + "}" +
					  {flag="+to_string(flag) + "}" +
					{callingNodeIp=" + n.networkIp + "}" +
					{startNodeIp=" + startNodeIp + "}";

		n.client->rpc_request( nextNetworkIp, n.networkIp, request );
	}
}

void receiveResponse(buffer)
{
	string strRequest = string(buffer);
	string targetNetwortIp = getField(strRequest, "targetNetwortIp");

	print("Result: The node found: " + targetNetwortIp );
}

