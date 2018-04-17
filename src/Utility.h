#ifndef UTILITY_H
#define UTILITY_H

#include <string>
#include <vector>
#include <stdlib.h>
#include <math.h>
#include <fstream>
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>


using namespace std;

class Utility
{
public:

        string stringToPackage(package result)
        {
        //    string ipStr = result.ip;
            string propagationStr = to_string(result.propagation); // std::string varAsString = std::to_string(myDoubleVar);
            string totalTimeStr = to_string(result.totalTime);
            // string will be formatted in "[ip]<propagation>{totalTime}" for RPC transfer.
            string packageStr = "["+result.ip + "]" + "<" + propagationStr + ">" + "{" + totalTimeStr + "}" ;
            return packageStr;
        }

};


#endif
