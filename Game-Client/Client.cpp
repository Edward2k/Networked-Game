#include "Client.h"


#include <stdio.h>
#ifdef _WIN32
#include "ansicolor-w32.h"
#endif



#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#define BUFSIZE 1024 //1024 bytes buffer for response stream (1 byte/char)
using namespace std;

#define foreground_magenta "\033[32m"
#define ASCII_ESC 27

//Global variables for chatting with server.
char response[BUFSIZE]; //Variable for response using Buffer size declared at head of document
int recLength; //Holds amount of data sent. used in !who.
string userInput;

bool inGame = false;
vector<string> gameWords;
int wordCount = 0; //counts what word we are on.

struct addrinfo hints, *infoptr; //must be global. Declare struct
//https://www.youtube.com/watch?v=NIqYwXcUdn0



vector<string> VectorizeStringWords(string theWords) {
    string word;
    vector<string> result;

    for (int i = 0; i < theWords.length()-1; i++) { //exclude \n
        if (theWords.at(i) == ',') {
            i++;
            result.push_back(word);
            word = ""; //empty
        } else {
            word += theWords.at(i);
        }
    }
    return result;
}


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
        printf("%c[32m", ASCII_ESC);
        theLine = socketBuffer.readLine();
        //cout << "We recieved : " << theLine << endl;

        if (inGame) {
            if (theLine.at(theLine.length() - 2) == '$') { // if we have the special symbol at end
                gameWords = VectorizeStringWords(theLine); //Create into a vector
                printf("%c[31m", ASCII_ESC);
                cout << "WORD: \t\t" << gameWords[wordCount] << endl; //print next game word
                wordCount++; //increment;
            } else if (theLine.at(0) == '$') { //END GAME
                inGame = false;
                printf ( "%c[2J", ASCII_ESC ); //Clear the screen
                printf ( "%c[H", ASCII_ESC );
                cout << "GAME has ended!!!! Congrats!\n"; //TODO : format this
                gameWords = {}; //empty vector
                wordCount = 0;
            } else {
                cout << "[SERVER] : " << theLine << "NEXT WORD : ";
                cout <<  "WORD: \t\t" << gameWords[wordCount] << endl; //print next game word
                wordCount++; //increment;
            }

        } else {
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
            } else if (theLine[0] == 'D') { //for delivery //Make this neater
                printf("%c[35m", ASCII_ESC); //Send in green
                cout << theLine;

            } else if (theLine[0] == 'S') { //for AKN on send ('SEND-OK')
                cout << theLine;
            } else if (theLine[0] == 'U') {
                cout << "The user is not currently logged in. Try again later." << endl;
            } else if (theLine == "GAME-START\n") {
                inGame = true;
                printf ( "%c[2J", ASCII_ESC ); //Clear the screen
                printf ( "%c[H", ASCII_ESC );
                cout << "THE GAME IS NOW STARTING!\n";

            } else {
                cout << "\a[SERVER] : " << theLine;
            }
        }
        printf("%c[m", ASCII_ESC); //remove bold. (turn off character attributes)
    }
}


int Client::readFromStdin() {
    int start;
    string userInput, messageToUser, targetUser;
    printf ( "%c[m", ASCII_ESC ); //remove bold. (turn off character attributes)
    getline(cin, userInput);
    printf ( "%c[1A", ASCII_ESC ); //move up one line
    printf ( "%c[2K", ASCII_ESC ); //Clear the entire line
    printf ( "%c[1m", ASCII_ESC ); // print in bold

    if (inGame == false) {
        cout << "\b[YOU] : " << userInput << endl; //TODO : find way to fix last input to put [you] before
    } else {
        cout << "\b[YOUR RESPONSE] : " << userInput << endl;
    }
    printf ( "%c[m", ASCII_ESC ); //remove bold. (turn off character attributes)

    if (userInput != "!quit") {
        if (inGame) {
            if (userInput.length() == 0) {
                cout << "SEND SOMETHING!\n";
                userInput = "###\n";
            } else {
                string countWord = to_string(wordCount);
                userInput += countWord;
                userInput += "\n";
                cout << "SENDING : " << userInput;
            }
        } else {
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
                    return 0; //leave function
                }

                userInput = "SEND " + targetUser + " " + messageToUser; //formulate

            }
        }

        //Now the new properly formatted string are in
        char *messageToStd = new char[userInput.size() + 1]; //Make buffer for the message to send
        copy(userInput.begin(), userInput.end(), messageToStd); //Add string of characterized to buffer
        messageToStd[userInput.size()] = '\n'; //newline byte to show end of string in char sequence

        stdinBuffer.writeChars(messageToStd, userInput.size() + 1);
//        cout << "SENT : " << userInput << "|||" << endl;
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
    printf ( "%c[2J", ASCII_ESC ); //Clear the screen
    printf ( "%c[H", ASCII_ESC );
    printf ( "%c[32m", ASCII_ESC ); //Send in green

    //Get info about server
    hints.ai_family = AF_INET; // AF_INET means IPv4 only addresses
    std::string ipNum, portNum = "";
    //This integer will find if it is a valid address
    //use 127.0.0.1:6969 for my server.
    std::string IPnum, PortNum;
    cout << "IP : "; cin >> IPnum;
    if (IPnum == "XXX") {
        IPnum = "127.0.0.1";
        PortNum = "7070";
    } else {
        cout << "PORT : "; cin >> PortNum;
    }


    int result = getaddrinfo(IPnum.data(), PortNum.data(), &hints, &infoptr);

    //if result is anything but 0 (success), print the error.
    if (result) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(result));
        exit(1);
    }

    cout << "Welcome to Type racing!!!!" << endl;
    cout << "The objective of the game is to type as many words as fast as you can. Try to avoid typos and any mistakes as those seriously" <<
         "bring down your score. To start, create a lobby, wait for some friends to join and then start the game. " << endl << "HAVE FUN!" << endl;

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
            printf ( "%c[m", ASCII_ESC ); //remove bold. (turn off character attributes)

            cout << "[YOU] : ";
            cin.clear();
            getline(cin, userInput); //go up to \n (not white space)
            printf ( "%c[32m", ASCII_ESC ); //Send in green

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
    cout << "\a\a\a\a\a\a\a\a[SYSTEM] : Welcome to the chat server! you can exit at anytime by typing '!quit'." << endl
         << "\t - Ask who is logged in using !who" << endl
         << "\t - Send message in the form @<username> <message>" << endl
         << "\t - Use a test-timer typing !time" << endl
         << "\t - For lobby managment : " << endl
         << "\t\t - To create a lobby : !clobby <name>" << endl
         << "\t\t - To join a lobby : !jlobby <name>" << endl
        << "\t\t - To list all open lobbies !wlobby" << endl
        << "\t\t - To list everyone in your lobby !whlobby" << endl
        << "\t\t - To message the lobby !mlobby <message body>" << endl;









    printf ( "%c[m", ASCII_ESC ); //remove bold. (turn off character attributes)


    //now tick() will run.

}

