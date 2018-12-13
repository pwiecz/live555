#ifndef _CIRCULAR_BUFFER_H
#define _CIRCULAR_BUFFER_H

#include <deque>
#include <set>
#include <utility>

#include <sys/time.h>


class CircularBuffer {
 public:
  static CircularBuffer* createNew(unsigned bufferSize);

  void close();

  void writeData(const unsigned char* data, unsigned size); 

  unsigned readData(unsigned char* data, unsigned maxSize, struct timeval* writeTime);

  bool isClosed() const;

  bool isEmpty() const;

  unsigned availableBytes() const;

  unsigned capacity() const;

  typedef void (onFilledFunc)(void* clientData);
  void addOnFilled(onFilledFunc* onFilled, void* clientData);
  void removeOnFilled(onFilledFunc* onFilled, void* clientData);

  typedef void (onCloseFunc)(void* clientData);
  void addOnClosed(onCloseFunc* onClose, void* clientData);
  void removeOnClosed(onCloseFunc* onClose, void* clientData);
		     
protected:
  CircularBuffer(unsigned bufferSize);
  // called only by createNew()

  ~CircularBuffer();

  unsigned fReadPos;
  unsigned fWritePos;
  bool fEmpty;
  bool fClosed;
  bool fWasDataRead;
  const unsigned fCapacity;
  unsigned char* const fBuffer;
  std::deque<struct timeval> fWriteTimes;
  std::deque<unsigned> fWriteSizes;

  std::set<std::pair<onFilledFunc*, void*> > fOnFilled;
  std::set<std::pair<onCloseFunc*, void*> > fOnClosed;
};

#endif
