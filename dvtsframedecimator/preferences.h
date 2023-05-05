/***************************************************************************
                          preferences.h  -  description
                             -------------------
    begin                : Wed Jan 8 2003
    copyright            : (C) 2003 by Sebastien Ardon
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

#ifndef PREFERENCES_H
#define PREFERENCES_H

extern "C" {
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

}

/**Keep track of prefs and options
  *@author Sebastien Ardon
  */

class Preferences {
private:
	static Preferences *instance;

protected:
    Preferences();

public:
   static Preferences &getInstance();
   int frameRateDivider;
   int frameBufferSize;
   bool isMulticast;
   struct in_addr inIpAddress;
   struct in_addr outIpAddress;
   char *inFilename;
   int inPortNumber;
   int outPortNumber;
   bool printPacketLoss;
   int difPerPacket;
   bool dropVideo;
   bool dropAudio;

   // multicast stuff
   bool mLoopback;
   int mTTL;
   struct in_addr inputIf;
   struct in_addr outputIf;
   
   bool shouldStopNow;
   bool debug;
	~Preferences();
};

#endif
