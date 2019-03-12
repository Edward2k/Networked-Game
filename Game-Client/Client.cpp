#include "Client.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string>
#include <iostream>
#include <sstream>

#define BUFSIZE 1024 //1024 bytes buffer for response stream (1 byte/char)
using namespace std;

//Global variables for chatting with server.
char response[BUFSIZE]; //Variable for response using Buffer size declared at head of document
int recLength; //Holds amount of data sent. used in !who.
string userInput;


struct addrinfo hints, *infoptr; //must be global. Declare struct
//https://www.youtube.com/watch?v=NIqYwXcUdn0
void Client::tick() {

    string theLine;

    if (stdinBuffer.hasLine()) {
        theLine = stdinBuffer.readLine();
        char *message = new char[theLine.size()]; //Make buffer for the message to send
        copy(theLine.begin(), theLine.end(), message); //Add string of characterized to buffer

        //Send message to server
        int SendLength = send(sock, message, strlen(message), 0);
        //cout << "Sent : " << SendLength << endl;
        delete[] message; //free the buffer
    }

    if (socketBuffer.hasLine()) {
        theLine = socketBuffer.readLine();
cout << "We recieved : " << theLine << endl;
        if (theLine[0] == 'W' && theLine[1] == 'H') { //for WHO-OK
            int numUsers = 0; //holds number of users.
            int recLength = theLine.size() + 1;
            cout << "[SYSTEM] The users currently logged in are: " << endl << "\t[" << numUsers << "]";
            for (int i = 7; i < recLength && theLine[i] != '\0'; i++) { //start from first name
                if (theLine[i] == 44) { //44 is ASCII for ','
                    numUsers++;
                    cout << endl << "\t[" << numUsers << ']'; // end with newline and tab
                } else {
                    cout << theLine[i];
                }
            }
            cout << endl;
        } else if (theLine[0] == 'D') { //for delivery
            cout << theLine;

        } else if (theLine[0] == 'S') { //for AKN on send ('SEND-OK')
            cout << theLine;
        } else if (theLine[0] == 'U') {
            cout << "The user is not currently logged in. Try again later." << endl;
        } else {
            cout << "[SERVER] : " << theLine << endl;
        }

    }
}

int Client::readFromStdin() {
    int start;
    string userInput, messageToUser, targetUser;
    getline(cin, userInput);

    if (userInput != "!quit") {
        if (userInput == "!who") { //send message
            userInput = "WHO";

        } else if (userInput[0] == '@') { //Check if first 4 letters are SEND
            //Get target user
            for (start = 1; userInput[start] != 32; start++) {
                targetUser += userInput[start];
            }
            //Get message to user (after space)
            for (start = start + 1; start < userInput.size(); start++) {
                messageToUser += userInput[start];
            }
            if (messageToUser.size() <= 0) {
                cout << "Message needs to be at least 1 character" << endl;
                exit(-1);
            }

            userInput = "SEND " + targetUser + " " + messageToUser; //formulate

        }

        //Now the new properly formatted string are in
        char *messageToStd = new char[userInput.size() + 1]; //Make buffer for the message to send
        copy(userInput.begin(), userInput.end(), messageToStd); //Add string of characterized to buffer
        messageToStd[userInput.size()] = '\n'; //newline byte to show end of string in char sequence

        stdinBuffer.writeChars(messageToStd, userInput.size() + 1);

        delete[] messageToStd;

        return 0;
    }
    return -1; //EXIT
} //reads from standard inout, properly formulates it and then puts into stdinBuffer to send

int Client::readFromSocket() {
    memset(&response[0], 0, sizeof(response)); // clear buffer to free the stream and prevent too much being written
    recLength = recv(sock, response, BUFSIZE, 0);
    socketBuffer.writeChars(response, recLength);
    //cout << "Recieved : " << recLength << endl;

    return 0;
} //reads from socket, gets message and puts in socketBuffer

void Client::closeSocket() { //close the socket
    close(sock);
}

//Assignment 1
void Client::createSocketAndLogIn() {

    //Get info about server
    hints.ai_family = AF_INET; // AF_INET means IPv4 only addresses

    //This integer will find if it is a valid address
    //use 127.0.0.1:6969 for my server.
    int result = getaddrinfo("127.0.0.1", "7070", &hints, &infoptr);

    //if result is anything but 0 (success), print the error.
    if (result) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(result));
        exit(1);
    }
    struct addrinfo *p;
    char host[256];

    //This will make the address inputed (IP or DNS) into machine readbale.
    //Will also cleanly print the code onto the print
    for (p = infoptr; p != NULL; p = p->ai_next) {
        getnameinfo(p->ai_addr, p->ai_addrlen, host, sizeof(host), NULL, 0, NI_NUMERICHOST);
    }
    cout << "Server is valid." << endl;

    /*_______________________TIME TO LOGIN and CONNECT_________________________*/

    //initialize and declare variables.
    bool loggedIn = 0; //login token to see if user is logged in and connected to server
    string busyResponse = "IN-USE\n"; //String with busy response from server
    string serverFullResponse = "BUSY\n"; //string with server full response from server
    string badRequest = "BAD-RQST-BODY\n";
    string badHeader = "BAD-RQST-HDR\n";

    while (loggedIn == 0) { //loop until accepted

        //Initiate the Socket
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == INVALID_SOCKET) { //check to see if valid socket.
            cout << "[SYSTEM] Could not create a socket! Adios!" << endl;
            exit(1);
        }

        //connect to a sever
        int success = connect(sock, infoptr->ai_addr, infoptr->ai_addrlen); //connect
        if (success != 0) { //Check connection
            cout << "[SYSTEM] Failed to connect! Quitting. " << endl;
            exit(-1); //exit with error code
        } else { //connection succesful Login

            cout << "[SYSTEM] What would you like your username to be?   " << endl;
            cout << "[YOU] : ";
            getline(cin, userInput); //go up to \n (not white space)

            userInput = "HELLO-FROM " + userInput + "\n"; //Make the string.

            //Send and get data. Data is stored in Response.
            char *message = new char[userInput.size() + 1]; //Make buffer for the message to send
            copy(userInput.begin(), userInput.end(), message); //Add string of characterized to buffer
            message[userInput.size()] = '\0'; //Zero bit to show end of string in char sequence

            //Send message to server
            send(sock, message, strlen(message), 0);
            delete[] message; //free the buffer

            //recieve message
            memset(&response[0], 0,
                   sizeof(response)); // clear buffer to free the stream and prevent too much being written
            recv(sock, response, BUFSIZE, 0);

            //Check response of server to see if accepted.
            if (response == busyResponse) {
                cout << "[SYSTEM] This name is already taken." << endl;
            } else if (response == serverFullResponse) {
                cout << "[SYSTEM] The server is full. Please try again later." << endl;
                exit(-1); //terminate session
            } else if (response == badRequest || response == badHeader) {
                cout << "[SYSTEM] A fatal error has occured. Aborting." << endl;
                exit(-1);
            } else {
                cout << "[SERVER] : " << response;
                loggedIn = 1; //User is now logged in, you can continue to chat.
            }
        } // end if big else branch on succesful connection.
    } //end of while loop (login session)

    freeaddrinfo(infoptr);//free memory space from addrinfo struct

    /*____________________Chat session succefully created.________________________*/
    cout << "[SYSTEM] : Welcome to the chat server! you can exit at anytime by typing '!quit'." << endl
         << "\t\t - Ask who is logged in using !who" << endl
         << "\t\t - Send message in the form @<username> <message>" << endl
         << "\t\t - Use a test-timer typing !test" << endl;

    //now tick() will run.

}

