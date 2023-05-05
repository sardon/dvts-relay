/***************************************************************************
                          preferences.cpp  -  description
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

#include "preferences.h"

Preferences *Preferences::instance = NULL;

Preferences &Preferences::getInstance() {
	if ( instance == NULL )
		instance = new Preferences;
	return *instance;
}

Preferences::Preferences(){
  shouldStopNow = false;
  frameRateDivider = 1; 
  inPortNumber = 8000;
  outPortNumber = 8000;
  isMulticast = false;
  inet_aton("127.0.0.1",&outIpAddress);
  frameBufferSize = 10;                      
  debug = false;
  difPerPacket = 17;
  printPacketLoss = false;
  mLoopback = false;
  mTTL = 2;
  inFilename = NULL;
  dropVideo = false;
  dropAudio = false;

  inputIf.s_addr = htonl(INADDR_ANY);
  outputIf.s_addr = htonl(INADDR_ANY);
}

Preferences::~Preferences(){
}
