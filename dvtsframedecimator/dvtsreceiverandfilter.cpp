/***************************************************************************
                          dvtsreceiverandfilter.cpp  -  description
                             -------------------
    begin                : Fri May 21 2004
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

#include "dvtsreceiverandfilter.h"


 
DvtsReceiverAndFilter::DvtsReceiverAndFilter(Preferences *p):DifBlockSource(p){
   assert(p!=NULL);

   groupAddress.s_addr = prefs->inIpAddress.s_addr;
   portNumber = prefs->inPortNumber;
   groupInterfaceAddress.s_addr = prefs->inputIf.s_addr;
   discardVideo = p->dropVideo;
   discardAudio = p->dropAudio;
   frameDecimationRatio = p->frameRateDivider;
   printPacketLoss = p->printPacketLoss;
   debug = p->debug;
   
   prefs = p;
   frameCpt = 0;
   skipCurrentFrame=false;
   droppedFrames = 0;
   packetLoss = 0;
   isMulticast = false;
   droppedFrames = 0;
}
  


   
DvtsReceiverAndFilter::~DvtsReceiverAndFilter(){
#ifdef USE_THREADS
  this->stop();
#endif
  close(socketFd)  ;
}



/* initialise the receiver */
bool DvtsReceiverAndFilter::init(){
   
  if (prepareSocket()==false){
    return(false);
  }
  return true;
}



/** create UDP socket and bind it to port */
bool DvtsReceiverAndFilter::prepareSocket() {
  struct sockaddr_in addr;
  struct ip_mreq mreq;
  int rxbufsize = 160000;
  int yes=1;

  addr.sin_family = AF_INET;
  addr.sin_port = htons(portNumber);
  addr.sin_addr.s_addr = INADDR_ANY;

  if ((socketFd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket");
    return(false);
  }

  if (setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
    perror("reuseaddr setsockopt");
    return(false);
  }

  if (setsockopt(socketFd, SOL_SOCKET, SO_RCVBUF,(char*)&rxbufsize, sizeof(rxbufsize))){
     perror("SO_RCVBUF setsockopt");
     return false;
  }

  if (bind(socketFd,(struct sockaddr *)&addr,sizeof(struct sockaddr))<0){
    perror("bind");
    return(false);
  }


  if (!IN_MULTICAST(ntohl(groupAddress.s_addr))) {
       isMulticast = false;
  } else {
       isMulticast = true;
       if (debug)
         cout << "Receiver: using multicast address" << endl;
  }

  if (isMulticast){     
    /* join the multicast group */
    memset((void *)&mreq,0,sizeof(struct ip_mreq));
    mreq.imr_multiaddr.s_addr = groupAddress.s_addr;
    mreq.imr_interface.s_addr = groupInterfaceAddress.s_addr;

    if (setsockopt(socketFd, IPPROTO_IP, IP_ADD_MEMBERSHIP,(char *)&mreq, sizeof(mreq)) < 0) {
      perror("ip_add_membership");
      return(false);
    }

    if (prefs->debug)
     cout << "receiver joined multicast group" << endl;

  }
  return(true);
}



/** No descriptions */
void DvtsReceiverAndFilter::error(char *s){
  fprintf(stderr,"%s\n",s);
}

void DvtsReceiverAndFilter::receiverLoopIterate() {
    struct sockaddr_storage from;
    size_t length = sizeof(from);
    quadlet_t *data;
    size_t   dataLen;
    int ret;
    rtp_hdr_t *header;
    bool newFrame = false;
   
    // Receive one UDP frame
    if ((ret = recvfrom(socketFd,(char *)recvbuf, sizeof(recvbuf), 0,(struct sockaddr *)&from, &length)) < 0){
        perror("recvfrom");
    }

    header = (rtp_hdr_t *)recvbuf;  

    // check for packet loss
    if (ntohs(header->seq)!=currentSeq+1){
      packetLoss++;
      if (printPacketLoss)
         cout << packetLoss << " packet loss (previous was " << currentSeq << " new is " <<ntohs(header->seq) <<")" << endl;
    }


    currentSeq = ntohs(header->seq);


    // check if this packet is the start of a new frame
    if (header->ts != currentTimestamp){
       newFrame = true;
       currentTimestamp = header->ts;
    }
     

    // get rid of RTP header and pass the data to next method
    data = (quadlet_t *)(recvbuf + sizeof(rtp_hdr_t));
    dataLen = ret - sizeof(rtp_hdr_t);

    processPacket(dataLen,data,newFrame);
}


int DvtsReceiverAndFilter::processPacket(size_t length, quadlet_t *data,bool newFrame)
{
    unsigned char *p;
    int section_type,dif_sequence,dif_block;
    unsigned int i;

    /* skip short packets */
    if (length <= 16){
       if (printPacketLoss)
         cout << "WARNING: short packet received" << endl;
       return 0;
    }

    // check packet length
    if ((length % 80) > 0)
        cerr << "WARNING: packet length not multiple of 80 bytes (" << length <<")"<<endl;

    // if this packet is the start of a new frame,
    // enqueue the current frame
    if (newFrame) {
        //cout << "beginning of a new frame "<<  endl;
        enqueueCurrentBuffer();
        currentBuf->newFrame = true;

        // check if we'll skip this new frame
        // note that we only skip video frames, unless
        // audio is to be discarded, in which case
        // we skip everything except header block
        frameCpt = (frameCpt+1) % frameDecimationRatio;
        //frameCpt = 1;
        //cout << "frame coutner: " << frameCpt << endl;
        if (frameCpt!=0) {
           skipCurrentFrame = true;
        } else {
           skipCurrentFrame = false;
        }
    }

    // check if we have a buffer - queue could be full
    if (currentBuf==NULL){
      if (printPacketLoss)
        cout << "WARNING: no buffer avail: one packet dropped" << endl;
      return 0;
    }

    // parse DIF blocks and throw out what we don't want
    for (i=0; i < length/80; i++){
       p = (unsigned char*) & data[i*80/4];
       section_type = p[0] >> 5;           /* section type is in bits 5 - 7 */
       dif_sequence = p[1] >> 4;           /* dif sequence number is in bits 4 - 7 */
       dif_block = p[2];

       switch (section_type) {
                case 0: /* 1 Header block */
                 /* p[3] |= 0x80; // hack to force PAL data */
                 currentBuf->addDif(p);
                 break;

                 case 1: /* 2 Subcode blocks */
                 currentBuf->addDif(p);
                 break;

                 case 2: /* 3 VAUX blocks */
                 currentBuf->addDif(p);
                 break;

                 case 3: /* 9 Audio blocks interleaved with video */
                 if (!discardAudio){
                   currentBuf->addDif(p);
                 }
                 break;

                 case 4: /* 135 Video blocks interleaved with audio */
                 if ((!discardVideo)&&(!skipCurrentFrame)){
                   currentBuf->addDif(p);
                 }
                 break;

                 default: /* we can´t handle any other data */
                 cout << "unknown SCT\n";
                 break;
       }  // switch

       if (currentBuf->IsComplete()){
          enqueueCurrentBuffer();
        } 
    } // for
    return 0;
}


