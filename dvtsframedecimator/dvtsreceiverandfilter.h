/***************************************************************************
                          dvtsreceiverandfilter.h  -  description
                             -------------------
    begin                : Fri May 21 2004
    copyright            : (C) 2004 by Sebastien Ardon
    email                : ardon@unsw.edu.au          w
    $Id: dvtsreceiverandfilter.h,v 1.5 2004/06/23 02:14:47 seb Exp $
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef DVTSRECEIVERANDFILTER_H
#define DVTSRECEIVERANDFILTER_H

#include <difblocksource.h>

typedef u_int32_t quadlet_t;

extern "C" {
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "RTP.h"
#include <assert.h>
}

/**
  *
  * This subclass of DvtsReceiver also performs some filtering operation
  * such as dropping audio or video data. We discard at the receiver
  * for performance reasons: since we parse each block, we might as well
  * filter here.
  *
  * @author Sebastien Ardon
  *
  */

class DvtsReceiverAndFilter : public DifBlockSource  {
public: 
   DvtsReceiverAndFilter(Preferences *p);
	~DvtsReceiverAndFilter();

   bool init();
   void receiverLoopIterate();

   
protected:
   bool prepareSocket();
   void error(char *s);
   int processPacket(size_t len, quadlet_t *data,bool newFrame);

private:
   bool discardAudio;
   int frameDecimationRatio;
   bool discardVideo;
   int frameCpt;
   bool skipCurrentFrame;

   int portNumber;
   bool isMulticast;
   struct in_addr groupAddress;
   struct in_addr groupInterfaceAddress;
   int pkt_count;
   int packetLoss;
   bool quit;
   int socketFd;
   unsigned int droppedFrames;
   int iDefaultPort;
   unsigned long currentTimestamp;
   unsigned int currentSeq;
   bool printPacketLoss;
   bool debug;

   u_int8 recvbuf[MAX_PACKET_LENGTH];
};

#endif
