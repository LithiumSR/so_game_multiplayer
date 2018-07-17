#pragma once
#include <netinet/in.h>
#include <time.h>
#include "../av_framework/image.h"
#include "audio_context.h"
typedef struct AudioListItem {
  struct AudioListItem* next;
  AudioContext* audio_context;
} AudioListItem;

typedef struct AudioListHead {
  AudioListItem* first;
  int size;
} AudioListHead;

void AudioList_init(AudioListHead* head);
AudioListItem* AudioList_findByContext(AudioListHead* head, AudioContext* ac);
AudioListItem* AudioList_find(AudioListHead* head, AudioListItem* item);
AudioListItem* AudioList_findByFilename(AudioListHead* head, char* filename);
AudioListItem* AudioList_insert(AudioListHead* head, AudioListItem* item);
AudioListItem* AudioList_detach(AudioListHead* head, AudioListItem* item);
void AudioList_destroy(AudioListHead* head);
void AudioList_setVolume(AudioListHead* head, float volume);
void AudioList_cleanExpiredItems(AudioListHead* head);
