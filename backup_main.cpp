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

using namespace std;

#define MAXLINE 1024

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

bool isNumber(string str){
    return !str.empty() && (str.find_first_not_of("[0123456789]") == string::npos);
}

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

bool validateIP(string ip){
    vector<string> pieces = split(ip, '.');
    if (pieces.size() != 4){
        return false;
    }

    for(string str : pieces){
        if((stoi(str) < 0) || (stoi(str) > 255) || (!isNumber(str))){
            return false;
        }
    }
    return true;
}

socklen_t sendRequest(string serverIP, char* request, socklen_t len, char* buffer){
    char ip[serverIP.length() + 1];
    strcpy(ip, serverIP.c_str());
    int clientSocket;
    int recvlen;

    struct sockaddr_in myaddr, serveraddr;
    socklen_t addrlen = sizeof(serveraddr);

    memset((char*)&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_port = htons(56654);
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(53);
    serveraddr.sin_addr.s_addr = inet_addr(ip);

    if((clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
        printErr("Socket creation failed", 3);
    }

    if(bind(clientSocket, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0){
        printErr("socket binding failed...", 3);
    }

    if (sendto(clientSocket, request, len, 0, (struct sockaddr *)&serveraddr, addrlen) < 0) {
        printErr("request sending failed...", 3);
    }

    recvlen = recvfrom(clientSocket, buffer, MAXLINE, 0, (struct sockaddr *)&serveraddr, &addrlen);

    close(clientSocket);
    return recvlen;
}

string getDomain(char* buffer){
    string domain;
    int i = 12;
    char len;
    while(buffer[i] != '\0'){
        len = buffer[i];
        for(int j = i + 1; j <= len + i; j++){
            domain += buffer[j];
        }
        domain += ".";
        i += len + 1;
    }
    return domain.substr(0, domain.size()-1);
}

bool isSubDomain(vector<string> unwanted, string domain){
    for(string line : unwanted){
        if((domain.find(line) != string::npos)){
            return true;
        }
    }
    return false;
}

void generateErrorPacket(char* buffer, char* responseBuffer){
    for (int i = 0; i < MAXLINE; i++){
        responseBuffer[i] = buffer[i];
    }

    responseBuffer[2] |= (char)128;
    responseBuffer[3] |= (char)5;
}

int main(int argc, char* argv[]) {
    /* ==================== PARSING ARGUMENTS ==================== */

    const struct option longopts[] =
            {
            {"help", no_argument, NULL, 'h'},
            {0, 0, 0, 0}
            };

    int index;
    int iarg = 0;

    //enable error messages
    opterr = 1;

    string server = "";
    string port = "53";
    string filter_file_name = "";
    string* unwanted_domains;
    ifstream filter_file;
    vector<string> unwanted;


    while(iarg != -1){
        iarg = getopt_long(argc, argv, "hs:p:f:", longopts, &index);
        switch(iarg){
            case 'h':
                printHelp();
                return 0;
            case 's':
                server = optarg;
                break;
            case 'p':
                port = optarg;
                break;
            case 'f':
                filter_file_name = optarg;
                break;
        }
    }

    if(server == ""){
        printErr("Input error: You must input server IP address...", 1);
    }
    if(filter_file_name == ""){
        printErr("Input error: You must input filter file...", 1);
    }

    if(!validateIP(server)){
        printErr("Enter valid IP address...", 1);
    }

    /* =========================================================== */

    /* ======================= READING FILE ====================== */

    string new_line;
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

    /* =========================================================== */
    /* =============== CREATING AND BINDING SOCKETS ============== */

    int serverSocket;
    int recvlen;
    char buffer[MAXLINE];
    char responseBuffer[MAXLINE];

    if((serverSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        printErr("Socket creation failed", 3);
    }



    struct sockaddr_in myaddr, srcaddr;
    socklen_t addrlen = sizeof(srcaddr);

    memset((char*)&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(stoi(port));

    if(bind(serverSocket, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0){
        printErr("socket binding failed...", 3);
    }

    while(true){
        recvlen = recvfrom(serverSocket, buffer, MAXLINE, 0, (struct sockaddr*)&srcaddr, &addrlen);
        buffer[recvlen] = '\0';

        if(isSubDomain(unwanted, getDomain(buffer))){
            generateErrorPacket(buffer, responseBuffer);
        }
        else{
            recvlen = sendRequest(server, buffer, recvlen, responseBuffer);
            if(recvlen <= 0){
                printErr("Could not get translation", 3);
            }
        }
        sendto(serverSocket, responseBuffer, recvlen, 0, (struct sockaddr *)&srcaddr, addrlen);
    }
    return 0;
}




































