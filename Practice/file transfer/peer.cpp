#include <iostream>
#include <pthread.h>
#include <bits/stdc++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
extern int errno;
using namespace std;

sockaddr_in GetTracker()
{
    struct sockaddr_in tracker;
    tracker.sin_family = AF_INET;
    tracker.sin_port = htons(50000);
    inet_pton (AF_INET, "127.0.0.1", &tracker.sin_addr);
    return tracker;
}

void NotifyTracker(string msg, int sockfd)
{
    char cmsg[msg.length()+1];
    strcpy(cmsg, msg.c_str());
    cmsg[msg.length()] = '\0';
    char * buff = cmsg;
    send(sockfd, buff, strlen(buff)+1, 0);
    if(errno == ENOTCONN) {
        //connect to new tracker
    }
    return;
}

int main ()
{
    //client

    string input = "";

    while (input != "quit")
    {
        cout << "#";
        cin >> input;
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in saddr = GetTracker();;
        int connectret = connect (sockfd,(struct sockaddr*) &saddr, sizeof(saddr) );
        if( connectret != 0) {
            cout << "Connection to tracker failed" << endl;
            exit(-1);
        }


        if(input == "create_user")
        {

            string user_id;
            string passwd;
            cin >> user_id >> passwd;
            NotifyTracker("create_user;"+user_id+";"+passwd, sockfd);


        }
        else if (input == "login" ) {
            string user_id;
            string passwd;
            cin >> user_id >> passwd;
            NotifyTracker("login;"+user_id+";"+passwd, sockfd);
        }


         close(sockfd);


    }

    //request file
//    char * buff = "file1.txt";
//    send(sockfd, buff, strlen(buff), 0);
//
//
//    //get file
//    long filesize;
//    recv(sockfd, &filesize, sizeof(filesize), 0);
//    cout << "File size to be received is " << filesize << endl;
//    long count;
//    long recvd = 0;
//    char buff1[1000];
//    FILE * fp = fopen ("copied file.txt", "w");
//    while( (count = recv(sockfd, buff1, 1000,0) )!= 0 && recvd<filesize) {
//	cout << "received " << buff1 <<endl << endl;
//        fwrite(buff1, sizeof(char), count-1, fp);
//
//        recvd += count;
//        recvd--;
//    }


 //   fclose(fp);
    return 0;


}

