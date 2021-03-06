#pragma once
#include "../av_framework/audio_context.h"
#include "../game_framework/protogame_protocol.h"
// converts a well formed packet into a string in dest.
// returns the written bytes
// h is the packet to write
int getID(int socket);
Image *getElevationMap(int socket);
Image *getTextureMap(int socket);
int sendVehicleTexture(int socket, Image *texture, int id);
int sendGoodbye(int socket, int id);
Image *getVehicleTexture(int socket, int id);
AudioContext *getAudioContext(int socket);
int joinChat(int socket_desc, int id, char *username);