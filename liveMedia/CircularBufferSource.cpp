#include "CircularBufferSource.h"

void onBufferFilled(void* data) {
  CircularBufferSource* source = (CircularBufferSource*)data;
  if (source->isCurrentlyAwaitingData()) {
    source->doGetNextFrame();
  }
}

void onBufferClosed(void* data) {
  CircularBufferSource* source = (CircularBufferSource*)data;
  if (source->isCurrentlyAwaitingData()) {
    source->handleClosure();
  }
}

CircularBufferSource* CircularBufferSource::createNew(UsageEnvironment& env, CircularBuffer* buffer) {
  return new CircularBufferSource(env, buffer);
}

CircularBufferSource::CircularBufferSource(UsageEnvironment& env, CircularBuffer* buffer)
  : FramedSource(env)
  , fBuffer(buffer) {
  fBuffer->addOnFilled(&onBufferFilled, this);
  fBuffer->addOnClosed(&onBufferClosed, this);
}

CircularBufferSource::~CircularBufferSource() {
  fBuffer->removeOnFilled(&onBufferFilled, this);
  fBuffer->removeOnClosed(&onBufferClosed, this);
}

void CircularBufferSource::doGetNextFrame() {
  //  if (fMaxSize < fBuffer->capacity() && fMaxSize > fBuffer->availableBytes()) {
  //    return;
  //  }
  if (fBuffer->isClosed()) {
    handleClosure();
    return;
  }

  if (!fBuffer->isEmpty()) {
    fFrameSize = fBuffer->readData(fTo, fMaxSize, &fPresentationTime);
    // We don't know a specific play time duration for this data,
    // so just record the current time as being the 'presentation time':
    //    gettimeofday(&fPresentationTime, NULL);
    FramedSource::afterGetting(this);
  }
}
