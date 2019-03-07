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

#include "ServerFunctionality.h"


int createSocket() {
    //'''struct sockaddr_in address''' //defined in ServerFunctionality.h
    //''fd_set readfds'' // The set of socket descriptors defined in header file
    //

    /*--------------------------------------------------------------------------------------------
     * NOW WE MUST INITIALIZE ALL THE SOCKETS TO 0 SO THEY ARE NOT READ BY THE 'SELECT()' FUNCTION.
     *--------------------------------------------------------------------------------------------- */

    for (int i=0; i < MAXCLIENTS; i++) {
        client_sock[i] = 0; //tells select that this sock is not active.
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

    /*--------------------------------------------------------------------------------------------
    * Check to see if name is available (yes: add to socket and client name. else:: close socket
     * and reply 'Busy\n' to indicate the server is full.
    *--------------------------------------------------------------------------------------------- */
    for (i = 0; i <= MAXCLIENTS; i++) { //use <= in the case its full and go one more
        //Check if name is available
        if (client_name[i] == name) {
            std::cout << "USER HAS CHOSEN INVALID NAME\n";
            toSend = "IN-USE\n";
            int a = send(new_socket, toSend.data(), toSend.length(), 0); //send to socket that is being opened. (new_socket)
            break;
        } else if(client_sock[i] == 0) { //if slot is empty, add to socket list
            client_sock[i] = new_socket;
            std::cout << "[UPDATE][SOCKET] : Placed in socket location : " << i << std::endl;
            loggedInUsers++; //increment number of logged in users.
            //Complete the handshake
            toSend = "HELLO " + name + '\n';
            send(client_sock[i], toSend.data(), toSend.length(), 0);
            client_name[i] = name; //add name to the list of users
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

    fromUser = client_name[i]; //from user

    //Find what socket the client needed is connected on.
    for (k = 0; k <= MAXCLIENTS; k++) {
        if (client_name[k] == targetUser) {
            toSend = "DELIVERY " + fromUser + messageBody + "\n";
            int a = send(client_sock[k], toSend.data(), toSend.length(), 0);//Send message to the target user.
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
        std::cout << "The user at index i is : " << client_name[i] << std::endl;
        if (client_name[i] == "") {
            //Do nothing, skip the empty slot.
        } else {
            if (j == loggedInUsers-1){ // check to see if last or not (for formatting)
                message += client_name[i]; //add last client without comma for formatting in Client side
                break;
            } else {
                message += client_name[i] + ',';
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

    while(true) {
        memset(&buffer[0], 0, sizeof(buffer));
        //clear the socket set
        FD_ZERO(&readfds);

        //add master socket to set
        FD_SET(master_sock, &readfds);
        max_sd = master_sock;

        for (int i=0; i < MAXCLIENTS; i++) {
            sd = client_sock[i];

            //if valid socket descriptor then add to read list
            if(sd > 0) {
                FD_SET(sd, &readfds);
            }

            //This is the highest possible file descriptor number. This is needed for the select() function used to pool
            //multiple users at the same time instead of multithreading.
            if(sd > max_sd) {
                max_sd = sd;
            }

        } //end for loop that initializes descriptors.

        /*--------------------------------------------------------------------------------------------
         * Wait for activity on one of the MAXCLIENTS ports (5). NULL indicates no timeout time.
         *--------------------------------------------------------------------------------------------- */

        int activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
        if (activity < 0 && errno!=EINTR) { //if there is an error: TODO: lookup errno!=EINTR
            throw std::runtime_error("Could not select! Adios!");
        }

        /*--------------------------------------------------------------------------------------------
         * if and only if something happens on the master socket, that means there is activity
         * We must connect and login.
         *--------------------------------------------------------------------------------------------- */

        if (FD_ISSET(master_sock, &readfds)) { //login
            std::cout<<"CONNECTING...\n";
            connectAndLogin();
        } //endif

        /*--------------------------------------------------------------------------------------------
        * anything else, there is some IO operation on another active socket.
        *--------------------------------------------------------------------------------------------- */
//https://www.binarytides.com/multiple-socket-connections-fdset-select-linux/
        for (int i = 0; i < MAXCLIENTS; i++ ) { //iterate through the clients
            sd = client_sock[i]; //check the socket

            if (FD_ISSET( sd , &readfds)) {//if there is activity here.
                std::cout << "ACTIVITY ON socket location : " << i << std::endl;
//TODO: Here you can decide what to do with incomming data.
                //check if activity is for closing the socket
                if ((valread = read( sd , buffer, 1024)) == 0) {
                    //Close the socket and mark as 0 in list for reuse
                    std::cout<< "[CLOSE SOCKET] : " << i << std::endl;
                    client_name[i] = "";//erase the name.
                    loggedInUsers--; //decrement counter.
                    close(sd);
                    client_sock[i] = 0;
                } else if (buffer[0] == 'S'){ //There is a message to send.
                    std::cout << "Message to send!\n";
                    if (forwardMessage(i) == 0) {
                        toSend = "SEND-OK\n";
                    } else {
                        toSend = "UNKNOWN\n";
                    }
                    send(sd, toSend.data(), toSend.length(), 0);
                } else if (buffer[0] == 'W') { //WHO
                    toSend = createLoggedUserString(); //Create the string
                    send(sd, toSend.data(), toSend.length(), 0);
                } else { //BAD request.
                    toSend = "BAD-RQST-BODY\n"; //bad body response
                    send(sd, toSend.data(), toSend.length(), 0);
                }
            }
        }
    }//End while loop

} //close function


int main() {
    try {
        runServer();
    } catch (std::runtime_error & except){
        std::cout<< "[ERROR] : " << except.what() << std::endl; //print error message
    }



    return 0;
}
