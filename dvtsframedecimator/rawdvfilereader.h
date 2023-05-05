/***************************************************************************
                          rawdvfilereader.h  -  description
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

#ifndef RAWDVFILEREADER_H
#define RAWDVFILEREADER_H

#include <preferences.h>
#include <difblocksource.h>
#include <difbuf.h>
#include <assert.h>

extern "C" {
  #include <stdio.h>
}


/**
  * DvFrameSource that reads raw DV files.
  * @author Sebastien Ardon
  */

class RawDvFileReader : public DifBlockSource  {
public: 
	RawDvFileReader(Preferences *p);
	~RawDvFileReader();

  bool init();


private:
  const char            *filename;
  FILE            *fp;
  int blockSize;

  void receiverLoopIterate();
};

#endif
