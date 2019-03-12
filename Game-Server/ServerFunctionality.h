//
// Created by Eduardo Lira on 2019-03-01.
//

#ifndef CHATSERVER_SERVERFUNCTIONALITY_H
#define CHATSERVER_SERVERFUNCTIONALITY_H
#endif //CHATSERVER_SERVERFUNCTIONALITY_H

#include <stdio.h>
#include <string.h>   //strlen
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>   //close
#include <arpa/inet.h>    //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //Using this for the FD commands.
#include <iostream> //for IO. DO NOT USE namespace std because it conflicts with names of socket programming [bind()].

#include <chrono>
#include <vector>

#define PORT 7070
#define BUFFERSIZE 4096 //4kb
#define MAXCLIENTS 20
#define MAXLOBBYSIZE 5
#define MAXLOBBIES 4

struct sockaddr_in address; //struct for machine readable address.
fd_set readfds; //set of socket descriptors.
int master_sock, addrlen, valread, sd, max_sd, check, i, client_sock[MAXCLIENTS], loggedInUsers;
std::string recieved, toSend; //IO to/from clients
char buffer[BUFFERSIZE]; //buffer for incoming messages.
std::string client_name[MAXCLIENTS]; //list of names

void runServer();

int createSocket();

int forwardMessage(); //sends message from 1 client to another. returns 0 on success. <0 on failure.

void connectAndLogin();

std::string createLoggedUserString();

/*******************************************************************************************
 * NOW IT IS TIME TO IMPLEMENT SOME CHEEKY TIMING FOR TESTING HOW IT WORKS
 *******************************************************************************************/

class timeClients {
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start; //start time stamp

    //TODO:: Do we need a destructor?

public:
    int client; //the client

    timeClients(int timeClient) {
        client = timeClient;
        start = std::chrono::high_resolution_clock::now();
    }

    timeClients() { //empty constructor
        client = 0;
        start = std::chrono::high_resolution_clock::now();
    }

    ~timeClients() {} //TODO : Do we need destructor?

    int getClient() {
        return client;
    }

    std::string getTimeStr() {  //Return time string
        auto diff = std::chrono::high_resolution_clock::now() - start;
        auto t1 = std::chrono::duration_cast<std::chrono::milliseconds>(diff);
        std::cout << "Time for reply was: " << t1.count() << std::endl;
        return "Time taken to reply was : " + std::to_string(t1.count()) + "ms." + '\n';
    }

}; //Class for timing replies (TEST for timing)

std::vector<timeClients> replyTimers(MAXCLIENTS);

//
void startTimeReply(int i) { //Time how long it takes to reply
    timeClients timer(sd); //object timer for the client
    replyTimers[i] = timer;
}

/*******************************************************************************************
 * NOW IT IS TIME TO IMPLEMENT LOBBIES AND SUCH. THE REAL STUFF
 *******************************************************************************************/

// TODO : Create lobby class or some way to manage several people together. Use same time-Stamping
//        method to find the time.
// TODO : Ask if we need multiple lobbies.

class lobbies {
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> startGame; //start time stamp
    std::string name;
    int leader; //matches leader
    int l_clients[MAXLOBBYSIZE];
    std::string l_clients_name[MAXLOBBYSIZE];
    int scoreBoard[MAXLOBBYSIZE];
    int usersInLobby;
    bool inGame; //0 for no, 1 for yes
public:

    lobbies(int client_leader, std::string leaderName, std::string lobbyName) {
        name = lobbyName; //Set the lobby name
        l_clients[0] = client_leader;
        l_clients_name[0] = leaderName;
        leader = client_leader;
        usersInLobby = 1;
        scoreBoard[0] = 0;
        inGame = 0;

        for (int i = 1; i < MAXLOBBYSIZE; i++) {
            l_clients_name[i] = "";
            l_clients[i] = 0; //set sockets to 0
        }
    } //lobby constructor

    lobbies() {
        name = ""; //Set the lobby name
        l_clients[0] = 0;
        l_clients_name[0] = "";
        leader = 0;
        usersInLobby = 0;
        scoreBoard[0] = 0;
        inGame = 0;
    }   //default constructor

    bool spaceToJoin() {
        return usersInLobby < MAXLOBBYSIZE; //while not max space //TODO : not working right
    }

    bool joinLobby(int client, std::string clientName) {
        if (!spaceToJoin()) {
            std::cout << "There is no space" <<std::endl;
            return false;
        }
        for (int i = 0; i <= MAXLOBBYSIZE; i++) {
            if (l_clients[i] == 0) {
                l_clients[i] = client; //add client
                l_clients_name[i] = clientName;
                usersInLobby++;
                toSend = "Welcome to lobby '" + name + "'!\n";
                send(client, toSend.data(), toSend.length(), 0); //send welcome message
                return true;
            }
        }
    }

    void sendLobbyList(int client) { //sends to requested user who is in lobby
        toSend = "In the lobby we have: ";
        for (int i =0 ; i < MAXLOBBYSIZE; i++) {
            if (l_clients_name[i] != "") {
                toSend += l_clients_name[i] + ", ";
            }
        }
    }

    int sendMessage(int send_client, std::string toSend) { //TODO : fix formatting
        int a = 0; //no failure
        for (int i = 0; i <= usersInLobby; i++) { //TODO : <= || < ?
            if (l_clients[i] != send_client)
                send(l_clients[i], toSend.data(), toSend.length(), 0);
        }
        toSend = "Sent to lobby!\n";
        send(send_client, toSend.data(), toSend.length(), 0);
        return 0; //success
    }

    std::string getLobbyName() {
        return name;
    }
};


std::vector<lobbies> listOfLobbies(MAXLOBBIES); //max lobbies.
int lobbiesUsed = 0;