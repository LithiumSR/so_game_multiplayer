#pragma once
#include "so_game_protocol.h"

// converts a well formed packet into a string in dest.
// returns the written bytes
// h is the packet to write
int get_client_ID(int socket);
Image* get_image_elevation(int socket,int id);
Image* get_image_texture(int socket,int id);
Image* send_vehicle_texture(int socket, int id);
Image* get_vehicle_texture(int socket, int id);
