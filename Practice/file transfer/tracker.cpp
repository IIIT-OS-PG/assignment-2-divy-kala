#include <iostream>
#include <pthread.h>
#include <bits/stdc++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
// #include <syncstream> C++ 20
#define MAX_PARALLEL_REQUESTS 20
// TRACKER
using namespace std;

void * RequestHandler (void *) ;
// osyncstream bout(std::cout); to sync output to terminal


pthread_mutex_t sessionkeymutex = PTHREAD_MUTEX_INITIALIZER;
static int sessionkey = 1000;
vector<string> GetArgs(char* buff);
void InitializeUsers() ;
struct conn_details
{
    int fd;
    string ip;
    int port;
};

class User
{
public:
    string username;
    string password;
    string ip;
    int port;
    bool loggedin;

    User (string username, string password, string ip, int port, bool loggedin)
    {
        this->username = username;
        this->ip = ip;
        this->port = port;
        this->loggedin = loggedin;
        this->password = password;
    }

};



class File {
    public:
    string hashoffile;
    string filename;
    vector<User> peerswithfile;
    vector<string> piecehash;


};

class Group {
public:
    Group (User* o, string gid) {
        owner = o;
        groupid = gid;

    }

    User* owner;
    string groupid;
    vector<File> files;


};




map<string, string> skeytouname;
map<string, User*> unametouser;
map<string, Group *> gidtogroup;
map<string, string> unametogid;
int main()
{
    //read registered users from per tracker file
    InitializeUsers ();

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
    User *u = new User (sargs[1], sargs[2], con.ip, con.port, false);
    unametouser[sargs[1]] = u;


}

void InitializeUsers () {
    ifstream userfile;
    userfile.open("userdata.txt");

    while (!userfile.eof() ) {
        string line;
        userfile >> line;
        if (userfile.eof()) break;
        char buff[1000];
        strcpy(buff, line.c_str());
        vector<string> sargs = GetArgs(buff);
        if (sargs[0] == "\n") {
            continue;
        }
        User *u = new User (sargs[0], sargs[1], "-1", -1, false);
        unametouser[sargs[0]] = u;
    }

    userfile.close();

}

void Notify(string msg, int sockfd)
{
    char cmsg[msg.length()+1];
    for(int i = 0; i < msg.length(); i++ ) {
    	cmsg[i] = msg.c_str()[i];
    }
 //   strcpy(cmsg, msg.c_str());
    cmsg[msg.length()] = '\0';
    char * buff = cmsg;
    send(sockfd, buff, strlen(buff)+1, 0);
    return;
}

int GenerateSessionKey ()
{
    pthread_mutex_lock( &sessionkeymutex );
    int tmp = sessionkey++;
    pthread_mutex_unlock( &sessionkeymutex );
    return tmp;
}

void ServiceLoginRequest( vector<string> sargs, struct conn_details con)
{
    string msg = "User not registered";

    for( auto i = unametouser.begin(); i != unametouser.end(); i++)
    {

        if(i->second->username == sargs[1])
        {

            if(i->second->password == sargs[2] )
            {
                msg = "Login successful";
                i->second->loggedin = true;
                i->second->ip = sargs[3];
                i->second->port = atoi(sargs[4].c_str());
                int skey = GenerateSessionKey();
                string sskey = to_string(skey);
                msg += ";" + sskey;
                skeytouname[sskey] = i->second->username;


                break;
            }
            else
            {
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

    unametouser[skeytouname[sargs[1]]]->loggedin = false;
    skeytouname.erase(sargs[1]);

}
void ServiceGroupsFetchRequest (vector<string> sargs, struct conn_details con) {
    string ret = "";
    for( auto i = gidtogroup.begin(); i != gidtogroup.end(); i++)
    {
        ret += i->first + ";" + i->second->owner->ip + ";" + to_string(i->second->owner->port) + ";";


    }
    if( ret == "") ret = "No groups";

    Notify(ret,con.fd);


}
void ServiceCreateGroupRequest( vector<string> sargs)
{

    //Update groups. Create new Group object, set owner. Make this user member by updating map<usr,grp>.


    string gid = sargs[1];
    string skey = sargs[2];
    if(skey == "-1") return;
    User * o = unametouser[skeytouname[skey]];
    Group * g = new Group (o, gid);
    gidtogroup[gid] = g;
    unametogid[o->username] = gid;


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
    else if ( sargs[0] == "login" )
    {
        ServiceLoginRequest(sargs, con);
    }
    else if (sargs[0] == "logout" )
    {
        ServiceLogoutRequest(sargs);
    }
    else if (sargs[0] == "create_group" )
    {
        ServiceCreateGroupRequest(sargs);
    }
    else if (sargs[0] == "list_groups" )
    {
        ServiceGroupsFetchRequest(sargs, con);
    }


    else {

    }




    close(remotesock);
    pthread_exit(NULL);

}



