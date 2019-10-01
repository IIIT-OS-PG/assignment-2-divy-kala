#include <iostream>
#include <pthread.h>
#include <bits/stdc++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
using namespace std;


int main () {
    //client

    //setup connection
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in saddr;

    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(50000);
    inet_pton (AF_INET, "127.0.0.1", &saddr.sin_addr);
    connect (sockfd,(struct sockaddr*) &saddr, sizeof(saddr) );

    //request file
    char * buff = "file1.txt";
    send(sockfd, buff, strlen(buff), 0);


    //get file
    long filesize;
    recv(sockfd, &filesize, sizeof(filesize), 0);
    cout << "File size to be received is " << filesize << endl;
    long count;
    long recvd = 0;
    char buff1[1000];
    FILE * fp = fopen ("copied file.txt", "w");
    while( (count = recv(sockfd, buff1, 1000,0) )!= 0 && recvd<filesize) {
	cout << "received " << buff1 <<endl << endl;
        fwrite(buff1, sizeof(char), count-1, fp);

        recvd += count;
        recvd--; 	
    }

    close(sockfd);
    fclose(fp);
    return 0;


}

