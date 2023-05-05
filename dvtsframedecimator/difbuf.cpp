/***************************************************************************
                          difbuf.cpp  -  description
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

#include "difbuf.h"

/*
 * some code taken from the Kino project
 * Copyright (C) 2000 Arne Schirmacher <arne@schirmacher.de>
 */

 
/** Code for handling raw DV frame data

    These methods are for handling the raw DV frame data. It contains methods for
    getting info and retrieving the audio data.

    \file frame.cc
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// C++ includes

#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <deque>

using std::setw;
using std::setfill;
using std::deque;
using std::cout;
using std::endl;

// C includes

#include <pthread.h>



// local includes
#include "difbuf.h"
#include "preferences.h"


/** constructor
    parameter is the max number of dif blocks in this buffer
*/

DifBuf::DifBuf(int maxDifCount)
{
    maxBufSize = maxDifCount*80;
    bytesInBuf = 0;
    newFrame = false;
    //memset(data, 0, 144000);
}


DifBuf::~DifBuf()
{

}



void DifBuf::reset(void)
{
   bytesInBuf = 0;
   //memset(data, 0, 144000);
   newFrame = false;
}


bool DifBuf::addDif(unsigned char *d){
  if (IsComplete())
    return false;
  else {
    memcpy(&data[bytesInBuf],d,80);
    bytesInBuf+=80;
  }
  return true;
}
   


bool DifBuf::IsComplete(void)
{
    return (bytesInBuf >= maxBufSize);
}




class MyFramePool : public FramePool
{
	private:
		// The list of available frames
		deque < DifBuf* > buffers;

	public:
		MyFramePool( )
		{
		}

		virtual ~MyFramePool( )
		{
			for ( int i = buffers.size( ); i > 0; --i )
			{
				DifBuf *buf = buffers[ 0 ];
				buffers.pop_front( );
				delete buf;
			}
		}

      DifBuf *createBuffer(int difcount){
         DifBuf *buf = new DifBuf(difcount);
         buffers.push_back(buf);
         return buf;         
      }
      
      
		DifBuf *GetBuffer()
		{
			DifBuf *buf = NULL;
			if ( buffers.begin() != buffers.end() )
			{
				buf = buffers[ 0 ];
				buffers.pop_front( );
			} 

			return buf;
		}

		void DoneWithBuffer(DifBuf *buf )
		{
			buffers.push_back(buf);
		}
};



FramePool *GetFramePool( )
{
	static MyFramePool *pool = new MyFramePool( );
	return pool;
}

