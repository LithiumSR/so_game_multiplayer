#pragma once
#include "../av_framework/audio_context.h"
#include "../game_framework/so_game_protocol.h"
// converts a well formed packet into a string in dest.
// returns the written bytes
// h is the packet to write
int getID(int socket);
Image* getElevationMap(int socket);
Image* getTextureMap(int socket);
int sendVehicleTexture(int socket, Image* texture, int id);
int sendGoodbye(int socket, int socket_udp, int id, int messaging_enabled,
                char* username, struct sockaddr_in server_addr);
Image* getVehicleTexture(int socket, int id);
AudioContext* getAudioContext(int socket);
