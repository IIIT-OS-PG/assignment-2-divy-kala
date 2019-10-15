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



class File {
public:
    File (string hf, string fn, User* u, vector<string>ph, long long fsize ) {
        hashoffile = hf;
        filename = fn;
        peerswithfile.push_back(u);
        piecehash = ph;
        filesize = fsize;

    }

    string hashoffile;
    string filename;
    vector<User*> peerswithfile;
    vector<string> piecehash;
    long long filesize;


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
multimap<string, string> unametogid;
int main() {
    //read registered users from per tracker file
    InitializeUsers ();

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
    int option = 1;
    setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));


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
    pthread_t tids[MAX_PARALLEL_REQUESTS];
    int i = 0;
    int remotesock[MAX_PARALLEL_REQUESTS];
    while(true) {
        remotesock[i] = accept(serverfd,(struct sockaddr *) &saddr,(socklen_t*) &x  );
        if(remotesock[i] == -1) {

            cout << "connnection accepting failed. remote sock is " << remotesock[i] << endl;
            exit(-1);
        }
        struct conn_details con;
        con.fd = remotesock[i];
        con.ip = inet_ntoa(saddr.sin_addr );
        con.port = ntohs(saddr.sin_port);
        int newthread = pthread_create(&tids[i], NULL, RequestHandler, &con);
        if( newthread != 0) {
            cout << "Failed to launch RequestHandler, exiting";
            exit(-1);
        }

        i++;
        if( i >= MAX_PARALLEL_REQUESTS) {

            for (int j = 0; j < i; j++) {
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

void ServiceRegisterRequest( vector<string> sargs, struct conn_details con ) {

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
    if(!userfile.good()) {
        userfile.close();
        return;
    }
    while (!userfile.eof() ) {
        string line;
        userfile >> line;
        if (userfile.eof())
            break;
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

void Notify(string msg, int sockfd) {
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

int GenerateSessionKey () {
    pthread_mutex_lock( &sessionkeymutex );
    int tmp = sessionkey++;
    pthread_mutex_unlock( &sessionkeymutex );
    return tmp;
}

void ServiceLoginRequest( vector<string> sargs, struct conn_details con) {
    string msg = "User not registered";

    for( auto i = unametouser.begin(); i != unametouser.end(); i++) {

        if(i->second->username == sargs[1]) {

            if(i->second->password == sargs[2] ) {
                msg = "Login successful";
                i->second->loggedin = true;
                i->second->ip = sargs[3];
                i->second->port = atoi(sargs[4].c_str());
                int skey = GenerateSessionKey();
                string sskey = to_string(skey);
                msg += ";" + sskey;
                skeytouname[sskey] = i->second->username;


                break;
            } else {
                msg = "Password wrong";
                break;
            }

        }

    }
    Notify(msg, con.fd);
}
vector<string> GetArgs(char* buff) {

    vector<string> ret(0);
    char * saveptr;

    char* tok = strtok_r(buff, ";", &saveptr);
    string s (tok);
    ret.push_back(s);

    while ( (tok = strtok_r(NULL, ";", &saveptr) )) {

        string s (tok);

        ret.push_back(s);

    }

    return ret;
}

void ServiceLogoutRequest( vector<string> sargs) {

    unametouser[skeytouname[sargs[1]]]->loggedin = false;
    skeytouname.erase(sargs[1]);

}
void ServiceGroupsFetchRequest (vector<string> sargs, struct conn_details con) {
    string ret = "";
    for( auto i = gidtogroup.begin(); i != gidtogroup.end(); i++) {
        ret += i->first + ";" + i->second->owner->ip + ";" + to_string(i->second->owner->port) + ";";


    }
    if( ret == "")
        ret = "No groups";

    Notify(ret,con.fd);


}
void ServiceCreateGroupRequest( vector<string> sargs) {

    //Update groups. Create new Group object, set owner. Make this user member by updating map<usr,grp>.


    string gid = sargs[1];
    string skey = sargs[2];
    if(skey == "-1")
        return;
    User * o = unametouser[skeytouname[skey]];
    Group * g = new Group (o, gid);
    gidtogroup[gid] = g;
    unametogid.emplace(o->username, gid);
    //    unametogid[o->username] = gid;


}

void ServiceGroupAcceptRequest( vector<string> sargs, conn_details con) {
    string skey = sargs[1];
    string uid = sargs[2];
    string gid = sargs[3];

    string requestingUid = skeytouname[skey];

    if (gidtogroup.find(gid) != gidtogroup.end() && gidtogroup[gid]->owner->username == requestingUid) {
        //   unametogid[uid] = gid;
        if(unametouser.find(uid) != unametouser.end()) {
            unametogid.emplace(uid, gid);
            Notify("Acceptance acknowledged by tracker", con.fd);
            return;
        }
    }
    Notify("Error occured", con.fd);
    return;



}

void ServiceGroupLeaveRequest( vector<string> sargs, conn_details con) {
    string skey = sargs[1];
    string gid = sargs[2];
    string req_uid = skeytouname[skey];


    //delete entry of uname,gid in unametogid multimap
    pair<std::multimap<string,string>::iterator, std::multimap<string,string>::iterator>utog = unametogid.equal_range(req_uid);
    bool left = false;
    for( auto i = utog.first; i != utog.second; i++ ) {
        if(i->second == gid) {
            unametogid.erase(i);
            Notify("Group " + i->second + " has been left by " + i->first,con.fd);
            left = true;
            break;
        }
    }


    Group *group = gidtogroup[gid];
    User *u = unametouser[req_uid];

    // delete group if owner deleted
    if (group->owner->username == u->username) {
        gidtogroup.erase(gid);
        delete group;
        return;
    }

    //TODO THIS CODE TO REMOVE USER FROM SHARED GROUP FILES HAS NOT BEEN TESTED YET
//    for (auto i = group->files.begin() ; i != group->files.end(); i++ ) {
//        for ( auto j = i->peerswithfile.begin(); j!= i->peerswithfile.end(); j++) {
    auto i = group->files.begin() ;
    auto j = i->peerswithfile.begin();

    while (i != group->files.end() ) {
        j = i->peerswithfile.begin();
        bool del = false;
        while (  j!= i->peerswithfile.end() ) {

            if( (*j)->username == req_uid ) {
                //  cout << "deleted " << (*j)->username  << (*j)->ip<< flush;

                j = i->peerswithfile.erase(j);

                if(i->peerswithfile.empty()) {
                    i = group->files.erase(i);
                    del = true;
                    break;
                }
                continue;
            }

            j++;

        }
        if(!del)
            i++;
    }
    if (left == false) {
        Notify("Group leaving failed, were you part of the group?", con.fd);
    }

    return;
}

void ServiceUploadRequest (vector<string> sargs, conn_details con) {
    string skey = sargs[1];
    string gid = sargs[2];
    string path = sargs[3];
    string filename = sargs[4];
    long long filesize = atoll(sargs[5].c_str());
    string filehash = sargs[6];
    string numOfPiecesS = sargs[7];
    int numOfPieces = atoi(numOfPiecesS.c_str());

    vector<string> piecehash(numOfPieces);
    for(int i = 0; i < numOfPieces; i++) {
        piecehash[i] = sargs[8+i];
    }



    string uid = skeytouname[skey];
    User * u= unametouser[uid];
    string actualgid = unametogid.find(uid)->second;
    if( actualgid != gid) {
        Notify("You do not belong to this group",con.fd);
        return;
    }
    Group * g = gidtogroup[gid];
    for( auto i = g->files.begin(); i != g->files.end(); i++) {
        if(i->filename == filename) {
            if(i->hashoffile == filehash) {

                i->peerswithfile.push_back(u);
                Notify("File details successfully shared (already existed)",con.fd);
                //     for( auto i = g->files.begin() ;  i != g->files.end(); i++) {
                //         cout << "lalala " << g->groupid << "  " << i->filename << "  " << i-> hashoffile << " " <<i->piecehash[0] << endl << flush;
                //     }
                return;
            } else {
                Notify("File with same name but different content already exists",con.fd);
                return;
            }
        }

    }
    g->files.push_back( File(filehash, filename, u, piecehash, filesize) );
    //  g->files.back().peerswithfile.push_back(u);
    Notify("File details successfully shared with tracker",con.fd);

//    for( auto i = g->files.begin() ;  i != g->files.end(); i++) {
//        cout << g->groupid << "  " << i->filename << "  " << i-> hashoffile << " " <<i->piecehash[0] << endl << flush;
//    }
    return;



}

void ServiceListFilesRequest(vector<string>sargs,conn_details con) {

    string gid = sargs[1];
    Group *g = gidtogroup[gid];
    string msg = "";
    for (auto i = g->files.begin() ; i != g->files.end(); i++) {
        msg += i->filename + "\t";
    }
    Notify(msg, con.fd);

    return;

}

void ServiceDownloadRequest(vector<string> sargs, conn_details con) {
    string skey = sargs[1];
    string gid = sargs[2];
    string srcpath = sargs[3];
    string uid = skeytouname[skey];

    //verify whether requesting user belongs to the group

    pair<std::multimap<string,string>::iterator, std::multimap<string,string>::iterator>utog = unametogid.equal_range(uid);
    bool authorized = false;
    for( auto i = utog.first; i != utog.second; i++ ) {
        if(i->second == gid) {
            authorized = true;
        }
    }
    if(!authorized) {
        string msg = "You do not belong to this group";
        Notify(msg,con.fd);
    }

    //fetch relevant data
//    string msg = "";
//    Group * g = gidtogroup[gid];
//    for( auto i = g->files.begin(); i != g->files.end(); i++) {
//        if(i->filename == srcpath) {
//            msg += i->hashoffile + ";";
//            msg += to_string(i->piecehash.length()) + ";";
//            for (auto j = i->piecehash.begin();
//       //     msg +=
//        }
//
//    string hashoffile;
//    string filename;
//    vector<User*> peerswithfile;
//    vector<string> piecehash;

//    }


    return;
}

void ServiceRetrieveRelevantRequest(vector<string> sargs, conn_details con) {
    string skey = sargs[1];
    string gid = sargs[2];
    string filename = sargs[3];

    string uid = skeytouname[skey];
    User * u= unametouser[uid];
    string actualgid = unametogid.find(uid)->second;
    if( actualgid != gid) {
        Notify("You do not belong to this group",con.fd);
        return;
    }
    Group * g = gidtogroup[gid];
    string msg = "";
    //Response format: hashoffile, filesize, num of pieces, piecewise hashes
    //, number of relevant peers, (username, ip, port)s
    for( auto i = g->files.begin(); i != g->files.end(); i++) {

        if(i->filename == filename) {
            msg += i->hashoffile + ";" + to_string(i->filesize) + ";" + to_string(i->piecehash.size()) +";" ;
            for (auto k = i->piecehash.begin(); k!=i->piecehash.end(); k++) {
                msg += *k + ";" ;
            }
            msg += to_string(i->peerswithfile.size()) + ";";
            for (auto j = i->peerswithfile.begin(); j != i->peerswithfile.end(); j++) {
                if((*j)->loggedin = false) continue;
                msg+= (*j)->username + ";" + (*j)->ip + ";" + to_string((*j)->port) +";";
            }
        }
    }
    Notify(msg, con.fd);
}
void * RequestHandler (void * args) {
    struct conn_details con = *(struct conn_details * )args;
    int remotesock = con.fd;
    char buff[10000];

    int datarec = recv (remotesock, buff, 10000, 0);

    if(datarec ==0) {
        close(remotesock);
        pthread_exit(NULL);
    }
    cout << "Command received: " << buff << " \n" << flush;
    vector<string> sargs = GetArgs(buff);

    if ( sargs[0] == "create_user") {
        ServiceRegisterRequest(sargs, con);
    } else if ( sargs[0] == "login" ) {
        ServiceLoginRequest(sargs, con);
    } else if (sargs[0] == "logout" ) {
        ServiceLogoutRequest(sargs);
    } else if (sargs[0] == "create_group" ) {
        ServiceCreateGroupRequest(sargs);
    } else if (sargs[0] == "list_groups" ) {
        ServiceGroupsFetchRequest(sargs, con);
    } else if (sargs[0] == "nop") {

    } else if (sargs[0] == "accept_request") {
        ServiceGroupAcceptRequest (sargs, con);
    } else if (sargs[0] == "leave_group") {
        ServiceGroupLeaveRequest (sargs, con);
    } else if (sargs[0] == "upload_file") {
        ServiceUploadRequest (sargs, con);
    } else if (sargs[0] == "list_files") {
        ServiceListFilesRequest(sargs,con);
    } else if(sargs[0] == "download") {
        ServiceDownloadRequest(sargs,con);
    } else if (sargs[0] == "retrieve_relevant") {
        ServiceRetrieveRelevantRequest(sargs,con);
    } else {

    }






    close(remotesock);
    pthread_exit(NULL);

}
















