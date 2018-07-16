#include "audio_list.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void AudioList_init(AudioListHead* head) {
  head->first = NULL;
  head->size = 0;
}

AudioListItem* AudioList_find_by_context(AudioListHead* head,
                                         AudioContext* ac) {
  if (head == NULL) return NULL;
  AudioListItem* tmp = head->first;
  while (tmp != NULL) {
    if (tmp->audio_context == ac)
      return tmp;
    else
      tmp = tmp->next;
  }
  return NULL;
}

AudioListItem* AudioList_insert(AudioListHead* head, AudioListItem* item) {
  if (head == NULL) return NULL;
  AudioListItem* track = head->first;
  item->next = track;
  head->first = item;
  head->size++;
  return item;
}

AudioListItem* AudioList_detach(AudioListHead* head, AudioListItem* item) {
  if (head == NULL || item == NULL) return NULL;
  AudioListItem* track = head->first;
  if (track == item) {
    head->first = track->next;
    head->size--;
    return track;
  }
  while (track != NULL) {
    if (track->next == item) {
      track->next = item->next;
      head->size--;
      return item;
    } else
      track = track->next;
  }
  return NULL;
}

void AudioList_destroy(AudioListHead* head) {
  if (head == NULL) return;
  AudioListItem* track = head->first;
  while (track != NULL) {
    AudioListItem* tmp = AudioList_detach(head, track);
    track = track->next;
    if (tmp != NULL) {
      AudioContext_free(tmp->audio_context);
      free(tmp);
    }
  }
  free(head);
}

void AudioList_setVolume(AudioListHead* head, float volume) {
  if (head == NULL) return;
  AudioListItem* track = head->first;
  while (track != NULL) {
    AudioContext_setVolume(track->audio_context, volume);
    track = track->next;
  }
}

void AudioList_cleanExpiredItems(AudioListHead* head) {
  if (head == NULL) return;
  AudioListItem* track = head->first;
  while (track != NULL) {
    if (track->audio_context!=NULL && track->audio_context->cflags==AC_PERSISTENT) {
      track = track->next;
      continue;
    }
    AudioListItem* tmp = track;
    track = track->next;
    ALenum state;
    if (tmp->audio_context != NULL) {
      alGetSourcei(tmp->audio_context->source, AL_SOURCE_STATE, &state);
      if (state != AL_STOPPED) continue;
    }
    AudioListItem* dt = AudioList_detach(head, tmp);
    if (dt == tmp) {
      AudioContext_free(dt->audio_context);
      free(dt);
    }
  }
}
