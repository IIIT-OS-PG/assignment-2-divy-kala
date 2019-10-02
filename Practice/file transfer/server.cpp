#include <iostream>
#include <pthread.h>
#include <bits/stdc++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define MAX_PARALLEL_REQUESTS 20
using namespace std;

void * RequestHandler (void *) ;

int main()
{
    //server
    int serverfd;
    serverfd = socket(AF_INET, SOCK_STREAM, 0);
    if ( serverfd == -1 )
    {
        cout << "Sender socket creation failed.";
        exit(-1);
    }
    struct sockaddr_in saddr;
    saddr.sin_port = htons(50000);
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = INADDR_ANY;
    int option = 1;
    setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));


    int bret = bind(serverfd,(struct sockaddr *) &saddr, sizeof(saddr));
    if(bret)
    {
        cout << "couldnt bind \n";
        exit(-1);
    }


    if ( listen(serverfd,5) )
    {
        cout << "listening failed" << endl;
        exit(-1);
    }
    int x = sizeof(saddr);
    pthread_t tids[MAX_PARALLEL_REQUESTS];
    int i = 0;
    while(true)
    {
        int remotesock = accept(serverfd,(struct sockaddr *) &saddr,(socklen_t*) &x  );
        if(remotesock == -1)
        {

            cout << "connnection accepting failed. remote sock is " << remotesock << endl;
            exit(-1);
        }

        int newthread = pthread_create(&tids[i], NULL, RequestHandler, &remotesock);
        if( newthread != 0)
        {
            cout << "Failed to launch RequestHandler, exiting";
            exit(-1);
        }

        i++;
        if( i >= MAX_PARALLEL_REQUESTS)
        {

            for (int j = 0; j < i; j++)
            {
                pthread_join(tids[j], NULL);
            }

        }



    }
//    char buff[1000];
//
//    int datarec = recv (remotesock, buff, 999, 0);
//    buff[datarec] = '\0';
//
//    cout << buff << " is requested.\n";
//    FILE * reqfile = fopen( buff, "r");
//    fseek(reqfile, 0, SEEK_END);
//    long sizeoffile = ftell (reqfile);
//    rewind(reqfile);
//    send(remotesock, &sizeoffile, sizeof(sizeoffile), 0);
//    int r;
//    while ( (r =  fread( buff, sizeof(char), 999, reqfile) )!= 0 ) {
//    	cout << "r : " << r << endl;
//        buff[r] = '\0';
//        cout << "sending " << buff << endl << endl;
//        send(remotesock, buff, r+1, 0);
//
//    }
    //  fclose(reqfile);
//    close(remotesock);
    close(serverfd);



    return 0;
}



void * RequestHandler (void * args)
{
    int remotesock = *(int *)args;
    char buff[1000];

    int datarec = recv (remotesock, buff, 999, 0);
    buff[datarec] = '\0';

    cout << buff << " is requested.\n";
//    recv(newSocket, client_message, 2000, 0);
//
//    // Send message to the client socket
//
//    pthread_mutex_lock(&lock);
//
//    char *message = malloc(sizeof(client_message)+20);
//
//    strcpy(message,"Hello Client : ");
//
//    strcat(message,client_message);
//
//    strcat(message,"\n");
//
//    strcpy(buffer,message);
//
//    free(message);
//
//    pthread_mutex_unlock(&lock);
//
//    sleep(1);
//
//    send(newSocket,buffer,13,0);
//
//    printf("Exit socketThread \n");
//
    close(remotesock);
    pthread_exit(NULL);

}


