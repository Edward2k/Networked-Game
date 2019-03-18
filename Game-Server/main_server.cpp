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
#include <stdexcept> //for error handling

#include <chrono>
#include <vector>
#include <ctime>
#include "ServerFunctionality.h"



int createSocket() {
    memset(&buffer[0], 0, sizeof(buffer)); // clear buffer to free the stream
    //'''struct sockaddr_in address''' //defined in ServerFunctionality.h
    //''fd_set readfds'' // The set of socket descriptors defined in header file
    //

    /*--------------------------------------------------------------------------------------------
     * NOW WE MUST INITIALIZE ALL THE SOCKETS TO 0 SO THEY ARE NOT READ BY THE 'SELECT()' FUNCTION.
     *--------------------------------------------------------------------------------------------- */

    for (int i=0; i < MAXCLIENTS; i++) {
        client newClient;
        allClients[i] = newClient;
    }

    /*--------------------------------------------------------------------------------------------
     * NOW WE MUST CREATE A MASTER SOCKET FOR ALL SUBSEQUENT SOCKETS THAT MIGHT BE CONNECTED
     *--------------------------------------------------------------------------------------------- */

    //??? Maybe we should include sockopt out of 'GOOD HABBIT'?

    master_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (master_sock < 0) { //check for error.
        throw std::runtime_error("Could not create the socket... EXITING!");
    }

    /*--------------------------------------------------------------------------------------------
     * NOW WE MUST CREATE A MASTER SOCKET FOR ALL SUBSEQUENT SOCKETS THAT MIGHT BE CONNECTED
     *--------------------------------------------------------------------------------------------- */

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT ); //port is set to 6969

    check = bind(master_sock, (struct sockaddr *)&address, sizeof(address));
    if (check < 0) {
        throw std::runtime_error("Could not bind the socket. Quitting.");
    }

    /*--------------------------------------------------------------------------------------------
     * NOW WE MUST CREATE A LISTENER ON THE MASTER SOCKET
     *--------------------------------------------------------------------------------------------- */

    check = listen(master_sock, MAXCLIENTS);
    if (check < 0) {
        throw std::runtime_error("Could not create a listener. Quitting.");
    }

    std::cout << "Listening on port : " << PORT << std::endl;

    /*--------------------------------------------------------------------------------------------
     * ACCEPT ALL INCOMING CONNECTIONS.
     *--------------------------------------------------------------------------------------------- */

    addrlen = sizeof(address); //size of address
    std::cout << "Awaiting connections....." << std::endl;
    return 0;//no error
}

void connectAndLogin() {
    /*--------------------------------------------------------------------------------------------
    * There is a new connection in the master_sock. Accept it and login.
    *--------------------------------------------------------------------------------------------- */

    int new_socket = accept(master_sock, (struct sockaddr *)&address, (socklen_t*)&addrlen);
    if (new_socket < 0) { //error
        throw std::runtime_error("Could not create subclass of mastersock.");
    }

    //Print to command line that there is a new connection
    std::cout << "[NEW][CONNECTION] : There is a new connection" << std::endl;

    /*--------------------------------------------------------------------------------------------
    * recv the message and store it in the buffer 'buffer', then clear the buffer.
    *--------------------------------------------------------------------------------------------- */

    memset(&buffer[0], 0, sizeof(buffer)); // clear buffer to free the stream
    recv(new_socket, buffer, BUFFERSIZE, 0);
    recieved = buffer; // save to string.
    memset(&buffer[0], 0, sizeof(buffer));

    /*--------------------------------------------------------------------------------------------
   * Seperate the message recv in the buffer and get the name in 'string name'
   *--------------------------------------------------------------------------------------------- */
    std::string name;
    for (int i=11; i < recieved.size()-1; i++) { //exclude '\n'
        name = name + recieved[i];
    }

    std::cout<< "User has chosen name : " << name << std::endl;
    client newClient(new_socket, name);
    /*--------------------------------------------------------------------------------------------
    * Check to see if name is available (yes: add to socket and client name. else:: close socket
     * and reply 'Busy\n' to indicate the server is full.
    *--------------------------------------------------------------------------------------------- */
    for (i = 0; i <= MAXCLIENTS; i++) { //use <= in the case its full and go one more
        //Check if name is available
        if (allClients[i].getName() == name) {
            std::cout << "USER HAS CHOSEN INVALID NAME\n";
            toSend = "IN-USE\n";
            int a = send(new_socket, toSend.data(), toSend.length(), 0); //send to socket that is being opened. (new_socket)
            break;
        } else if(allClients[i].getSock() == 0) { //if slot is empty, add to socket list
            std::cout << "[UPDATE][SOCKET] : Placed in socket location : " << i << std::endl;
            loggedInUsers++; //increment number of logged in users.
            //Complete the handshake
            toSend = "HELLO " + name + '\n';
            send(newClient.getSock(), toSend.data(), toSend.length(), 0);
            allClients[i] = newClient;
            break; //exit the loop
        }
        if (i == MAXCLIENTS-1 ) {
            //SERVER IS FULL
            std::cout << "SERVER IS FULL\n";
            toSend = "BUSY\n";
            send(new_socket, toSend.data(), toSend.length(), 0);
        }
    }
}

int forwardMessage(int i) { //the I is the index used in the runServer Loop. Used to index fromUsr
    //Message to process is stored in the buffer 'buffer'
    //SEND messages arrive in format 'SEND <targetUserByName> <messageBody>
    std::string targetUser, messageBody, fromUser;
    int k; //k is general index;

    //Get content from buffer
    for (k = 5; buffer[k] != ' '; k++) {
        targetUser += buffer[k];
    }
    for (k; buffer[k] != '\n'; k++) { //includes space.
        messageBody += buffer[k];
    }

    fromUser = allClients[i].getName(); //from user name

    //Find what socket the client needed is connected on.
    for (k = 0; k <= MAXCLIENTS; k++) {
        if (allClients[k].getName() == targetUser) {
            if(allClients[k].inGame) {
                toSend = "The user is currently in a game. Please try again later.\n";
                send(allClients[i].getSock(), toSend.data(), toSend.length(), 0);//Send message to the target user.
                return 0;
            } else {
                toSend = "DELIVERY " + fromUser + messageBody + "\n";
                int a = send(allClients[k].getSock(), toSend.data(), toSend.length(), 0);//Send message to the target user.
            }
            break;
        } else if (k == MAXCLIENTS) { //if not in list, send this message.
            return -1;
        }
    }

//    std::cout << "SENT: " << a << std::endl;

    return 0; //Success.
}

std::string createLoggedUserString() {
    std::string message = "WHO-OK ";
    std::cout << "The number of logged in users is : " << loggedInUsers << std::endl;
    int j = 0; //need to seperate count from main loop.
    for (int i = 0; i < MAXCLIENTS; i++) { //logged in for the number of users logged in.
        std::cout << "The user at index i is : " << allClients[i].getName() << std::endl;
        if (allClients[i].getName() == "") {
            //Do nothing, skip the empty slot.
        } else {
            if (j == loggedInUsers-1){ // check to see if last or not (for formatting)
                message += allClients[i].getName(); //add last client without comma for formatting in Client side
                break;
            } else {
                message += allClients[i].getName() + ',';
            }
            j++;//increment the second index for the second array
        }
    }
    std::cout<< "The final message is : " << message << std::endl;
    message += '\n';
    return message;
}


void runServer() {

    createSocket();

    while (true) {

        /*--------------------------------------------------------------------------------------------
       * SET UP SERVER
       *--------------------------------------------------------------------------------------------- */
        memset(&buffer[0], 0, sizeof(buffer));
        //clear the socket set
        FD_ZERO(&readfds);

        //add master socket to set
        FD_SET(master_sock, &readfds);
        max_sd = master_sock;

        for (int i = 0; i < MAXCLIENTS; i++) {
            sd = allClients[i].getSock();

            //if valid socket descriptor then add to read list
            if (sd > 0) {
                FD_SET(sd, &readfds);
            }

            //This is the highest possible file descriptor number. This is needed for the select() function used to pool
            //multiple users at the same time instead of multithreading.
            if (sd > max_sd) {
                max_sd = sd;
            }

        } //end for loop that initializes descriptors.

        /*--------------------------------------------------------------------------------------------
         * Wait for activity on one of the MAXCLIENTS ports (5). NULL indicates no timeout time.
         *--------------------------------------------------------------------------------------------- */

        int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0 && errno != EINTR) { //if there is an error: TODO: lookup errno!=EINTR
            throw std::runtime_error("Could not select! Adios!");
        }
        /*--------------------------------------------------------------------------------------------
        * CHECK GAMES TIMES
        *--------------------------------------------------------------------------------------------- */
        for (int i = 0; i < MAXLOBBIES; i++) {
            //Check difference
            auto diff = std::chrono::high_resolution_clock::now() - listOfLobbies[i].getTime();
            auto t1 = std::chrono::duration_cast<std::chrono::milliseconds>(diff);
            //if in game, do something
            if (listOfLobbies[i].isInGame()) {
                std::cout << "Lobby has been in game for : " << t1.count() << std::endl;
                if (t1.count() > GAMETIME) { //if time is over tie
                    std::cout << "GAME HAS EXPIRED\n.";
                    listOfLobbies[i].endGame();
                    for (int k = 0; k < MAXCLIENTS; k++) {
                        if (allClients[k].getLobbyIndex() == i) {
                            listOfLobbies[i].scoreGame(allClients[k]);
                        }
                    }
                }
            }
        }
        /*--------------------------------------------------------------------------------------------
         * if and only if something happens on the master socket, that means there is activity
         * We must connect and login.
         *--------------------------------------------------------------------------------------------- */

        if (FD_ISSET(master_sock, &readfds)) { //login
            std::cout << "CONNECTING...\n";
            connectAndLogin();
        } //endif

        /*--------------------------------------------------------------------------------------------
        * anything else, there is some IO operation on another active socket.
        *--------------------------------------------------------------------------------------------- */
//https://www.binarytides.com/multiple-socket-connections-fdset-select-linux/
        for (int i = 0; i < MAXCLIENTS; i++) { //iterate through the clients
            sd = allClients[i].getSock(); //check the socket
            std::string clientName = allClients[i].getName();

                if (FD_ISSET(sd, &readfds)) {//if there is activity here.
                    std::cout << "ACTIVITY ON socket location : " << i << std::endl;
//TODO: Here you can decide what to do with incoming data.
                    //check if activity is for closing the socket
//                    memset(&buffer[0], 0, sizeof(buffer)); // clear buffer to free the stream
                    int valread = read(sd, buffer, BUFFERSIZE); //check to see. 0 means close socket.
                    inStr = ""; //convert to string for easy branching below.
                    int k = 0;
                    //convert to string.
                    while (buffer[k] != '\n') {
                        std::cout << "K";
                        inStr += buffer[k];
                        k++;
                    }
                    inStr += "\n";
                    std::cout << "WE RECEIVED THE MESS : " << inStr << valread;

/*Close sock*/  if (valread == 0) {
                        //Close the socket and mark as 0 in list for reuse
                        std::cout << "[CLOSE SOCKET] : " << i << std::endl;
                        loggedInUsers--; //decrement counter.
                        if (allClients[i].isInLobby()) {
                            int theLobby = allClients[i].getLobbyIndex();
                            allClients[i].exitLobby(); //user must exit the lobby
                            int a = listOfLobbies[theLobby].exitLobby(allClients[i]); //exit the lobby
                            if (a == 0) { //lobby was empty and erased, decrement counter
                                lobbiesUsed--;
                            }
                        }
                        close(sd);
                        allClients[i].eraseClient(); //reset to default values
                    }

                    if (allClients[i].inGame) {
                        toSend = listOfLobbies[allClients[i].getLobbyIndex()].addToScoreandReturnplaces(inStr, allClients[i]);
                        send(sd, toSend.data(), toSend.length(), 0);
                    } else {

                        if (buffer[0] == '!') {
                            if (buffer[1] == 't') {
                                if (sd == replyTimers[i].getClient()) { //if timer is running. stop.
                                    toSend = replyTimers[i].getTimeStr();
                                    timeClients emptyContainer; //creates empty object
                                    replyTimers[i] = emptyContainer;
                                    send(sd, toSend.data(), toSend.length(), 0);
                                } else { //start the timer.
                                    std::cout << "Starting the timer \n";
                                    toSend = "Timing how long it takes you to reply! Reply with \"!time\" to get time\n";
                                    send(sd, toSend.data(), toSend.length(), 0);
                                    startTimeReply(i);
                                }
/*C lobby*/         } else if (inStr.at(1) == 'c') {
                                if (allClients[i].isInLobby()) { //User is already in lobby
                                    toSend = "You are currently in another lobby. Please leave by typing !qlobby\n";
                                    send(sd, toSend.data(), toSend.length(), 0); //send the data
                                } else {
                                    std::cout << "Making the lobby" << std::endl;
                                    if (lobbiesUsed < MAXLOBBIES) { //If space, make the lobby
                                        //Get lobby name
                                        std::string lobbyName;
                                        for (int i = 8; i < inStr.length() - 1; i++) {
                                            lobbyName += buffer[i];
                                        }
                                        if (isLobbyNameOriginal(lobbyName)) { //is name original
                                            int a = nextFreeLobby(); //get next free lobby
                                            if (a != -1) { //There is a lobby ready
                                                //create new lobby;
                                                lobbies lobby(allClients[i], lobbyName);
                                                listOfLobbies[a] = lobby; //add to list of lobbies
                                                allClients[i].joinLobbyIndex(a);
                                                lobbiesUsed++;
                                                toSend = "Lobby made with name " + lobbyName + "!\n";
                                                send(sd, toSend.data(), toSend.length(), 0);
                                            }
                                        } else {
                                            toSend = "That lobby name is taken. Please try another name.\n";
                                            send(sd, toSend.data(), toSend.length(), 0);
                                        }
                                    } else { //no space
                                        toSend = "All lobbies are full... Try again later.\n";
                                        send(sd, toSend.data(), toSend.length(), 0);
                                    }
                                }
/*J lobby*/         } else if (inStr.at(1) == 'j') {
                                if (allClients[i].isInLobby()) {
                                    toSend = "You are currently in another lobby. Please leave by typing !qlobby\n";
                                    send(sd, toSend.data(), toSend.length(), 0); //send the data
                                } else {
                                    std::cout << "Putting user in Lobby\n";
                                    //get the name
                                    std::string lobbyName;
                                    for (int i = 8; i < inStr.length() - 1; i++) {
                                        lobbyName += buffer[i];
                                    }
                                    if (lobbyName == "") { //make sure he put in a name
                                        toSend = "Please enter a lobby name to join.\n";
                                        send(sd, toSend.data(), toSend.length(), 0); //send the data
                                    } else {
                                        //look for lobby
                                        for (int k = 0; k < MAXLOBBIES; k++) {
                                            if (listOfLobbies[k].getLobbyName() == lobbyName) {
                                                if (listOfLobbies[k].joinLobby(allClients[i])) {
                                                    allClients[i].joinLobbyIndex(k);
                                                };
                                                break;
                                            }
                                            if (k == MAXLOBBIES - 1) {
                                                toSend = "There is no lobby by that name\n";
                                                send(sd, toSend.data(), toSend.length(), 0); //send the data

                                            }
                                        }
                                    }
                                }
/*quit lbby*/       } else if (inStr.at(1) == 'q') {
                                if (allClients[i].isInLobby()) {
                                    int theLobby = allClients[i].getLobbyIndex();
                                    allClients[i].exitLobby(); //user must exit the lobby/////////////////////////
                                    int a = listOfLobbies[theLobby].exitLobby(allClients[i]); //exit the lobby
                                    if (a == 0) { //lobby was empty and erased, decrement counter
                                        lobbiesUsed--;
                                        allClients[i].inGame = false;
                                    }

                                } else {
                                    toSend = "You are not in a lobby\n";
                                    send(sd, toSend.data(), toSend.length(), 0);
                                }
/*what lobby*/      } else if (inStr.at(1) == 'w') {
                                /*who lobby*/       if (inStr.at(2) == 'h') {
                                    if (allClients[i].isInLobby()) {
                                        listOfLobbies[allClients[i].getLobbyIndex()].sendLobbyList(
                                                allClients[i].getSock());
                                        //Sorry :/ had to once.
                                    } else {
                                        toSend = "You are not in a lobby.\n";
                                        send(sd, toSend.data(), toSend.length(), 0);
                                    }
                                } else {
                                    std::cout << "Sending list of lobbies\n";
                                    toSend = "List of lobbies is : ";
                                    if (lobbiesUsed > 0) {
                                        for (int i = 0; i < MAXLOBBIES; i++) {
                                            if (listOfLobbies[i].getLobbyName() != "") {
                                                toSend += listOfLobbies[i].getLobbyName() + ", ";
                                            }
                                        }
                                        toSend += "\n";
                                        std::cout << "List of lobbies send is : " << toSend;
                                    } else {
                                        toSend = "There are no lobbies currently open! You can open one with '!clobby <lobbyName>'\n";
                                    }
                                    send(sd, toSend.data(), toSend.length(), 0);
                                }
/*mssg lobby*/      } else if (inStr.at(1) == 'm') {
                                if (allClients[i].isInLobby()) {
                                    std::string theMessage = "";
                                    for (int i = 8; i < inStr.length(); i++) { //include \n
                                        theMessage += buffer[i];
                                    }

                                    theMessage = "LobbyMessage from " + allClients[i].getName() + " " + theMessage;
                                    int lobbyPlace = allClients[i].getLobbyIndex();
                                    listOfLobbies[lobbyPlace].sendMessage(allClients[i].getSock(), theMessage);
                                    toSend = "Lobby SEND-OK\n";
                                } else {
                                    toSend = "You are not in a lobby.\n";
                                }
                                send(sd, toSend.data(), toSend.length(), 0); //send the result to user.

/*strt lobby*/      } else if (inStr.at(1) == 's') { //TODO : add start game here
                                if (allClients[i].isInLobby()) {
                                    std::string groupLeader = listOfLobbies[allClients[i].getLobbyIndex()].whoLeader();
                                    if (groupLeader == clientName) {
                                        int wordLength = 0;
                                        for (int i = 8; i < inStr.length() && inStr.at(i) != '\n'; i++) { //include \n
                                            int a = inStr.at(i);
                                            if (a > 47 && a < 58) {//48 - 57
                                                wordLength = wordLength * 10;
                                                wordLength += (a - 48); //get int value, not char value
                                            } else {
                                                wordLength = -1;
                                                break;
                                            }
                                        }
                                        if (wordLength > 0) {
                                            std::cout << "Word length chosen is: " << wordLength << std::endl;
                                            //Begin the game
                                            listOfLobbies[allClients[i].getLobbyIndex()].beginGame(wordLength);
                                        } else { //INVALID WORD LENGTH. SEND AGAIN.
                                            toSend = "Please use numbers greater than 0 for word length\n";
                                            send(sd, toSend.data(), toSend.length(), 0); //report error
                                        }
                                    } else {
                                        toSend = "You must be the leader to start the game. The leader is " + groupLeader + "\n";
                                        send(sd, toSend.data(), toSend.length(), 0); //send the result to user.
                                        std::cout << "User wanted to start game but was not leader\n";
                                    }
                                } else {
                                    toSend = "You are not in a lobby. Create one to start a game.\n";
                                    send(sd, toSend.data(), toSend.length(), 0); //send the result to user.
                                }
/*BAD RQST*/        } else { //BAD request.
                                toSend = "Sorry. Please try sending that again.\n"; //bad body response
                                send(sd, toSend.data(), toSend.length(), 0);
                            }
                        } else {
                            if (buffer[0] == 'S') {
                                std::cout << "Message to send!\n";
                                if (forwardMessage(i) == 0) {
                                    toSend = "SEND-OK\n";
                                } else {
                                    toSend = "UNKNOWN\n";
                                }
                                send(sd, toSend.data(), toSend.length(), 0);
/*WHO*/             } else if (buffer[0] == 'W') {
                                toSend = createLoggedUserString(); //Create the string
                                send(sd, toSend.data(), toSend.length(), 0);
/*test_time*/       } else { //BAD request.
                                toSend = "Sorry. Please try sending that again.\n"; //bad body response
                                send(sd, toSend.data(), toSend.length(), 0);

                            }
                        }
                        //TODO : Add lobby functionality and game stuff. above else ofc.
                        //memset(&buffer[0], 0, sizeof(buffer)); // clear buffer to free the stream
                }
            }
        }

    }//End while loop

}//close function


int main() {
    try {
        runServer();
    } catch (std::runtime_error & except){
        std::cout<< "[ERROR] : " << except.what() << std::endl; //print error message
    }



    return 0;
}
