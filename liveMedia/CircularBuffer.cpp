#include "CircularBuffer.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>


CircularBuffer* CircularBuffer::createNew(unsigned capacity) {
  return new CircularBuffer(capacity);
}

CircularBuffer::CircularBuffer(unsigned capacity)
  : fReadPos(0)
  , fWritePos(0)
  , fEmpty(true)
  , fClosed(false)
  , fWasDataRead(false)
  , fCapacity(capacity)
  , fBuffer(new unsigned char[capacity]){}

CircularBuffer::~CircularBuffer() {
  delete[] fBuffer;
}

unsigned CircularBuffer::readData(unsigned char* data, unsigned maxSize, struct timeval* writeTime) {
  if (fEmpty || fClosed || maxSize == 0) {
    return 0;
  }
  assert(!fWriteTimes.empty());
  assert(!fWriteSizes.empty());
  if (writeTime != NULL) {
    *writeTime = fWriteTimes.front();
  }
  unsigned toRead = 0;
  if (maxSize < fWriteSizes.front()) {
    toRead = maxSize;
    fWriteSizes.front() -= toRead;
  } else {
    while (!fWriteSizes.empty() && toRead + fWriteSizes.front() <= maxSize) {
      toRead += fWriteSizes.front();
      fWriteSizes.pop_front();
      fWriteTimes.pop_front();
    }
  }

  if (fReadPos < fWritePos) {
    memcpy(data, fBuffer + fReadPos, toRead);
  } else {
    const unsigned tillEnd = std::min(toRead, fCapacity - fReadPos);
    memcpy(data, fBuffer + fReadPos, tillEnd);
    if (tillEnd < toRead) {
      memcpy(data+tillEnd, fBuffer, toRead - tillEnd);
    }
  }
  fReadPos += toRead;
  fReadPos %= fCapacity;
  if (fReadPos == fWritePos) {
    fEmpty = true;
  }
  fWasDataRead = true;

  return toRead;
}

void CircularBuffer::writeData(const unsigned char* data, unsigned size) {
  if (size == 0) {
    return;
  }

  struct timeval writeTime;
  gettimeofday(&writeTime, NULL);

  if (!fWasDataRead) {
    // keep the beginning of the stream intact until it's read by some source.
    // Otherwise a client won't recognize it as a correct h264 stream
    size = std::min(size, fEmpty ? fCapacity : ((fCapacity - fWritePos) % fCapacity));
    if (size > 0) {
      memcpy(fBuffer + fWritePos, data, size);
      fWritePos = (fWritePos + size) % fCapacity;
      if (fEmpty) {
	fWriteSizes.push_back(size);
	fWriteTimes.push_back(writeTime);
      } else {
	fWriteSizes.front() += size;
      }
    }
  } else {
    if (size > fCapacity) {
      data += size - fCapacity;
      size = fCapacity;
    }
    while (fCapacity - availableBytes() < size) {
      fReadPos = (fReadPos + fWriteSizes.front()) % fCapacity;
      assert(!fWriteSizes.empty());
      assert(!fWriteTimes.empty());
      fWriteSizes.pop_front();
      fWriteTimes.pop_front();
      if (fReadPos == fWritePos) {
	fEmpty = true;
      }
    }
    const unsigned availableBefore = availableBytes();
    fWriteSizes.push_back(size);
    fWriteTimes.push_back(writeTime);

    if (fWritePos + size <= fCapacity) {
      memcpy(fBuffer + fWritePos, data, size);
    } else {
      memcpy(fBuffer + fWritePos, data, fCapacity - fWritePos);
      memcpy(fBuffer, data + fCapacity - fWritePos, size - (fCapacity - fWritePos));
    }
    fWritePos += size;
    fWritePos %= fCapacity;

    if (availableBefore + size >= fCapacity) {
      fReadPos = fWritePos;
    }
  }

  fEmpty = false;
  for (std::set<std::pair<onFilledFunc*, void*> >::iterator it = fOnFilled.begin(); it != fOnFilled.end(); ++it) {
    it->first(it->second);
  }
}

void CircularBuffer::close() {
  fClosed = true;
  for (std::set<std::pair<onCloseFunc*, void*> >::iterator it = fOnClosed.begin(); it != fOnClosed.end(); ++it) {
    it->first(it->second);
  }
}

bool CircularBuffer::isClosed() const {
  return fClosed;
}

bool CircularBuffer::isEmpty() const {
  return fEmpty;
}

unsigned CircularBuffer::availableBytes() const {
  if (fEmpty) {
    return 0;
  }
  if (fReadPos < fWritePos) {
    return fWritePos - fReadPos;
  }
  return fCapacity - fReadPos + fWritePos;
}

unsigned CircularBuffer::capacity() const {
  return fCapacity;
}

void CircularBuffer:: addOnFilled(onFilledFunc* onFilled, void* clientData) {
  fOnFilled.insert(std::make_pair(onFilled, clientData));
}

void CircularBuffer::removeOnFilled(onFilledFunc* onFilled, void* clientData) {
  fOnFilled.erase(std::make_pair(onFilled, clientData));
}

void CircularBuffer::addOnClosed(onCloseFunc* onClose, void* clientData) {
  fOnClosed.insert(std::make_pair(onClose, clientData));
}

void CircularBuffer::removeOnClosed(onCloseFunc* onClose, void* clientData) {
  fOnClosed.erase(std::make_pair(onClose, clientData));
}
