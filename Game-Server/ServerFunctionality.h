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
#include <cmath>

#define PORT 7070
#define BUFFERSIZE 2048 //4kb //TODO : Use circular buffer
#define MAXCLIENTS 10
#define MAXLOBBYSIZE 5
#define MAXLOBBIES 4
#define GAMETIME 20000 //miliseconds //20000 SET
#define WORDSPERVECTOR 40

struct sockaddr_in address; //struct for machine readable address.
fd_set readfds; //set of socket descriptors.
int master_sock, addrlen, valread, sd, max_sd, check, i, loggedInUsers;
std::string recieved, toSend; //IO to/from clients
char buffer[BUFFERSIZE]; //buffer for incoming messages.
std::string inStr = "";


void runServer();

int createSocket();

int forwardMessage(); //sends message from 1 client to another. returns 0 on success. <0 on failure.

void connectAndLogin();

std::string createLoggedUserString();

//Generates random words for game sequence.
//Returns vector of random strings (20 strings)
std::vector<std::string> wordGenerator(int wordLength){

    std::string ranWord = "";
    std::vector<std::string> wordVec;
    srand(time(0));

    for(int i = 0; i <= WORDSPERVECTOR; ++i) {
        for(int j = 0; j < wordLength; j++) {
            char random = rand() % 58 + 65; // all letters caps and no caps
            if (random > 90 && random < 97) { // ignore none letters between
                random += 6;
            }
            ranWord += random;
        }
        wordVec.push_back(ranWord);
        ranWord = "";
    }
    return wordVec;
}

std::string stringifyVectorOfStrings(std::vector<std::string> theVector) {
    std::string result= "";
    for (int i = 0; i < theVector.size() - 1; i++) {
        result += theVector.at(i);
        result += ", ";
    }
    result += theVector.at(theVector.size()-1); //add last element
    return result;
}

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

    bool inGame;

    client() { //default constructor
        clientSock = 0;
        clientName = "";
        listOfLobbiesIndex = -1;
        inLobby = false;
        inGame = false;
    }

    client(int clientSocket, std::string nameOfClient) { //list of clients
        clientSock = clientSocket;
        clientName = nameOfClient;
        listOfLobbiesIndex = -1;
        inLobby = false;
        inGame = false;
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

client allClients[MAXCLIENTS]; //make class for clients
/*******************************************************************************************
 * NOW IT IS TIME TO IMPLEMENT SOME CHEEKY TIMING FOR TESTING HOW IT WORKS
 *******************************************************************************************/

class timeClients {
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start; //start time stamp

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

    ~timeClients() {}

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
    std::chrono::time_point<std::chrono::high_resolution_clock> gameTimeStart; //start time stamp
    std::string name;
    client leader; //matches leader
    client l_clients[MAXLOBBYSIZE];
    int scoreBoard[MAXLOBBYSIZE];
    std::vector<std::string> gameWords;
    int gameWordSize;
    int usersInLobby;
    bool inGame; //true or false
public:

    lobbies(client masterClient, std::string lobbyName) {
        name = lobbyName; //Set the lobby name
        leader = masterClient;
        l_clients[0] = leader;
        scoreBoard[0] = 0;
        usersInLobby = 1;
        int gameWordSize = 0;
        inGame = false;
        gameWords = {};

        for (int i = 1; i < MAXLOBBYSIZE; i++) {
            scoreBoard[i] = 0;
            l_clients[i].eraseClient(); //Set to empty objects
        }
    } //lobby constructor

    lobbies() {
        name = ""; //Set the lobby name
        client emptyContainer;
        leader = emptyContainer;
        usersInLobby = 0;
        inGame = false;
        int gameWordSize = 0;
        gameWords = {};
        for (int i = 0; i < MAXLOBBYSIZE; i++) {
            scoreBoard[i] = 0;
            l_clients[i].eraseClient(); //Set to empty objects
        }
    }   //default constructor

    std::chrono::time_point<std::chrono::high_resolution_clock> getTime() {
        return gameTimeStart;
    }

    bool spaceToJoin() {
        return usersInLobby < MAXLOBBYSIZE; //while not max space
    }

    std::string whoLeader() {
        return leader.getName();
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

    bool isInGame() {
        return inGame;
    }

    void sendLobbyList(int client) { //sends to requested user who is in lobby
        toSend = "In the lobby '" + name + "' we have: ";  //TODO : finsih implementing this
        for (int i =0 ; i < MAXLOBBYSIZE; i++) {
            if (l_clients[i].getName() != "") {
                toSend += l_clients[i].getName() + ", ";
            }
        }
        toSend += "\n";
        send(client, toSend.data(), toSend.length(), 0);
    }

    //Sends everyone but sender message
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

    //SENDS everyone a message
    //TODO : FIX THIS
    int sendAll(std::string message) {
        for (int i = 0; i < MAXLOBBYSIZE; i++) {
            if (l_clients[i].getSock() != 0) {
                std::cout<< "Sent the string to lobby\n";
                int a = send(l_clients[i].getSock(), message.data(), message.length(), 0);
                std::cout << "Sent byte count of : " << a << std::endl;
            }
        }
    }

    std::string getLobbyName() {
        return name;
    }

    //Starts game with length of word given by user (a is wordLength)
    void beginGame(int a) {
        gameWordSize = a;
        std::cout << "The game for lobby " + name + " is beggining.." << std::endl;
        inGame = true; //In game
        gameTimeStart = std::chrono::high_resolution_clock::now(); //TIME STAMPED
        gameWords = wordGenerator(a);
        std::string Words = stringifyVectorOfStrings(gameWords); //get random words and stringify
        Words += "$\n";
        std::cout << "The GAMEWORDS are : " << Words << "\n";
        for (int i = 0; i < MAXLOBBYSIZE; i++) {
            if (l_clients[i].getSock() != 0) {
                for (int k = 0; k < MAXCLIENTS; k++) {
                    if (allClients[k].getSock() == l_clients[i].getSock()) {
                        allClients[k].inGame = true;
                        l_clients[i] = allClients[k];
                    }
                }
            }
        }

        //Notify everyone the game is starting
        toSend = "GAME-START\n";
        sendAll(toSend);
        //Immediately after send everyone the game list
        sendAll(Words);
    }

    std::string addToScoreandReturnplaces(std::string userIn, client user) {
        std::cout << "SCORING!\n";
        std::string result, wordAgainst, toSend;
        int userPlace;
        //get users number
        int k = 2;
        int theNum = 0; //number in vector

//        if (a > 47 && a < 58) {//48 - 57
//            wordLength = wordLength * 10;
//            wordLength += (a - 48); //get int value, not char value
//
//TODO : fix this

        for(int i = 0; i < userIn.length() - 1; i++){ // itterate userIn till it finds end of actual userWord
            if (userIn.at(i) > 47 && userIn.at(i) < 58 ) {//is num?
                theNum = theNum * 10;
                theNum += (userIn.at(i) - 48); //get int value, not char value
                } else {
                //Nothing;
            }
        }
        //

//        while (userIn.at(userIn.length() - k) > 47 && userIn.at(userIn.length() - k) < 58) { //48-57
//            theNum = theNum * 10; //shift decimal left
//            theNum += (userIn.at(userIn.length() - k) - 48); //get int value
//            k++; //increase counter
//        }
        std::cout << "SCORING!\n";

        std::cout << "Number of vec is : " << theNum << std::endl;
        int wordSubmit = theNum; //penultimate char gives word to submit
        if (wordSubmit <= 0 || wordSubmit > WORDSPERVECTOR) {
            return "INVALID WORD\n";
        }
        wordAgainst = gameWords[wordSubmit-1];

        for (int i = 0; i < userIn.length()-2; i++) {//exclude \n
            result += userIn.at(i);
        }

        //find user place
        for (int i = 0; i < MAXLOBBYSIZE; i++) {
            if (l_clients[i].getName() == user.getName()) {
                userPlace = i;
                break;
            }
        }
        std::cout << "USER PLACE IS : " << userPlace << std::endl;
        std::cout << "SCORE BEFORE : " << scoreBoard[userPlace] << std::endl;
        //score the response

        //if(wordAgainst.length()  - 2 <= )
        //Placing in score board
        for (int i = 0; i < result.length() && i < wordAgainst.length(); i++) {
            if (result.at(i) == wordAgainst.at(i)) {
                scoreBoard[userPlace] += 5;
            } else {
                scoreBoard[userPlace] = scoreBoard[userPlace] - 2;
            }
        }

        //check size diff and change score by abs value
        int sizeDiff = result.length() - wordAgainst.length(); //gives difference in words (+ for too long, - for too short)
        scoreBoard[userPlace] = scoreBoard[userPlace] - (abs(sizeDiff));

        std::cout << "SCORE AFTER : " << scoreBoard[userPlace] << std::endl;

        //get the time left;
        auto diff = std::chrono::high_resolution_clock::now() - getTime();
        auto t1 = std::chrono::duration_cast<std::chrono::milliseconds>(diff);
        std::cout << "It as been : " << t1.count();
        int timeLeft = (GAMETIME - (t1.count()));
        std::cout<< "TIME LEFT IS : " << timeLeft << std::endl;

        std::string scores = "";
        for (int i = 0; i < MAXLOBBYSIZE; i++) {
            if (l_clients[i].getSock() != 0) {
                scores += (l_clients[i].getName() + " " + std::to_string(scoreBoard[i]));
                scores += ", ";
            }
        }
        //create string with user places, and time left;
        toSend = "Time left is: " + std::to_string(timeLeft) + "ms. Scores so far are : " + scores + "\n";

        return toSend;


    }

    void endGame() {
        for (int i = 0; i < MAXLOBBYSIZE; i++) {
            if (l_clients[i].getSock() != 0) {
                for (int k = 0; k < MAXCLIENTS; k++) {
                    if (allClients[k].getSock() == l_clients[i].getSock()) {
                        allClients[k].inGame = false;
                        l_clients[i] = allClients[k];
                    }
                }
            }
        }
        gameWords = {};
        inGame = false;
        sendAll("$END-GAME\n");

    };

    void emptyLobby() {
        name = ""; //Set the lobby name
        client emptyContainer;
        leader = emptyContainer;
        usersInLobby = 0;
        scoreBoard[0] = 0;
        inGame = false;
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
