#include <iostream>
#include <pthread.h>
#include <bits/stdc++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
// #include <syncstream> C++ 20
#define MAX_PARALLEL_REQUESTS 20
using namespace std;

void * RequestHandler (void *) ;
// osyncstream bout(std::cout); to sync output to terminal


pthread_mutex_t sessionkeymutex = PTHREAD_MUTEX_INITIALIZER;
static int sessionkey = 1000;

struct conn_details {
    int fd;
    string ip;
    int port;
};

class User {
    public:
    string username;
    string password;
    string ip;
    int port;
    bool loggedin;

    User (string username, string password, string ip, int port, bool loggedin) {
        this->username = username;
        this->ip = ip;
        this->port = port;
        this->loggedin = loggedin;
        this->password = password;
    }

};


vector<User> listOfUsers;

map<string, User*> skeytouser;

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
    int remotesock[MAX_PARALLEL_REQUESTS];
    while(true)
    {
        remotesock[i] = accept(serverfd,(struct sockaddr *) &saddr,(socklen_t*) &x  );
        if(remotesock[i] == -1)
        {

            cout << "connnection accepting failed. remote sock is " << remotesock[i] << endl;
            exit(-1);
        }
        struct conn_details con;
        con.fd = remotesock[i];
        con.ip = inet_ntoa(saddr.sin_addr );
        con.port = ntohs(saddr.sin_port);
        int newthread = pthread_create(&tids[i], NULL, RequestHandler, &con);
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
            i=0;

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

void ServiceRegisterRequest( vector<string> sargs, struct conn_details con )
{

    FILE * fp = fopen ("userdata.txt", "a");
    string s = "\n" + sargs[1] + ";" + sargs[2];
    fwrite( s.c_str(), sizeof(char), s.length(), fp);
    fclose(fp);
    User u (sargs[1], sargs[2], con.ip, con.port, false);
    listOfUsers.push_back( u);


}

void Notify(string msg, int sockfd)
{
    char cmsg[msg.length()+1];
    strcpy(cmsg, msg.c_str());
    cmsg[msg.length()] = '\0';
    char * buff = cmsg;
    send(sockfd, buff, strlen(buff)+1, 0);
    return;
}

int GenerateSessionKey () {
    pthread_mutex_lock( &sessionkeymutex );
    int tmp = sessionkey++;
    pthread_mutex_unlock( &sessionkeymutex );
    return tmp;
}

void ServiceLoginRequest( vector<string> sargs, struct conn_details con)
{
    string msg = "User not registered";
    for( int i = 0; i < listOfUsers.size(); i++ ) {

        if(listOfUsers[i].username == sargs[1]) {

            if(listOfUsers[i].password == sargs[2] ) {
                msg = "Login successful";
                listOfUsers[i].loggedin = true;
                int skey = GenerateSessionKey();
                string sskey = to_string(skey);
                msg += ";" + sskey;
                skeytouser[sskey] = &listOfUsers[i];

                break;
            }
            else {
                msg = "Password wrong";
                break;
            }

        }

    }
    Notify(msg, con.fd);
}
vector<string> GetArgs(char* buff)
{

    vector<string> ret(0);
    char * saveptr;

    char* tok = strtok_r(buff, ";", &saveptr);
    string s (tok);
    ret.push_back(s);

    while ( (tok = strtok_r(NULL, ";", &saveptr) ))
    {

        string s (tok);

        ret.push_back(s);

    }

    return ret;
}

void ServiceLogoutRequest( vector<string> sargs)
{

    skeytouser[sargs[1]]->loggedin = false;

}


void * RequestHandler (void * args)
{
    struct conn_details con = *(struct conn_details * )args;
    int remotesock = con.fd;
    char buff[1000];

    int datarec = recv (remotesock, buff, 999, 0);


    cout << "Command received: " << buff << " \n" << flush;
    vector<string> sargs = GetArgs(buff);

    if ( sargs[0] == "create_user")
    {
        ServiceRegisterRequest(sargs, con);
    }
    else if ( sargs[0] == "login" ) {
        ServiceLoginRequest(sargs, con);
    }
    else if (sargs[0] == "logout" ) {
        ServiceLogoutRequest(sargs);
    }




    close(remotesock);
    pthread_exit(NULL);

}



