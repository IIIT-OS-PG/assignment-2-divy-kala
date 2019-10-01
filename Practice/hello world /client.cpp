#include <iostream>
#include <pthread.h>
#include <bits/stdc++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;


int main () {
    //client
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in saddr;

    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(50000);
    inet_pton (AF_INET, "127.0.0.1", &saddr.sin_addr);
    connect (sockfd,(struct sockaddr*) &saddr, sizeof(saddr) );
    char * buff = "hello world";
    cout << "sending bytes: " << strlen(buff) << endl;
    send(sockfd, buff, strlen(buff), 0);
    
    return 0;


}

