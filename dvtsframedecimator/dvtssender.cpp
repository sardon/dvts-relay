/***************************************************************************
                          dvtssender.cpp  -  description
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

#include "dvtssender.h"

u_int8   DvtsSender::buffer[MAX_PACKET_SIZE];


#ifdef USE_THREADS
// C function for multithreading
void *threadFunction2(void *arg){
  DvtsSender *sender = (DvtsSender *)arg;

  while(true){
    sender->senderLoopIterate();
    pthread_testcancel();
  }
  printf("DvSender thread exiting\n");
  return NULL;
}
#endif


DvtsSender::DvtsSender(DifBlockSource *b, Preferences *prefs){
  assert(b!=NULL);
  assert(prefs!=NULL);
   
  bufferSource = b;
  isMulticast = false;

  // copy data in from preferences object
  destAddress.sin_addr.s_addr = prefs->outIpAddress.s_addr;
  mInterfaceAddress.sin_addr.s_addr = prefs->outputIf.s_addr;
  destAddress.sin_port = htons(prefs->outPortNumber);
  difPerPacket = prefs->difPerPacket;
  mTTL = prefs->mTTL;
  debugMode = prefs->debug;
  mLoopback = prefs->mLoopback;
  
    
  wasInit=false;  
}


// Initialisation
bool DvtsSender::init(){

   assert(bufferSource!=NULL);
   assert(wasInit!=true);
   
   if (!IN_MULTICAST(ntohl(destAddress.sin_addr.s_addr))) {
      isMulticast = false;
   } else {
      isMulticast = true;
      if (debugMode)
         cout << "output to multicast destination address" << endl;
   }
   droppedFrames = 0;
   srandom(time(0));

   // map Rtp header to beginning of buffer and init invariants
   rtpHeader = (rtp_hdr_t *)&buffer[0];
   rtpHeader->version = RTP_VERSION;
   rtpHeader->p = 0; // no padding since DVTS RTP packets will always of length multiple of 80
   rtpHeader->pt = 0; // ??? copy from dvsender
   rtpHeader->seq  = htons(random());  // initialise sequence number
   rtpHeader->ts  = htonl(random());
   rtpHeader->ssrc  = htonl(random());

   // map payload
   rtpPayload = &buffer[sizeof(rtp_hdr_t)];

#ifdef USE_THREADS
   pthread_mutex_init(&mutex, NULL);
#endif

  // prepare socket for sending          
  if (prepareSocket()==false){
    return(false);
  }

  wasInit = true;
  return true;
}


DvtsSender::~DvtsSender(){
#ifdef USE_THREADS
  this->stop();
#endif 
  close(socketFd);
}


// create socket, bind it, do multicast stuff
bool DvtsSender::prepareSocket() {
  struct sockaddr_in addr;
  //struct ip_mreq mreq;
  int val;
  u_char ttl = mTTL;
  

  addr.sin_family = AF_INET;
  addr.sin_port = htons(destAddress.sin_port);
  addr.sin_addr.s_addr = INADDR_ANY;

  if ((socketFd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket");
    return(false);
  }

  val = 1;
  if (setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0) {
    perror("reuseaddr setsockopt");
    return(false);
  }

  if (isMulticast){

    // set output multicast interface
    if (setsockopt(socketFd,IPPROTO_IP, IP_MULTICAST_IF,&mInterfaceAddress, sizeof(mInterfaceAddress))<0){
       perror("ip_multicast_if");
       cout << "warning: could not set desired output multicast interface" << endl;
    }


    // set socket send buffer
    val=10976; // should we make this variable
    if (setsockopt(socketFd, SOL_SOCKET, SO_SNDBUF,(char*)&val, sizeof(val))){
     perror("SO_SNDBUF setsockopt");
     cout << "warning: could not set desired outgoing socket buffer" << endl;
    }
        

    // set output multicast loopback option
    if (mLoopback)
       val=1;
    else
       val=0;

    if (setsockopt(socketFd, IPPROTO_IP, IP_MULTICAST_LOOP, &val, sizeof(val))<0) {
       perror("ip_multicast_loop");
       cout << "warning: could not set multicast loopback" << endl;
    }

    // set multicast TTL option
    if (setsockopt(socketFd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0) {
      perror("ip_multicast_ttl");
      cout << "warning: could not set multicast ttl" << endl;
    }  
  }

  
  
  return(true);
}


#ifdef USE_THREADS
// start sending thread
bool DvtsSender::start(){
  bool ret;

  pthread_mutex_lock( &mutex );
  isStopping = false;

  ret = pthread_create(&thread, NULL, threadFunction2, this);

  if (ret != 0) {
    perror("could not start dvts sender thread - pthread_create:");
    pthread_mutex_unlock( &mutex );
    stop();
    return false;
  }
  pthread_mutex_unlock( &mutex );
  return(true);
}


bool DvtsSender::stop(){
  if (isStopping)
    return true;
  cout << "DvtsSender::stop()\n";
  isStopping = true;
  pthread_mutex_lock( &mutex );
  pthread_cancel(thread);
  pthread_join(thread, NULL);
  pthread_mutex_unlock( &mutex );
  return(true);
}
#endif


void DvtsSender::senderLoopIterate() {
#ifdef USE_THREADS
    static struct timespec t = { 0, 0 };
#endif

    DifBuf *buffer;
    
    // try to get one DV frame to send from frame buffer
    if ((buffer = bufferSource->GetBuffer())!=NULL){
        // we have a frame, send it over
        if (buildAndSendPacket(buffer)==false){
           cout << "error: could not send packet" << endl;
        }
        // release this frame and return it to the pool
        bufferSource->DoneWithBuffer(buffer);
    }
#ifdef USE_THREADS
   else {
        nanosleep(&t, NULL);
   }
#endif

}


bool DvtsSender::buildAndSendPacket(DifBuf *dBuf) {
    int ret;
    unsigned int toLen = sizeof(destAddress);
       
    // copy into output buffer
    memcpy(&buffer[sizeof(rtp_hdr_t)],dBuf->data,dBuf->bytesInBuf);
    
    // increase RTP sequence number
    rtpHeader->seq = htons(ntohs(rtpHeader->seq)+1);

    // set RTP timestamp if it's a new frame
    if (dBuf->newFrame == true) {
      rtpHeader->ts = htonl(ntohl(rtpHeader->ts)+3003); 
    }

    // send packet
    if ((ret = sendto(socketFd, (const void *)buffer,
                      dBuf->bytesInBuf+sizeof(rtp_hdr_t), 0,
                      (const struct sockaddr *)&destAddress, toLen)) < 0){
         perror("sendto");
         return false;
    }

    return true;
}
