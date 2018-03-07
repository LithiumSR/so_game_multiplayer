#pragma once
#include "so_game_protocol.h"

// converts a well formed packet into a string in dest.
// returns the written bytes
// h is the packet to write
int getID(int socket);
Image* getElevationMap(int socket);
Image* getTextureMap(int socket);
int sendVehicleTexture(int socket,Image* texture, int id);
int sendGoodbye(int socket,int id);
