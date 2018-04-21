#pragma once
#include <time.h>
#include "../common/common.h"
#include "vehicle.h"
#if SERVER_SIDE_POSITION_CHECK == 1
#define _USE_SERVER_SIDE_FOG_
#endif
// ia brief desription required
typedef enum {
  GetId = 0x1,
  GetTexture = 0x2,
  GetElevation = 0x3,
  PostTexture = 0x4,
  PostElevation = 0x5,
  WorldUpdate = 0x6,
  VehicleUpdate = 0x7,
  PostDisconnect = 0x8,
  GetAudioInfo = 0x9,
  PostAudioInfo = 0x10,
  ChatMessage = 0x11, 
  ChatHistory = 0x12 
} Type;

#ifdef _USE_SERVER_SIDE_FOG_
typedef enum {
  Online = 0x1,
  Offline = 0x2,
  Connecting = 0x3,
  Dropped = 0x4
} Status;
#endif

typedef struct {
  Type type;
  int size;
} PacketHeader;

// sent from client to server to notify its intentions
typedef struct {
  PacketHeader header;
  float translational_force;
  float rotational_force;
} VehicleForces;

// sent from client to server to ask for an id (id=-1)
// sent from server to client to assign an id
typedef struct {
  PacketHeader header;
  int id;
} IdPacket;

// sent from client to server (with type=PostTexture),
// to send its texture
// sent from server to client
//       (with type=PostTexture and id=0) to assign the surface texture
//       (with type=PostElevation and id=0) to assign the surface texture
//       (with type=PostTexture and id>0) to assign the  texture to vehicle id
typedef struct {
  PacketHeader header;
  int id;
  Image* image;
} ImagePacket;

// sent from client to server, in udp to notify the updates
typedef struct {
  PacketHeader header;
  int id;
  float rotational_force;
  float translational_force;
  float x, y, theta;
  struct timeval time;
} VehicleUpdatePacket;

// block of the client updates, id of vehicle
// x,y,theta (read from vehicle id) are position of vehicle
// id is the id of the vehicle
typedef struct {
  int id;
  float x;
  float y;
  float theta;
  float rotational_force;
  float translational_force;
  struct timeval client_update_time, client_creation_time;
} ClientUpdate;

#ifdef _USE_SERVER_SIDE_FOG_
typedef struct {
  int id;
  Status status;
} ClientStatusUpdate;
#endif

typedef struct {
  int id;
  char sender[USERNAME_LEN];
  char text[TEXT_LEN];
  time_t time;  // Used only by server to save the time the message was received
} Message;

typedef struct {
  PacketHeader header;
  Message message;
} MessagePacket;

typedef struct {
  PacketHeader header;
  int num_messages;
  Message* messages;
} MessageHistory;

// server world update, send by server (UDP)
typedef struct {
  PacketHeader header;
  int num_update_vehicles;
#ifdef _USE_SERVER_SIDE_FOG_
  int num_status_vehicles;
#endif
  struct timeval time;
  ClientUpdate* updates;
#ifdef _USE_SERVER_SIDE_FOG_
  ClientStatusUpdate* status_updates;
#endif
} WorldUpdatePacket;

// Send info about a track that should be played by the client
typedef struct {
  PacketHeader header;
  int track_number;
  char loop;
} AudioInfoPacket;

// converts a well formed packet into a string in dest.
// returns the written bytes
// h is the packet to write
int Packet_serialize(char* dest, const PacketHeader* h);

// returns a newly allocated packet read from the buffer
PacketHeader* Packet_deserialize(const char* buffer, int size);

// deletes a packet, freeing memory
void Packet_free(PacketHeader* h);
