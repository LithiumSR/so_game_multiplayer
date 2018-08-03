#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../av_framework/audio_context.h"
#include "../av_framework/audio_list.h"
#include "../game_framework/protogame_protocol.h"

int main(int argc, char const* argv[]) {
  printf("Opening audio device... ");
  AudioContext_openDevice();
  printf("Done.\n");
  printf("Creating audio list... ");
  AudioListHead* head = (AudioListHead*)malloc(sizeof(AudioListHead));
  AudioList_init(head);
  printf("Done.\n");
  printf("Creating audio context and adding it to the list... ");
  AudioContext* ac = (AudioContext*)malloc(sizeof(AudioContext));
  int res = AudioContext_init(ac, "./resources/sounds/track2.wav", 1, AC_DISPOSABLE);
  if (res == -1) {
    printf("Error!\nFile does not exist or you don't have read permission \n");
    return -1;
  }
  AudioListItem* item = (AudioListItem*)malloc(sizeof(AudioListItem));
  item->audio_context = ac;
  item->next = NULL;
  AudioListItem* el = AudioList_insert(head, item);
  if (el != NULL) printf("Done.\n");
  printf("Starting track... ");
  AudioContext_startTrack(ac);
  printf("Done\n");
  sleep(10);
  printf("Interrupting... ");
  AudioContext_stopTrack(ac);
  printf("Done\n");
  printf("Cleaning resources... ");
  AudioList_cleanExpiredItems(head);
  AudioList_destroy(head);
  AudioContext_closeDevice();
  printf("Done\n");
  return 0;
}
