/***************************************************************************
                          dvtssender.h  -  description
                             -------------------
    begin                : Mon Jan 5 2004
    copyright            : (C) 2004 by Sebastien Ardon
    email                : ardon@unsw.edu.au
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef DVTSSENDER_H
#define DVTSSENDER_H


#include <deque>
#include <iostream>
using std::deque;
using namespace std;


extern "C" {
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#ifdef USE_THREADS
#include <pthread.h>
#include <wait.h>
#endif
#include <assert.h>
#include "RTP.h"
}

#include "difbuf.h"
#include "preferences.h"
#include "difblocksource.h"

#define MIN(X,Y)    ( (X) < (Y) ? (X) : (Y) )
#define MAX_PACKET_SIZE 8021


/**
  * DvtsSender class
  *
  * The multithreaded code comes UNTESTED  
  *
  * @author Sebastien Ardon
  */

class DvtsSender {
public:
    DvtsSender(DifBlockSource *s, Preferences *p);
    ~DvtsSender();

#ifdef USE_THREADS
    bool start();
    bool stop();
#endif


    // open and bind socket
    bool init();

    // main loop
    void senderLoopIterate();

private:
     DifBlockSource *bufferSource;
     bool isMulticast;
     struct sockaddr_in destAddress;
     int mTTL;
     struct sockaddr_in mInterfaceAddress;
     bool mLoopback;
     bool debugMode;
     
     int pkt_count;
     int socketFd;
     int difPerPacket;
     unsigned int droppedFrames;
     bool wasInit;
     

     // packet stuff
     static u_int8 buffer[MAX_PACKET_SIZE];
     rtp_hdr_t  *rtpHeader;
     u_int8 *rtpPayload;

#ifdef USE_THREADS
     // thread stuff
     pthread_t               thread;
     pthread_mutex_t         mutex;
     //bool quit;
     bool isStopping;
#endif

     bool prepareSocket();
     bool buildAndSendPacket(DifBuf *buffer);
};

#endif
