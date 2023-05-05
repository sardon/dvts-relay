/***************************************************************************
                          difbuf.h  -  description
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

#ifndef DIFBUF_H
#define DIFBUF_H


/**Simple buffer class to hold DV DIF blocks
  *@author Sebastien Ardon
  */
#include <time.h>
#include <string>
using std::string;


class DifBuf {

public:
    DifBuf(int maxDifCount);
    ~DifBuf();

    void reset();
    bool addDif(unsigned char *data);
    bool IsComplete(void);

    int bytesInBuf;
    unsigned char data[144000];
    bool newFrame;
    int maxBufSize;

};


class FramePool
{
	public:
      virtual DifBuf *createBuffer(int difcount)=0;
		virtual DifBuf *GetBuffer () = 0;
		virtual void DoneWithBuffer( DifBuf * ) = 0;
};

extern FramePool *GetFramePool( );

#endif

