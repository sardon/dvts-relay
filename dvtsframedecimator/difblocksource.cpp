/***************************************************************************
                          difblocksource.cpp  -  description
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

#include "difblocksource.h"

#ifdef USE_THREADS
// C function for multithreading
void *threadFunction(void *arg){
  DifBlockSource *src = (DifBlockSource *)arg;

  while(true){
    src->receiverLoopIterate();
    pthread_testcancel();
  }
  printf("Frame source thread exiting\n");
  return NULL;
}

#endif



DifBlockSource::DifBlockSource(Preferences *p){
  DifBuf *buffer;
   
  assert(p!=NULL);

  prefs = p;
  bufSize = prefs->frameBufferSize;

  /* Create empty frames and put them in our inBuffers queue */
  for (int i = 0; i < bufSize; ++i) {
    buffer = GetFramePool( )->createBuffer(prefs->difPerPacket);
    inBuffers.push_back(buffer);
  }

  currentBuf = NULL;

#ifdef USE_THREADS  
  /* Initialize mutexes */
  pthread_mutex_init(&mutex, NULL);
#endif
}

DifBlockSource::~DifBlockSource(){

}


/** release the frames */
void DifBlockSource::freeBuffers(){
  DifBuf *buffer;

  //free up our frames queues
  for (int i = inBuffers.size(); i > 0; --i) {
    buffer = inBuffers[0];
    inBuffers.pop_front();
    GetFramePool( )->DoneWithBuffer(buffer);
  }
  for (int i = outBuffers.size(); i > 0; --i) {
    buffer = outBuffers[0];
    outBuffers.pop_front();
    GetFramePool( )->DoneWithBuffer(buffer);
  }
  if (currentBuf != NULL) {
    GetFramePool( )->DoneWithBuffer(currentBuf);
    currentBuf = NULL;
  }
}



/**
  * This method is called by the sending
  * thread to get the next buffer to send
  * in the output buffer
  *
  * Note that we yield the sending thread if the input buffer
  * size is larger than the output buffer size.
  *
  */
DifBuf* DifBlockSource::GetBuffer()
{
    DifBuf *buffer = NULL;
#ifdef USE_THREADS
    struct timespec t = { 0, 0 }; //nanosleep 0ns is equivalent to yield in linux
    pthread_mutex_lock(&mutex);
#endif

    if (outBuffers.size() > 0) {
        buffer = outBuffers[0];
        outBuffers.pop_front();
    }
    pthread_mutex_unlock(&mutex);

#ifdef USE_THREADS
    // give more time to receiving thread if output buffer is getting low
    //while (outBuffers.size()<(prefs->frameBufferSize/2)){
        nanosleep(&t, NULL);
    //}
#endif
      
    return buffer;
}

/**
  * This method is called by the sending thread once it has
  * finished with a DifBuf. The method put the difbuf
  * back into the inBuffer pool of difbug
  * Put back a frame to the queue of available frames

  */
void DifBlockSource::DoneWithBuffer(DifBuf* buffer) {

#ifdef USE_THREADS
   bool needToSleep = false;
   pthread_mutex_lock(&mutex);
#endif

   buffer->reset();
   inBuffers.push_back(buffer);
   
#ifdef USE_THREADS
   pthread_mutex_unlock(&mutex);
#endif

}

#ifdef USE_THREADS
// start receiving threads
bool DifBlockSource::start(){
  bool ret;

  pthread_mutex_lock( &mutex );
  isStopping = false;

  ret = pthread_create(&thread, NULL, threadFunction, this);

  if (ret != 0) {
    perror("could not start receiver thread - pthread_create:");
    pthread_mutex_unlock( &mutex );
    stop();
    return false;
  }
  pthread_mutex_unlock( &mutex );
  return(true);
}

bool DifBlockSource::stop(){
  if (isStopping)
    return true;
  isStopping = true;
  pthread_mutex_lock( &mutex );
  cout << "DifBlockSource::stop()\n";
  pthread_cancel(thread);
  pthread_join(thread, NULL);
  pthread_mutex_unlock( &mutex );
  return(true);
}
#endif

/**
  * This method is called by subclasses,i.e receiving threads
  * to enqueue a buffer into the outBuffer
  * as it is ready to be sent
  * the sending thread should pick it up using
  * the GetBuffer method
  */
bool DifBlockSource::enqueueCurrentBuffer(){
  int droppedFrames = 0;
  bool result = true;

#ifdef USE_THREADS
  struct timespec t = { 0, 0 };
  bool needToSleep = false;  
  pthread_mutex_lock(&mutex);
#endif

  
  // save current frame in out queue
  if (currentBuf != NULL) {
    outBuffers.push_back(currentBuf);
    currentBuf = NULL;
  }

  // get new frame from in queue
  if (inBuffers.size() > 0) {
      currentBuf = inBuffers.front();
      currentBuf->reset();
      inBuffers.pop_front();
  } else {
      // queue overflow: no more frame in input queue
      currentBuf = NULL;
      droppedFrames++;
      cout << "reader < # dropped buffer: " << droppedFrames << endl;
      result = false;
  }

   if (prefs->debug)
         cout << "# available buffers: " << inBuffers.size() << ", # output buffers: " << outBuffers.size() << endl;

#ifdef USE_THREADS
   pthread_mutex_unlock(&mutex);         
   // give more time to sending thread if input buffer is getting low
   //while (inBuffers.size()<(prefs->frameBufferSize/4)){
         nanosleep(&t, NULL);
   //}
#endif

  return result;
}


bool DifBlockSource::mustSend(){
   return(outBuffers.size()>(prefs->frameBufferSize*0.8));
}

bool DifBlockSource::canSend(){
   return(outBuffers.size()>0);
}


