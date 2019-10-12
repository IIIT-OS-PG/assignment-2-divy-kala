#include <iostream>
#include <pthread.h>
#include <bits/stdc++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <openssl/sha.h>
#define PSIZE (512*1024)
#define MAX_PARALLEL_REQUESTS 20

//PEER

extern int errno;
using namespace std;
string skey = "-1";
int s_port;
string s_ip;
string username;

struct User {
    string username;
    string ip;
    int port;

};

class File {
public:
    File (string filename, string path, int numOfPieces, bool piecesAvail = false ) {

        this->filename = filename;

        this->path = path;
        this->numOfPieces = numOfPieces;
        for(int i =0 ; i < numOfPieces;i++)
            piecesAvailable.push_back(piecesAvail);

    }


    string filename;
    string path;
    int numOfPieces;
    vector<bool> piecesAvailable;
    string hashoffile;
    vector<string> piecehash;


};


unordered_map<string, File> sharedFiles;
map<string, pair<string,string> > groups;

struct join_request {
    string groupid;
    string req_user;

};
vector<struct join_request> join_requests;


struct conn_details {
    int fd;
    string ip;
    int port;
};


string GetSHA (string path) {
    FILE *f = fopen (path.c_str(), "r");
    if (f == NULL) {
        cout << "couldn't compute hash because file could not be opened";
        exit(-1);

    }
    unsigned char buff[8192];
    SHA_CTX sha;

    SHA1_Init(&sha);
    while(true) {
        size_t dataread;

        dataread = fread(buff, 1, sizeof (buff), f);
        if (dataread == 0)
            break;
        SHA1_Update(&sha, buff, dataread);
    }
    unsigned char  chash[ SHA_DIGEST_LENGTH];
    fclose(f);
    SHA1_Final(chash, &sha);
    char schash[20];

    for (int i = 0; i < 20; i++) {
        sprintf(schash+i, "%02X", chash[i]);
    }

    string ret (schash);
    ret = ret.substr(0,20);
    return ret;

}

string GetSHAFromChars(char* str, int len = PSIZE)  {

    unsigned char inputbuff[len] ;
    for (int i = 0; i < len; i++) {
        inputbuff[i] = str[i];
    }
    unsigned char obuff[20];

    SHA1(inputbuff,len, obuff);
    char schash[20];
    for (int i = 0; i < 20; i++) {
        sprintf(schash+i, "%02X", obuff[i]);
    }

    string ret (schash);
    ret = ret.substr(0,20);
    return ret;


}

sockaddr_in GetTracker() {
    struct sockaddr_in tracker;
    tracker.sin_family = AF_INET;
    tracker.sin_port = htons(50000);
    inet_pton (AF_INET, "127.0.0.1", &tracker.sin_addr);
    return tracker;
}
vector<string> GetArgs(char* buff, char* delimit = ";") {

    vector<string> ret(0);
    char * saveptr;

    char* tok = strtok_r(buff, delimit, &saveptr);
    string s (tok);
    ret.push_back(s);

    while ( (tok = strtok_r(NULL, delimit, &saveptr) )) {

        string s (tok);

        ret.push_back(s);

    }

    return ret;
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


void NotifyTracker(string msg, int sockfd) {
    char cmsg[msg.length()+1];
    for(int i = 0; i < msg.length(); i++ ) {
        cmsg[i] = msg.c_str()[i];
    }
    //   strcpy(cmsg, msg.c_str());
    cmsg[msg.length()] = '\0';
    char * buff = cmsg;
    send(sockfd, buff, strlen(buff)+1, 0);
    if(errno == ENOTCONN) {
        //connect to new tracker
    }
    return;
}
/*
    • Service Query for Available Pieces
          If logged in,
          Return Available Pieces for filename
          */
void  ServiceAvailablePiecesRequest(vector<string> sargs, conn_details con) {

    if( skey != "-1") {
        Notify("Peer not logged in\n", con.fd);
        return;
    }
    string filename = sargs[1];
    string ret = "";
    if (auto i = sharedFiles.find(filename)) {
        for(auto j = i->piecesAvailable.begin(); j != i->piecesAvailable.end(); j++) {
            if(*j) ret += "1";
            else ret += "0";
        }
    }
    Notify(ret,con.fd);
    return;

}

void  ServiceGroupJoinRequest(vector<string> sargs) {
//    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
//    struct sockaddr_in saddr = GetTracker();
//    int connectret = connect (sockfd,(struct sockaddr*) &saddr, sizeof(saddr) );
//    if( connectret != 0) {
//        cout << "Connection to tracker failed" << endl;
//        exit(-1);
//    }
//    string msg = "join
//    NotifyTracker(
    struct join_request j;
    j.groupid = sargs[1];
    j.req_user = sargs[2];
    join_requests.push_back(j);


}


void ServiceRequestDownload (vector<string> sargs, conn_details con) {
       /* • Service Request Download <filename, pieceno>
        ◦ Open relevant file and start sending asked piece
        */
        string filename = sargs[0];
        int pieceno = atoi(sargs[1]);

        ifstream ifs;
        string path = sharedFiles[filename]->path;
        ifs.open(path, ios_base::binary) ;
        ifs.seekg(pieceno * PSIZE );
        char readdata[PSIZE+1];
        ifs.read(readdata, PSIZE);
        int readsize = ifs.gcount();
        readdata[readsize] = 0;
        string msg (readdata);
        Notify (msg, con.fd);
        ifs.close();

}

void *  ServicePeerServerRequests (void * args) {


    struct conn_details con = *(struct conn_details * )args;
    int remotesock = con.fd;
    char buff[1000];

    int datarec = recv (remotesock, buff, 999, 0);



    vector<string> sargs = GetArgs(buff);

    if ( sargs[0] == "join_group") {
        ServiceGroupJoinRequest(sargs);
    }
    else if (sargs[0] == "available_peer_request") {

        ServiceAvailablePiecesRequest(sargs, con);

    }

}



void * PeerServerListener  ( void * ) {

    int serverfd;
    serverfd = socket(AF_INET, SOCK_STREAM, 0);
    if ( serverfd == -1 ) {
        cout << "Server socket creation failed.";
        exit(-1);
    }
    struct sockaddr_in saddr;
    saddr.sin_port = htons(s_port);
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = INADDR_ANY;
    int option = 1;
    setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));


    int bret = bind(serverfd,(struct sockaddr *) &saddr, sizeof(saddr));
    if(bret) {
        cout << "couldnt bind Peer Server\n";
        exit(-1);
    }


    if ( listen(serverfd,5) ) {
        cout << "listening failed for Peer Server" << endl;
        exit(-1);
    }
    int x = sizeof(saddr);
    pthread_t tids[MAX_PARALLEL_REQUESTS];
    int i = 0;
    int remotesock[MAX_PARALLEL_REQUESTS];
    while(true) {
        remotesock[i] = accept(serverfd,(struct sockaddr *) &saddr,(socklen_t*) &x  );
        if(remotesock[i] == -1) {

            cout << "connnection accepting failed for peer server. remote sock is " << remotesock[i] << endl;
            exit(-1);
        }
        struct conn_details con;
        con.fd = remotesock[i];
        con.ip = inet_ntoa(saddr.sin_addr );
        con.port = ntohs(saddr.sin_port);
        int newthread = pthread_create(&tids[i], NULL, ServicePeerServerRequests, &con);
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


}

vector <string> GetMessage(int sockfd) {
    char buff[10000];
    int datarec = recv (sockfd, buff, 9999, 0);

    vector<string> sargs = GetArgs(buff);
    return sargs;
}


int main (int argc, char* argv[]) {
    //get PeerServer ip,port from command line

    if(argc==3) {
        s_port = atoi(argv[2]);
        s_ip = argv[1];

    } else {
        s_port = 50001;
        s_ip = "127.0.0.1";
    }

    //create PeerServer thread
    pthread_t tserv;
    int newthread = pthread_create(&tserv, NULL, PeerServerListener, NULL);
    if( newthread != 0) {
        cout << "Failed to launch PeerServerHandler, exiting";
        exit(-1);
    }



    //PeerClient code
    string input = "";
    skey = "-1";
    while (input != "quit") {
        cout << "#";
        cin >> input;
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in saddr = GetTracker();;
        int connectret = connect (sockfd,(struct sockaddr*) &saddr, sizeof(saddr) );
        if( connectret != 0) {
            cout << "Connection to tracker failed" << endl;
            exit(-1);
        }


        if(input == "create_user") {

            string user_id;
            string passwd;
            cin >> user_id >> passwd;
            NotifyTracker("create_user;"+user_id+";"+passwd, sockfd);


        } else if (input == "login" ) {
            string user_id;
            string passwd;
            cin >> user_id >> passwd;

            NotifyTracker("login;"+user_id+";"+passwd+";"+s_ip+";"+to_string(s_port), sockfd);

            vector<string> sargs = GetMessage(sockfd);
            if(sargs[0] == "Login successful") {
                skey = sargs[1];
                cout << sargs[0] << endl << "Session key is " << skey << endl;
                username = user_id;
            } else {
                cout << sargs[0] << endl;
            }
        } else if (input == "logout" ) {
            NotifyTracker("logout;"+skey, sockfd);
            skey = "-1";

        } else if (input == "create_group" ) {
            string groupid;
            cin >> groupid;
            NotifyTracker("create_group;"+groupid+";"+skey, sockfd);


        } else if (input == "list_groups" ) {
            NotifyTracker("list_groups;"+skey, sockfd);
            vector<string> sargs = GetMessage(sockfd);
            if(sargs[0] == "No groups") {

                cout << sargs[0] << endl;
                close(sockfd);
                continue;

            }
            vector<string> sargsGrp;
            vector<string> sargsIP;
            vector<string> sargsPort;

            groups.clear();
            for(int i = 0; i < sargs.size(); i++) {
                if((i+1) % 3==1 ) {
                    sargsGrp.push_back(sargs[i] );

                } else if ( (i+1) %3 ==2 ) {
                    sargsIP.push_back(sargs[i]);
                } else if ( (i+1) % 3 == 0) {
                    sargsPort.push_back(sargs[i]);
                }


            }
            for(int i = 0; i < sargsGrp.size(); i++) {
                cout << sargsGrp[i] << " " << sargsIP[i] << " " << sargsPort[i] << endl << flush;
                groups[ sargsGrp[i] ] = make_pair(sargsIP[i], sargsPort[i]);

            }

        } else if (input == "join_group") {
            //after executing this tracker shuts down for some reason
            close(sockfd);

            string groupid;
            cin >> groupid;
            if(groups.find(groupid) == groups.end() ) {
                cout << "Such a group does not exist" << endl;
                close(sockfd);
                continue;
            }
            string ip = groups [groupid].first;
            int port = atoi(groups[groupid].second.c_str());

            int peerfd = socket(AF_INET, SOCK_STREAM, 0);

            struct sockaddr_in peer;
            peer.sin_family = AF_INET;
            peer.sin_port = htons(port);
            inet_pton (AF_INET, ip.c_str(), &peer.sin_addr);

            int joingrppeer = connect (peerfd,(struct sockaddr*) &peer, sizeof(peer) );
            Notify("join_group;"+groupid+";"+username, peerfd);
            close(peerfd);
            continue;

        } else if (input == "list_requests") {
            for(int i = 0; i < join_requests.size(); i++) {
                cout << join_requests[i].groupid << " : " << join_requests[i].req_user << endl << flush;
                NotifyTracker("nop",sockfd);
            }
        } else if (input == "accept_request") {
            string gid, userid;
            cin >> gid >> userid;
            NotifyTracker("accept_request;" + skey +";" + userid + ";" + gid, sockfd);
            cout << GetMessage(sockfd)[0] << endl;
            for(auto i = join_requests.begin(); i != join_requests.end(); i++) {
                if ( i->groupid == gid &&i->req_user == userid) {
                    join_requests.erase(i);
                    break;
                }
            }
        } else if (input == "leave_group") {
            string gid;
            cin >> gid;
            NotifyTracker ("leave_group;" + skey +";" + gid, sockfd);
            cout << GetMessage(sockfd)[0] << endl;

        } else if (input == "upload_file") {
            string path, gid;
            cin >> path >> gid;

            //get file name
            char tmp[path.length()];
            strcpy(tmp,path.c_str());
            vector <string> toks = GetArgs(tmp,"/");
            string filename = toks[toks.size()-1] ;

            //get file hash
            string filehash = GetSHA(path);


            //get file size
            long long filesize = 0;
            ifstream ifs (path, std::ifstream::binary);
            if (ifs) {
                ifs.seekg (0, ifs.end);
                filesize = ifs.tellg();
                ifs.seekg (0, ifs.beg);
            } else {
                cout << "file could not be opened";
                exit(-1);
            }


            //get number of pieces and piece wise hash
            int numOfPieces = ceil((float)filesize/PSIZE);
            //cout << "pieces = " << numOfPieces << endl;
           // cout << filehash << endl;
            string piecehash[numOfPieces];
            for(int i = 0; i < numOfPieces; i++) {
                char tmp[PSIZE];
                ifs.read( tmp, PSIZE);

                piecehash[i] = GetSHAFromChars(tmp, ifs.gcount());
              //  cout << i <<". = " << piecehash[i] << endl;
            }


            ifs.close();


            // send file details to tracker
            string msg = "upload_file;" + skey +";" + gid + ";" + path
                        + ";" + filename + ";" + to_string(filesize) + ";"
                            +filehash + ";" + to_string(numOfPieces) + ";";
            for(int i = 0; i < numOfPieces; i++) {
                msg += piecehash[i] + ";";
            }
            NotifyTracker (msg, sockfd);
            //TODO: TEST ON EMPTY FILE
            cout << GetMessage(sockfd)[0] << endl;



            // update sharedfiles unordered_map with filename,
            //path, available pieces etc
            sharedFiles.emplace(filename, File(filename,path,numOfPieces, true));
        } else if (input == "list_files") {
            string gid;
            cin >> gid;
            NotifyTracker("list_files;" + gid, sockfd);
            cout << GetMessage(sockfd)[0] << endl;

        } else if (input == "download") {
            string gid, filename, destpath;
            cin >> gid >> filename >> destpath;

            //Retrieve relevant peers, hash from tracker for the file,
            //piecewise hash,file size,
            NotifyTracker("retrieve_relevant;" + skey +
                            ";"+gid+";"+filename+";",sockfd);
            vector<string> sargs = GetMessage(sockfd);
            if(sargs[0] == "You do not belong to this group") {

                cout <<sargs[0]<<endl;
                close(sockfd);
                continue;
            }
            string hashoffile = sargs[0];
            long long filesize = atoll(sargs[1]);
            int numOfPieces = atoi(sargs[2]);
            vector<string> piecehashes;
            vector<User> relevantPeers;
            for(int i = 0; i < numOfPieces; i++) {
                piecehashes.push_back(sargs[i+3];
            }
            int numOfPeers = atoi(sargs[3+numOfPieces]);
            for(int i = 0; i < numOfPeers; i++) {
                struct User u;
                u.username = sargs [4+numOfPieces+i];
                u.ip = sargs [4+numOfPiece +i +1];
                u.port = atoi ( sargs [4 +numOfPieces + i +2] );
                relevantUsers.push_back(u);
            }


            //create NULL file, lots of bugs to fix












             //NotifyTracker("download;" + skey + ";"+gid+";"+srcpath+";",sockfd);
        } else if (input == "show_downloads") {


        }
        else {


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

