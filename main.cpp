/*
 * ISA project 2020
 * Filtering DNS resolver
 * author: David Rubý (xrubyd00/213482)
 */

/*      RETURN CODES:
 *  0 -> everything OK
 *  1 -> arguments error
 *  2 -> IO error
 *  3 -> internal error
 */


#include <iostream>
#include <cstring>
#include <string>
#include <fstream>
#include <getopt.h>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <algorithm>
#include <unistd.h>
#include <netdb.h>

using namespace std;

#define MAXLINE 1024
struct sockaddr_in serveraddr;

void printHelp(){
    cout << "----- help -----" << endl;
    cout << "arguments:" << endl;
    cout << "-s: IP address or domain name of DNS server (resolver), where requests are sent." << endl;
    cout << "-p: port number, where app will await requests." << endl;
    cout << "-f: filter file, where unwanted domains are stored" << endl;
}

void printErr(string message, int returnCode = 1){
    cerr << message << endl;
    printHelp();
    exit(returnCode);
}

// Returns true, if string isn't empty and does contain only numeric characters >> represents a positive number.
bool isNumber(string str){
    return !str.empty() && (str.find_first_not_of("[0123456789]") == string::npos);
}

// Splits a string representing an IP address into 4 numbers seperated by ".".
vector<string> split(string str, char splitter){
    int i = 0;
    int pos = str.find(splitter);
    vector<string> list;

    while(pos != string::npos){
        list.push_back(str.substr(i, pos - i));
        i = ++pos;
        pos = str.find(splitter, pos);
    }
    list.push_back(str.substr(i, str.length()));
    return list;
}

// Returns true, if given string is an valid IP address.
bool validateIP(string ip){
    vector<string> pieces = split(ip, '.');         // Firsts seperates IP address into 4 numbers seperated by ".".
    if (pieces.size() != 4){
        return false;
    }

    for(string str : pieces){                       // Those numbers must be valid numbers with value between 0 and 255.
        if((stoi(str) < 0) || (stoi(str) > 255) || (!isNumber(str))){
            return false;
        }
    }
    return true;
}

// Returns socket, that is binded to desired port.
int readySocket(unsigned short port){
    struct sockaddr_in address;
    memset((char*)&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    int newSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(newSocket < 0){
        printErr("Socket creation failed", 3);
    }

    if(bind(newSocket, (struct sockaddr *)&address, sizeof(address)) < 0){
        printErr("socket binding failed...", 3);
    }

    return newSocket;
}

// Forwards request from client to specified DNS server.
socklen_t sendRequest(string serverIP, char* request, socklen_t len, char* buffer){
    char ip[serverIP.length() + 1];
    strcpy(ip, serverIP.c_str());

    int clientSocket = readySocket(56654);          // Creating and binding socket to pseudo random port number.
    int recvlen;

    // Creating address structure, where packet will be sent.
    socklen_t addrlen = sizeof(serveraddr);
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(53);
    serveraddr.sin_addr.s_addr = inet_addr(ip);

    // Forwarding packet to specified DNS server.
    if (sendto(clientSocket, request, len, 0, (struct sockaddr *)&serveraddr, addrlen) < 0) {
        printErr("request sending failed...", 3);
    }

    // Waiting for response from DNS server.
    recvlen = recvfrom(clientSocket, buffer, MAXLINE, 0, (struct sockaddr *)&serveraddr, &addrlen);

    close(clientSocket);
    return recvlen;
}

// Parses DNS requests and returns requested domain as string.
string getDomain(char* buffer){
    string domain;
    int i = 12;
    char len;
    while(buffer[i] != '\0'){
        len = buffer[i];                         // Reads numbers of bytes, that this part of domain is gonna take in DNS request.
        for(int j = i + 1; j <= len + i; j++){
            domain += buffer[j];                 // Then reads this number of characters.
        }
        domain += ".";                           // Pasting "." in between parts of this domain.
        i += len + 1;
    }
    return domain.substr(0, domain.size()-1);
}

// Returns true, if given domain is subdomain of different domain.
bool isSubDomain(vector<string> unwanted, string domain){
    for(string line : unwanted){
        if((domain.find(line) != string::npos)){
            return true;
        }
    }
    return false;
}

// copyes data from recieved packet into packet, that I'm gonna send back. Then edits specific bits in this response to generate error at client side.
void generateErrorPacket(char* buffer, char* responseBuffer){
    for (int i = 0; i < MAXLINE; i++){
        responseBuffer[i] = buffer[i];
    }

    // Thanks to bitwise OR operation, I can edit only specific bits. The rest of them can remain unchanged.
    responseBuffer[2] |= (char)128;
    responseBuffer[3] |= (char)5;
}

char* strToCharPtr(string str){
    char* ptr = &str[0];
    return ptr;
}

string charPtrToStr(char* ptr){
    string str(ptr);
    return str;
}

// Parses and processes arguments of this application.
void parseArgs(int argc, char* argv[], string* server, string* port, string* filter_file_name){
    const struct option longopts[] =
            {
                    {"help", no_argument, NULL, 'h'},
                    {0, 0, 0, 0}
            };

    int index;
    int iarg = 0;

    //enable error messages
    opterr = 1;

    while(iarg != -1){
        iarg = getopt_long(argc, argv, "hs:p:f:", longopts, &index);
        switch(iarg){
            case 'h':
                printHelp();
                exit(0);
            case 's':
                *server = optarg;
                break;
            case 'p':
                *port = optarg;
                break;
            case 'f':
                *filter_file_name = optarg;
                break;
        }
    }

    if(*server == ""){
        printErr("Input error: You must input server IP address...", 1);
    }
    if(*filter_file_name == ""){
        printErr("Input error: You must input filter file...", 1);
    }

    if(!validateIP(*server)){
        struct hostent* host;
        char* neco = strToCharPtr(*server);
        if((host = gethostbyname(strToCharPtr(*server))) == NULL){
            printErr("Enter valid IP address or host name...", 1);
        }
        // Getting IPv4 address string from hostname
        else{
            struct in_addr addr;
            int i = 0;
            while(host->h_addr_list[i] != 0){
                addr.s_addr = *(u_long*)host->h_addr_list[i++];
            }
            //addr.s_addr = *(u_long*)host->h_addr_list[0];
            *server = charPtrToStr(inet_ntoa(addr));
        }
    }
}

// Reads file line by line and returns vector of string, containing lines.
vector<string> readFile(string filter_file_name){
    string new_line;
    ifstream filter_file;
    vector<string> unwanted;

    filter_file.open(filter_file_name);

    if(filter_file.is_open()){
        while(getline(filter_file, new_line)){
            // filtering out empty and comment lines
            if((new_line[0] != '#') && (new_line[0] != '\0')){
                unwanted.push_back(new_line);
            }
        }
        filter_file.close();
    }
    else{
        printErr("Could not open filter file...", 2);
    }

    return unwanted;
}

int main(int argc, char* argv[]) {
    string server = "";
    string port = "53";
    string filter_file_name = "";

    parseArgs(argc, argv, &server, &port, &filter_file_name);     // Arguments processing.
    vector<string> unwanted = readFile(filter_file_name);           // Reading file of domains to be filtered.

    int serverSocket = readySocket(stoi(port));                     // Creating and binding socket to accept UDP communication with this server from client.
    int recvlen;

    char buffer[MAXLINE];
    char responseBuffer[MAXLINE];

    struct sockaddr_in srcaddr;
    socklen_t addrlen = sizeof(srcaddr);

    while(true){
        recvlen = recvfrom(serverSocket, buffer, MAXLINE, 0, (struct sockaddr*)&srcaddr, &addrlen);
        buffer[recvlen] = '\0';

        // if domain from DNS request is filtered, responseBuffer is modified.
        if(isSubDomain(unwanted, getDomain(buffer))){
            generateErrorPacket(buffer, responseBuffer);
        }
        // Else request is forwarded to specified DNS server.
        else{
            recvlen = sendRequest(server, buffer, recvlen, responseBuffer);
            if(recvlen <= 0){
                printErr("Could not get translation", 3);
            }
        }
        // Response is sent back to the client. Either forwarded response from specified DNS server, or Error response packet.
        sendto(serverSocket, responseBuffer, recvlen, 0, (struct sockaddr *)&srcaddr, addrlen);
    }
}