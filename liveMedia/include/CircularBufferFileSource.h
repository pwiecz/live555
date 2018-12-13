#ifndef _CIRCULAR_BUFFER_FILE_SOURCE_H
#define _CIRCULAR_BUFFER_FILE_SOURCE_H

#ifdef _FRAMED_SOURCE_HH
#include "FramedSource.hh"
#endif

class CircularBufferSource: public FramedSource {
  static CircularBufferSource* createNew(UsageEnvironment& env, size_t bufferSize);

  void doGetNextFrame() override;

  void writeData(char* data, size_t size); 

protected:
  CircularBufferSource(UsageEnvironment& env);
  // called only by createNew()

  virtual ~CircularBufferSource();

  size_t fBufferSize;
  std::unique_ptr<char[]> fBuffer;
};

};

#endif
