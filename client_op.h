#pragma once
#include "so_game_protocol.h"

// converts a well formed packet into a string in dest.
// returns the written bytes
// h is the packet to write
int send_request_type(int socket,Type tp);
IdPacket* get_next_IdPacket(int socket);
ImagePacket* get_next_ImagePacket(int socket_desc);
int send_client_Texture(int socket, Image* my_texture, int id);
