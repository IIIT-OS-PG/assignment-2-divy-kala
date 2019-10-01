#include <iostream>
#include <pthread.h>
#include <bits/stdc++.h>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace std;

int main() {
    //server
    int serverfd;
    serverfd = socket(AF_INET, SOCK_STREAM, 0);
    if ( serverfd == -1 ) {
        cout << "Sender socket creation failed.";
        exit(-1);
    }
    struct sockaddr_in saddr;
    saddr.sin_port = htons(50000);
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = INADDR_ANY;


    int bret = bind(serverfd,(struct sockaddr *) &saddr, sizeof(saddr));
    if(bret) {
        cout << "couldnt bind \n";
        exit(-1);
    }


    if ( listen(serverfd,5) ) {
        cout << "listening failed" << endl;
        exit(-1);
    }
    int x = sizeof(saddr);
    int remotesock = accept(serverfd,(struct sockaddr *) &saddr,(socklen_t*) &x  );
    if(remotesock == -1) {

        cout << "accepting failed. remote sock is " << remotesock << endl;
        exit(-1);
    }
    char buff[1000];

    int datarec = recv (remotesock, buff, 1000 , 0);
    cout << "data received " << datarec << endl;
    buff[datarec] = '\0';
    cout << buff;

	return 0;
}


