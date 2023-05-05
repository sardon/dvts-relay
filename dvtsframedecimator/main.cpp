 /***************************************************************************
                          main.cpp  -  description
                             -------------------
    begin                : Mon Jan  5 11:20:04 EST 2004
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
using namespace std;

extern "C" {
#include <wait.h>
#include <stdlib.h>
#include <signal.h>
}


#include "difblocksource.h"
#include "dvtssender.h"
#include "dvtsreceiverandfilter.h"
#include "rawdvfilereader.h"
#include "preferences.h"

#define PAL_WIDTH 720
#define PAL_HEIGHT 576

DifBlockSource *globalDvSource = NULL;
DvtsSender *sender = NULL;
bool mustStop = false;


// get default values
Preferences *prefs = &Preferences::getInstance();

void cleanup(){
#ifdef USE_THREADS
  if (globalDvSource!=NULL) globalDvSource->stop();
  if (sender !=NULL) sender->stop();
#endif

  if (globalDvSource!=NULL) delete(globalDvSource);
  if (sender !=NULL) delete(sender);

  exit(0);
}

void _sigint (int sig) {
  mustStop = true;
  cleanup();
  exit(0*sig);
}
                                      

void printParameters(){
  // print parameters
  cout << "DvtsFrameDecimator $Revision: 1.6 $ started" << endl;
  if (prefs->inFilename!=NULL)
     cout << "Relaying from " << prefs->inFilename << ":" << prefs->inPortNumber;
  else
     cout << "Relaying from " << inet_ntoa(prefs->inIpAddress) << ":" << prefs->inPortNumber;
  cout << " to " << inet_ntoa(prefs->outIpAddress) << ":" << prefs->outPortNumber << endl;
  cout << "Frame decimation ratio: " << prefs->frameRateDivider << endl;
  if (prefs->dropVideo) cout << ", dropping video" << endl;
  if (prefs->dropAudio) cout << ", dropping audio" << endl;
  cout << "Outgoing packet size: " << prefs->difPerPacket*80+20 << endl;
  cout << "Internal packet buffer size: " << prefs->frameBufferSize << endl;
}



void show_usage(char *s){
  cout << endl;
  cout << s << " $Revision: 1.6 $" << endl;
  cout << "usage:  " << s << " <options>" << endl;
  cout << "Options are:"<<endl;

  cout << "-I <ip address>     ip address to send to (compulsory option), " << endl;
  cout << "                    this can be a multicast address" << endl;
  cout << "-P <port number>    destination port number (default " << prefs->outPortNumber << ")" << endl;
  cout << "-p <port number>    port number to listen to (default " << prefs->inPortNumber << ")" << endl;
  cout << "-i <ip address>     ip address to listen on "<< endl;
  cout << "-r <ratio>          frame rate divider (default " << prefs->frameRateDivider << ")" << endl;
  cout << "-f <filename>       testing: do not listen, read a file instead (raw dv)" << endl;
  cout << "-a                  drop the audio" << endl;
  cout << "                    (can't do when reading from file)" << endl;
  cout << "-v                  drop the video"<<endl;
  cout << "                    (can't do when reading from file)" << endl;
  cout << "-b <buffer size>    internal buffer size, in number of outgoing packets" << endl;
  cout << "                    (default " << prefs->frameBufferSize << ") This introduces delay" << endl;
  cout << "-d <dif count>      number of dif blocks per packet (default " << prefs->difPerPacket << ", max 1000)" <<endl ;
  cout << "                    outgoing RTP payload size will be count*80 bytes " << endl;
  cout << "-m <iface address>  incoming multicast interface ip" << endl;
  cout << "-M <iface address>  outgoing multicast interface ip" << endl;
  cout << "-L                  turn on multicast loopback mode for outgoing traffic" << endl;
  cout << "-D                  Debug mode (mostly print buffer levels) " << endl;
  cout << "-l                  print packet loss information" << endl;
  cout << " Notes:" << endl;
  cout << "  - this will work for PAL video ONLY. Unpredictable behaviour if not PAL" << endl;
  cout << "  - input or output IP addresses can be multicast  " << endl;
  cout << "    (the corresponding groups will be joined and left)" << endl;
  cout << "    this means that you could use this software as a " << endl;
  cout << "    multicast<->unicast reflector !)" << endl;
  cout << "  - no RTCP sender-report" << endl;
  cout << "  - any input/output filename specified will prevail from IP "<< endl;
  cout << "    address specification" << endl;
  cout << endl << endl;
}




int main(int argc, char *argv[])
{
    struct sigaction saction;
//    struct timespec t = { 0, 10000};
//    sigset_t smask;
    int ret;
     // optarg stuff
  extern char *optarg;
  int op;
  bool readFromFile = false;
  bool destSpecified = false;
  DvtsReceiverAndFilter *receiver;
  RawDvFileReader *fileReader;

  if (argc==0){
     show_usage(argv[0]);
     exit(1);
  }
  
  /* getting options */
  while ((op = getopt(argc, argv, "i:I:p:P:r:f:c:d:avb:Dlm:M:LT:")) > 0) {
    switch (op) {
      case 'i':
        // parse input IP address
        if ((ret=inet_aton(optarg,&(prefs->inIpAddress))) == 0){
          cout << " input on any interface" << endl;
          prefs->inIpAddress.s_addr = 0;
        }
        break;
      case 'I':
        if ((ret=inet_aton(optarg,&(prefs->outIpAddress))) == 0){
          cerr << "invalid output IP address";
          exit(1);
        }
        destSpecified = true;
        break;
      case 'p':
         prefs->inPortNumber = atoi(optarg);
         if ((prefs->inPortNumber < 1)||(prefs->inPortNumber>65534)){
            cerr << "invalid input port number\n";
            show_usage(argv[0]);
            exit(1);
         }
         break;
      case 'P':
         prefs->outPortNumber = atoi(optarg);
         if ((prefs->outPortNumber < 1)||(prefs->outPortNumber>65534)){
          cerr << "invalid output port number\n";
          show_usage(argv[0]);
          exit(1);
         }
         break;
      case 'r':
         prefs->frameRateDivider = atoi(optarg);
         if (prefs->outPortNumber < 1) {
          cerr << "invalid frame rate divider (must be greater than 1)n";
          show_usage(argv[0]);
          exit(1);
         }
         break;
      case 'f':
         prefs->inFilename = optarg;
         readFromFile = true;
         break;
      case 'd':
         prefs->difPerPacket = atoi(optarg);
         if ((prefs->difPerPacket<1)&&(prefs->difPerPacket>999)){
            cerr << "invalid dif per packet size, min 1, max 1000" << endl;
            exit(1);
         }
         break;
      case 'a':
         prefs->dropAudio = true;
         break;
      case 'v':
         prefs->dropVideo = true;
         break;
      case 'D':
         prefs->debug = true;
         break;
      case 'l':
         prefs->printPacketLoss = true;
         break;
      case 'L':
         prefs->mLoopback = true;
         break;
      case 'm':
         if ((ret=inet_aton(optarg,&(prefs->inputIf))) == 0){
          cerr << "invalid input multicast interface address " << optarg << endl;
          exit(1);
         }
         break;
      case 'M':
         if ((ret=inet_aton(optarg,&(prefs->outputIf))) == 0){
          cerr << "invalid output multicast interface address " << optarg << endl;
          exit(1);
         }
         break;
      case 'T':
         prefs->mTTL = atoi(optarg);
         if ((prefs->mTTL < 1)||(prefs->mTTL>255)) {
          cerr << "multicast TTL must be between 1 and 255" << endl;
          exit(1);
         }
         break;
      case 'b':
         prefs->frameBufferSize = atoi(optarg);
         if ((prefs->frameBufferSize < 1)||(prefs->frameBufferSize>500)){
          cerr << "invalid frame buffer size (max 500)\n";
          show_usage(argv[0]);
          exit(1);
         }
         break;
      default:
        show_usage(argv[0]);
        exit(1);
        break;
    }
  }

  if (!destSpecified){
     cout << "error: You must specify at least a destination address" << endl;
     show_usage(argv[0]);
     exit(1);
  }


  // Create DvSource and DvSender object
  if (readFromFile){
     if (prefs->dropAudio||prefs->dropVideo){
         cout << "Will not drop audio or video when reading from file, sorry !" <<endl;
         exit(1);
     }                                      
     fileReader = new RawDvFileReader(prefs);
     globalDvSource = fileReader;
  } else {
    receiver = new DvtsReceiverAndFilter(prefs);
    globalDvSource = receiver;
  }

  sender = new DvtsSender(globalDvSource,prefs);

  // Print parameters
  printParameters();   
  
  // initialize
  if (!globalDvSource->init() || !sender->init()){
    cout << "bailing out" << endl;
    return(1);
  }
  
#ifdef USE_THREADS
  globalDvSource->start();
  sender->start();
#endif

   // set signal handler for INT, TERM and KILL
  saction.sa_handler = _sigint;
  saction.sa_flags = SA_RESTART;
  sigemptyset(&saction.sa_mask);
  sigaction (SIGINT, &saction, NULL);
  sigaction (SIGTERM, &saction, NULL);
  sigaction (SIGKILL, &saction, NULL);

   // main loop
  while (!mustStop){
#ifndef USE_THREADS
     if (!globalDvSource->mustSend())
       globalDvSource->receiverLoopIterate();
     if (globalDvSource->canSend())   
      sender->senderLoopIterate();
#else
  sleep(1);
#endif
  }
  return EXIT_SUCCESS;
}

   

