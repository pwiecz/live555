#ifndef _CIRCULAR_BUFFER_FILE_SOURCE_H
#define _CIRCULAR_BUFFER_FILE_SOURCE_H

#ifndef _FRAMED_SOURCE_HH
#include "FramedSource.hh"
#endif

#ifndef _CIRCULAR_BUFFER_H
#include "CircularBuffer.h"
#endif


class CircularBufferSource: public FramedSource {
 public:
  static CircularBufferSource* createNew(UsageEnvironment& env, CircularBuffer* buffer);

  virtual void doGetNextFrame();

protected:
  CircularBufferSource(UsageEnvironment& env, CircularBuffer* buffer);

  ~CircularBufferSource();
  // called only by createNew()

  unsigned fLastPlayTime;
  CircularBuffer* fBuffer;
};

#endif
