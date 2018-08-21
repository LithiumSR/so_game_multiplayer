#pragma once
#include <time.h>
#include "../common/common.h"
#include "vehicle.h"
// These are the types of action that are covered by the protocol
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
  ChatHistory = 0x12,
  ChatAuth = 0x13
} PacketType;

typedef enum { Effect = 0x1, Track = 0x2 } MusicType;

typedef enum { Hello = 0x1, Goodbye = 0x2, Text = 0x3 } MessageType;

typedef enum {
  Online = 0x1,
  Offline = 0x2,
  Connecting = 0x3,
  Dropped = 0x4
} Status;

typedef struct {
  PacketType type;
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

// contains client's status
typedef struct {
  int id;
  Status status;
} ClientStatusUpdate;

typedef struct {
  int id;
  char text[TEXT_LEN];
  time_t time;  // Used only by server to save the time the message was received
  MessageType type;
} Message;

typedef struct {
  int id;
  char text[TEXT_LEN];
  char sender[USERNAME_LEN];
  time_t time;  // Used only by server to save the time the message was received
  MessageType type;
} MessageBroadcast;

typedef struct {
  PacketHeader header;
  Message message;
} MessagePacket;

typedef struct {
  PacketHeader header;
  int num_messages;
  MessageBroadcast* messages;
} MessageHistoryPacket;

// server world update, send by server (UDP)
typedef struct {
  PacketHeader header;
  int num_update_vehicles;
  int num_status_vehicles;
  struct timeval time;
  ClientUpdate* updates;
  ClientStatusUpdate* status_updates;
} WorldUpdatePacket;

typedef struct {
  PacketHeader header;
  char username[USERNAME_LEN];
  int id;
} MessageAuthPacket;

// Send info about a track that should be played by the client
typedef struct {
  PacketHeader header;
  int track_number;
  char loop;
  MusicType type;
} AudioInfoPacket;

// converts a well formed packet into a string in dest.
// returns the written bytes
// h is the packet to write
int Packet_serialize(char* dest, const PacketHeader* h);

// returns a newly allocated packet read from the buffer
PacketHeader* Packet_deserialize(const char* buffer, int size);

// deletes a packet, freeing memory
void Packet_free(PacketHeader* h);
