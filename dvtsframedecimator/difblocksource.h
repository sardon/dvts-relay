/***************************************************************************
                          difblocksource.h  -  description
                             -------------------
    begin                : Fri Jun 4 2004
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

#ifndef DIFBLOCKSOURCE_H
#define DIFBLOCKSOURCE_H

#include <deque>
#include <iostream>
using std::deque;
using namespace std;

extern "C" {
  #include <stdio.h>
  #include <time.h>
#ifdef USE_THREADS
  #include <pthread.h>
  #include <wait.h>
#endif
#include <assert.h>
}

#include "difbuf.h"
#include "preferences.h"



/**This class is a super class for different DV data sources
  *@author Sebastien Ardon
  */

class DifBlockSource {
public: 
  
  virtual ~DifBlockSource();

#ifdef USE_THREADS
  bool start();
  bool stop();
#endif

  /** Signal that the frame may be recycled */
  void DoneWithBuffer(DifBuf *buf);
  
  /** Get a buffer - usually a packet's worth of dif blocks */
  DifBuf * GetBuffer();

  /** main loop function - subclasses must implement this and return as often as possible */
  virtual void receiverLoopIterate() = 0;

  /** subclass must implement this initializer. Called before thread is started */
  virtual bool init() = 0;

  bool mustSend();
  bool canSend();


protected:
  DifBlockSource(Preferences *prefs);
   
  /** release the frames */
  void freeBuffers();
  bool enqueueCurrentBuffer();
  
  // a pointer to the frame which is currently been processed
  DifBuf    *currentBuf;
  Preferences *prefs ;

private:
  pthread_t               thread;
  pthread_mutex_t         mutex;
  bool                    isStopping;
  int bufSize;
  // a list of empty frames
  deque < DifBuf* > inBuffers;
  // a list of already received frames
  deque < DifBuf* > outBuffers;




   
};

#endif
