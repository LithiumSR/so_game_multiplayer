#pragma once
#include "so_game_protocol.h"

// converts a well formed packet into a string in dest.
// returns the written bytes
// h is the packet to write
int get_Client_ID(int socket, char* buf_send, int buf_send_len, char* buf_recv, int buf_recv_len)
Image* get_image_elevation(int socket,int id,char* buf_send, int buf_send_len, char* buf_recv, int buf_recv_len)
Image* get_image_texture(int socket,int id,char* buf_send, int buf_send_len, char* buf_recv, int buf_recv_len)
