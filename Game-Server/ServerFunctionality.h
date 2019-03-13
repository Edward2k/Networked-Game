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
#define BUFFERSIZE 16384 //16kb //TODO : Use circular buffer
#define MAXCLIENTS 10
#define MAXLOBBYSIZE 5
#define MAXLOBBIES 4

struct sockaddr_in address; //struct for machine readable address.
fd_set readfds; //set of socket descriptors.
int master_sock, addrlen, valread, sd, max_sd, check, i, loggedInUsers;
std::string recieved, toSend; //IO to/from clients
char buffer[BUFFERSIZE]; //buffer for incoming messages.

void runServer();

int createSocket();

int forwardMessage(); //sends message from 1 client to another. returns 0 on success. <0 on failure.

void connectAndLogin();

std::string createLoggedUserString();

bool compareBuffToStr(char * bufferInQuestion, std::string stringInQuestion) {
    int buffSize = sizeof(bufferInQuestion);
    for (int i = 0; i < buffSize; i++) {
        if (bufferInQuestion[i] != stringInQuestion.at(i)) {
            memset(&buffer[0], 0, sizeof(buffer)); //clear the buffer to prevent from happening again
            return false;
        }
    }
    return true;
} //compares to see if buffer contains string.

/*******************************************************************************************
 * Define the client class for easy managment of lobbies and names etc.
 *******************************************************************************************/

class client {
private:
    int clientSock; //socket of the client
    std::string clientName; //name
    int listOfLobbiesIndex;
    bool inLobby; //is he in a lobby

public:

    client() { //default constructor
        clientSock = 0;
        clientName = "";
        listOfLobbiesIndex = -1;
        inLobby = false;
    }

    client(int clientSocket, std::string nameOfClient) { //list of clients
        clientSock = clientSocket;
        clientName = nameOfClient;
        listOfLobbiesIndex = -1;
        inLobby = false;
    }

    std::string getName() {
        return clientName;
    }
    int getSock() {
        return clientSock;
    }

    void eraseClient() {
        clientSock = 0;
        clientName = "";
        listOfLobbiesIndex = -1;
        inLobby = false;
    }

    int getLobbyIndex() {
        if (inLobby == true) {
            return listOfLobbiesIndex;
        }
        return -1; //not in lobby
    }

    int joinLobbyIndex(int a) {
        listOfLobbiesIndex = a;
        inLobby = true;
    }

    void exitLobby() {
        listOfLobbiesIndex = -1;
        inLobby = false;
    }

    bool isInLobby() {return inLobby;}
};

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
    client leader; //matches leader
    client l_clients[MAXLOBBYSIZE];
    int scoreBoard[MAXLOBBYSIZE];
    int usersInLobby;
    bool inGame; //0 for no, 1 for yes
public:

    lobbies(client masterClient, std::string lobbyName) {
        name = lobbyName; //Set the lobby name
        leader = masterClient;
        l_clients[0] = leader;
        usersInLobby = 1;
        scoreBoard[0] = 0;
        inGame = 0;

        for (int i = 1; i < MAXLOBBYSIZE; i++) {
            l_clients[i].eraseClient(); //Set to empty objects
        }
    } //lobby constructor

    lobbies() {
        name = ""; //Set the lobby name
        client emptyContainer;
        leader = emptyContainer;
        usersInLobby = 0;
        scoreBoard[0] = 0;
        inGame = 0;
    }   //default constructor

    bool spaceToJoin() {
        return usersInLobby < MAXLOBBYSIZE; //while not max space
    }

    bool joinLobby(client addClient) {
        if (!spaceToJoin()) {
            std::cout << "There is no space" <<std::endl;
            return false;
        }
        for (int i = 0; i < MAXLOBBYSIZE; i++) {
            if (l_clients[i].getSock() == 0) {
                l_clients[i] = addClient; //add client
                usersInLobby++;
                toSend = "Welcome to lobby '" + name + "'!\n";
                send(addClient.getSock(), toSend.data(), toSend.length(), 0); //send welcome message
                sendMessage(addClient.getSock(), (addClient.getName() + " has joined the lobby.\n"));
                return true;
            }
        }
    }

    void sendLobbyList(int client) { //sends to requested user who is in lobby
        toSend = "In the lobby we have: ";  //TODO : finsih implementing this
        for (int i =0 ; i < MAXLOBBYSIZE; i++) {
            if (l_clients[i].getName() != "") {
                toSend += l_clients[i].getName() + ", ";
            }
        }
    }

    int sendMessage(int send_client, std::string toSend) {
        for (int i = 0; i < MAXLOBBYSIZE; i++) {
            if (l_clients[i].getSock() != send_client && l_clients[i].getSock() != 0) {
                std::cout<< "Sent the message to lobby\n";
                int a = send(l_clients[i].getSock(), toSend.data(), toSend.length(), 0);
                std::cout << "Sent byte count of : " << a << std::endl;
            }
        }
        return 0; //success
    }

    std::string getLobbyName() {
        return name;
    }

    void emptyLobby() {
        name = ""; //Set the lobby name
        client emptyContainer;
        leader = emptyContainer;
        usersInLobby = 0;
        scoreBoard[0] = 0;
        inGame = 0;
        //Global scope

    }

    int exitLobby(client user) {
        std::cout << "User is exiting a lobby\n";
        client emptyContainer;
        for (int i = 0; i < MAXLOBBYSIZE; i++) {
            if (l_clients[i].getName() == user.getName()) { //user was located
                if (l_clients[i].getName() == leader.getName()) {  //if he was lobby leader, we have to shuffle to make new leader and keep lobby open
                    toSend = "You were the group leader. Giving next player leader status\n";
                    send(user.getSock(), toSend.data(), toSend.length(), 0);
                    l_clients[i] = emptyContainer;
                    usersInLobby--;
                    if (usersInLobby > 0) { //Look if there are any other users
                        toSend = user.getName() + " has left the lobby.\n";
                        sendMessage(user.getSock(), toSend);
                        for (int k = 0; k < MAXLOBBYSIZE; k++) { //Look for new lobby leader
                            if (l_clients[k].getSock() != 0) {
                                leader = l_clients[k];
                                toSend = "You are now group leader!\n";
                                send(leader.getSock(), toSend.data(), toSend.length(), 0);
                                break;
                            }
                        }
                    } else {
                        std::cout << "Lobby is empty... ERASING\n";
                        emptyLobby();
                        break;
                    }
                } else { //Erase the client (not leader)
                    l_clients[i] = emptyContainer;
                    usersInLobby--;
                    toSend = "You have left lobby : "+ name + "\n";
                    send(user.getSock(), toSend.data(), toSend.length(), 0);
                    sendMessage(user.getSock(), (user.getName() + " has left the lobby.\n"));
                    break;
                }
            }
        }
        return usersInLobby; // 0 means the lobby is empty
    }
};


std::vector<lobbies> listOfLobbies(MAXLOBBIES); //max lobbies.
int lobbiesUsed = 0;

int nextFreeLobby() {
    for (int i = 0; i < MAXLOBBIES; i++) {
        if (listOfLobbies[i].getLobbyName() == "") { //lobby is free
            return i;
        }
    }
    return -1;
}

bool isLobbyNameOriginal(std::string lobbyName) {
    for (int i = 0; i < MAXLOBBIES; i++) {
        if (listOfLobbies[i].getLobbyName() == lobbyName) {
            return false;
        }
    }
    return true;
}
