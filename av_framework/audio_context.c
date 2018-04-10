#include "audio_context.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
char setup;
ALuint setupBuffer(char *filename) {
  ALuint buffer = alutCreateBufferFromFile(filename);
  return buffer;
}

ALuint setupSource(ALuint sndBuffer) {
  ALuint source;
  alGenSources(1, &source);
  alSourcei(source, AL_BUFFER, sndBuffer);
  return source;
}

int AudioContext_openDevice(void) {
  if (!setup) {
    setup = 1;
    alutInit(0, NULL);
    alGetError();
  }
}

void AudioContext_closeDevice(void) { alutExit(); }

void AudioContext_init(AudioContext *ac, char *filename) {
  ac->buffer = setupBuffer(filename);
  ac->source = setupSource(ac->buffer);
}

void AudioContext_startTrackLoop(AudioContext *ac) {
  alSourcei(ac->source, AL_LOOPING, AL_TRUE);
  alSourcePlay(ac->source);
}

void AudioContext_startTrack(AudioContext *ac) { alSourcePlay(ac->source); }

void AudioContext_pauseTrack(AudioContext *ac) { alSourcePause(ac->source); }

void AudioContext_stopTrack(AudioContext *ac) { alSourceStop(ac->source); }

void AudioContext_setVolume(AudioContext *ac, float volume) {
  alSourcef(ac->source, AL_GAIN, volume);
}

void AudioContext_free(AudioContext *ac) {
  AudioContext_stopTrack(ac);
  alDeleteSources(1, &ac->source);
  alDeleteBuffers(1, &ac->buffer);
  free(ac);
}
