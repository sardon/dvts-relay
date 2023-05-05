/***************************************************************************
                          rawdvfilereader.cpp  -  description
                             -------------------
    begin                : Mon Jan 20 2003
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

#include "rawdvfilereader.h"
#include <stdio.h>

RawDvFileReader::RawDvFileReader(Preferences *p):DifBlockSource(p)
{
 filename = p->inFilename;
}


RawDvFileReader::~RawDvFileReader(){
  fclose(fp);
  freeBuffers();
}


/** start the file reader. */
bool RawDvFileReader::init(){
  //cout << "opening "<<filename << endl;
  if ((fp = fopen(filename,"r")) == NULL){
    perror("could not open file");
    return false;
  }     
  return true;
}


void RawDvFileReader::receiverLoopIterate() {
  int n;
  struct  timespec t = { 0,0 }; // this is wrong- should be calculated dynamically - testing only

  assert(fp!=NULL);

  enqueueCurrentBuffer();

  if (currentBuf!=NULL){ 
    if ((n = fread((char *)currentBuf->data, currentBuf->maxBufSize, 1, fp))==0) {
      if (feof(fp)){
         cout << "Rewind file" << endl;
         rewind(fp);
       } else {
         perror("reading from file: fread");
       }
    }
    currentBuf->bytesInBuf += (n*currentBuf->maxBufSize);
  }

  nanosleep(&t,NULL);
  
}
