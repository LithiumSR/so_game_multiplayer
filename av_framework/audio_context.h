#pragma once
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>
#include "../game_framework/protogame_protocol.h"

typedef struct AudioContext {
  ALuint buffer;
  ALuint source;
  float volume;
  char loop;
} AudioContext;

int AudioContext_openDevice(void);
void AudioContext_closeDevice(void);
int AudioContext_init(AudioContext *ac, char *filename, char loop);
void AudioContext_startTrack(AudioContext *ac);
void AudioContext_stopTrack(AudioContext *ac);
void AudioContext_free(AudioContext *ac);
void AudioContext_setVolume(AudioContext *ac, float volume);
void AudioContext_pauseTrack(AudioContext *ac);
