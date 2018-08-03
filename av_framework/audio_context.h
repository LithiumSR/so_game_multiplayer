#pragma once
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>
#include "../game_framework/protogame_protocol.h"

typedef enum {
  AC_PERSISTENT = 0x1,
  AC_DISPOSABLE = 0x2,
} CleanupFlag;

typedef struct AudioContext {
  ALuint buffer;
  ALuint source;
  float volume;
  char loop;
  CleanupFlag cflags;
  char* filename;
} AudioContext;

int AudioContext_openDevice(void);
void AudioContext_closeDevice(void);
int AudioContext_init(AudioContext *ac, char *filename, char loop, CleanupFlag flag);
void AudioContext_startTrack(AudioContext *ac);
void AudioContext_stopTrack(AudioContext *ac);
void AudioContext_free(AudioContext *ac);
void AudioContext_setVolume(AudioContext *ac, float volume);
void AudioContext_pauseTrack(AudioContext *ac);
void AudioContext_setCleanupFlag(AudioContext *ac, CleanupFlag flag);
ALenum AudioContext_getStatus(AudioContext *ac);