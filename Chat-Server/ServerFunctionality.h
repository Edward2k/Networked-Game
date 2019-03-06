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



#define PORT 7070
#define BUFFERSIZE 1024 //1kb
#define MAXCLIENTS 5


struct sockaddr_in address; //struct for machine readable address.
fd_set readfds; //set of socket descriptors.
int master_sock, addrlen, valread, sd, max_sd, check, i, client_sock[MAXCLIENTS], loggedInUsers;
std::string recieved, toSend; //IO to/from clients
char buffer[BUFFERSIZE]; //buffer for incoming messages.
std::string client_name[MAXCLIENTS]; //list of names

void runServer();

int createSocket();

void connectAndLogin();

int forwardMessage(); //sends message from 1 client to another. returns 0 on success. <0 on failure.

std::string createLoggedUserString();