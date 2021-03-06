/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// Copyright (c) 1996-2013, Live Networks, Inc.  All rights reserved
// A test program that demonstrates how to stream - via unicast RTP
// - various kinds of file on demand, using a built-in RTSP server.
// main program

#include <signal.h>
#include <unistd.h>

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"

#include "CircularBuffer.h"
//#include "CircularBufferSource.h"
#include "H264VideoStreamServerMediaSubsession.h"
#include "MediaSink.hh"

UsageEnvironment* env;

// To make the second and subsequent client for each stream reuse the same
// input stream as the first client (rather than playing the file from the
// start for each client), change the following "False" to "True":
Boolean reuseFirstSource = True;

// To stream *only* MPEG-1 or 2 video "I" frames
// (e.g., to reduce network bandwidth),
// change the following "False" to "True":
Boolean iFramesOnly = False;

static const int BUFFER_SIZE = 1 << 19; // 512k bytes

volatile char stop = 0;
volatile char quit = 0;

static void signal_handler(int signum) {
  //  if (stop) {
  //    abort();
  //  }
  stop = 1;
  if (signum != SIGUSR1) {
    quit = 1;
  }
}

static void announceStream(RTSPServer* rtspServer, ServerMediaSession* sms,
			   char const* streamName); // fwd

bool hasDataToReadFromStdin() {
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 0;

  fd_set readfds;
  FD_ZERO(&readfds);
  FD_SET(STDIN_FILENO, &readfds);

  return select(1, &readfds, NULL, NULL, &timeout) == 1;
}

void readDataFromStdin(void* userData, int mask) {
  CircularBuffer* buffer = (CircularBuffer*)userData;
  if (stop) {
    buffer->close();
    return;
  }

  if (!hasDataToReadFromStdin()) {
    return;
  }
 
  static unsigned char readBuf[BUFFER_SIZE];
  ssize_t actuallyRead = read(0, readBuf, BUFFER_SIZE);
  if (actuallyRead == 0) {
    buffer->close();
    quit = 1;
    stop = 1;
  } else {
    buffer->writeData(readBuf, actuallyRead);
  }
}

int main(int argc, char** argv) {
  signal(SIGINT, signal_handler);
  signal(SIGHUP, signal_handler);
  signal(SIGUSR1, signal_handler);

  CircularBuffer* buffer = CircularBuffer::createNew(BUFFER_SIZE);

  while (quit == 0) {
    stop = 0;
    buffer->reset();

    // Begin by setting up our usage environment:
    TaskScheduler* scheduler = BasicTaskScheduler::createNew();
    env = BasicUsageEnvironment::createNew(*scheduler);

    // OutPacketBuffer::maxSize = BUFFER_SIZE;

    UserAuthenticationDatabase* authDB = NULL;
#ifdef ACCESS_CONTROL
    // To implement client access control to the RTSP server, do the following:
    authDB = new UserAuthenticationDatabase;
    authDB->addUserRecord("username1", "password1"); // replace these with real strings
    // Repeat the above with each <username>, <password> that you wish to allow
    // access to the server.
#endif

    RTSPServer* rtspServer = NULL;
    while (true) {
      // Create the RTSP server:
      rtspServer = RTSPServer::createNew(*env, 8554, authDB);
      if (rtspServer == NULL) {
	*env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
      } else {
	break;
      }
      sleep(1);
    }


    env->taskScheduler().turnOnBackgroundReadHandling(0, readDataFromStdin, buffer);
    char const* descriptionString = "Session streamed from Sinterit printer";

    // Set up each of the possible streams that can be served by the
    // RTSP server.  Each such stream is implemented using a
    // "ServerMediaSession" object, plus one or more
    // "ServerMediaSubsession" objects for each audio/video substream.

    // A H.264 video elementary stream:
    {
      char const* streamName = "h264";

      ServerMediaSession* sms = ServerMediaSession::createNew(*env, streamName, streamName, descriptionString);

      sms->addSubsession( H264VideoStreamServerMediaSubsession::createNew(*env, buffer, reuseFirstSource));

      rtspServer->addServerMediaSession(sms);

      announceStream(rtspServer, sms, streamName);	// not needed just for better diagnostic
    }

    // Also, attempt to create a HTTP server for RTSP-over-HTTP tunneling.
    // Try first with the default HTTP port (80), and then with the alternative HTTP
    // port numbers (8000 and 8080).

    /*  if (rtspServer->setUpTunnelingOverHTTP(80) || rtspServer->setUpTunnelingOverHTTP(8000) || rtspServer->setUpTunnelingOverHTTP(8080)) {
     *env << "\n(We use port " << rtspServer->httpServerPortNum() << " for optional RTSP-over-HTTP tunneling.)\n";
     } else {
     *env << "\n(RTSP-over-HTTP tunneling is not available.)\n";
     }*/

    env->taskScheduler().doEventLoop(&stop); // does not return
    *env << "Signal received stopping\n";
    
    env->taskScheduler().turnOffBackgroundReadHandling(0);

    Medium::close(rtspServer);

    delete scheduler;
    env->reclaim();
  }

  return 0; // only to prevent compiler warning
}

static void announceStream(RTSPServer* rtspServer, ServerMediaSession* sms,
			   char const* streamName) {
  char* url = rtspServer->rtspURL(sms);
  UsageEnvironment& env = rtspServer->envir();
  env << "\n\"" << streamName << "\" stream, from stdin\n";
  env << "Play this stream using the URL \"" << url << "\"\n";
  delete[] url;
}
