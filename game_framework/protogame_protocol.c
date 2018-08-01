#include "protogame_protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../av_framework/audio_context.h"
// converts a packet into a (preallocated) buffer
int Packet_serialize(char* dest, const PacketHeader* h) {
  char* dest_end = dest;
  switch (h->type) {
    case PostDisconnect:
    case GetId:
    case GetTexture:
    case GetElevation: {
      const IdPacket* id_packet = (IdPacket*)h;
      memcpy(dest, id_packet, sizeof(IdPacket));
      dest_end += sizeof(IdPacket);
      break;
    }
    case GetAudioInfo:
    case PostAudioInfo: {
      const AudioInfoPacket* id_packet = (AudioInfoPacket*)h;
      memcpy(dest, id_packet, sizeof(AudioInfoPacket));
      dest_end += sizeof(AudioInfoPacket);
      break;
    }
    case ChatAuth: {
      const MessageAuthPacket* mp = (MessageAuthPacket*)h;
      memcpy(dest, mp, sizeof(MessageAuthPacket));
      dest_end += sizeof(MessageAuthPacket);
      break;
    }
    case ChatMessage: {
      const MessagePacket* mp = (MessagePacket*)h;
      memcpy(dest, mp, sizeof(MessagePacket));
      dest_end += sizeof(MessagePacket);
      break;
    }
    case ChatHistory: {
      const MessageHistoryPacket* mh = (MessageHistoryPacket*)h;
      memcpy(dest, mh, sizeof(MessageHistoryPacket));
      dest_end += sizeof(MessageHistoryPacket);
      memcpy(dest_end, mh->messages, mh->num_messages * sizeof(MessageBroadcast));
      dest_end += mh->num_messages * sizeof(MessageBroadcast);
      break;
    }
    case PostTexture:
    case PostElevation: {
      debug_print("cast\n");
      const ImagePacket* img_packet = (ImagePacket*)h;
      debug_print("memcopy\n");
      memcpy(dest, img_packet, sizeof(ImagePacket));
      // the image is invalid, we need to read it from the buffer
      debug_print("forward address\n");
      dest_end += sizeof(ImagePacket);
      debug_print("image serialization");
      dest_end += Image_serialize(img_packet->image, dest_end, 1024 * 1024);
      debug_print("end\n");
      break;
    }
    case WorldUpdate: {
      const WorldUpdatePacket* world_packet = (WorldUpdatePacket*)h;
      memcpy(dest, world_packet, sizeof(WorldUpdatePacket));
      dest_end += sizeof(WorldUpdatePacket);
      memcpy(dest_end, world_packet->updates,
             world_packet->num_update_vehicles * sizeof(ClientUpdate));
      dest_end += world_packet->num_update_vehicles * sizeof(ClientUpdate);
#ifdef _USE_SERVER_SIDE_FOG_
      memcpy(dest_end, world_packet->status_updates,
             world_packet->num_status_vehicles * sizeof(ClientStatusUpdate));
      dest_end +=
          world_packet->num_status_vehicles * sizeof(ClientStatusUpdate);
#endif
      break;
    }
    case VehicleUpdate: {
      VehicleUpdatePacket* vehicle_packet = (VehicleUpdatePacket*)h;
      memcpy(dest, vehicle_packet, sizeof(VehicleUpdatePacket));
      dest_end += sizeof(VehicleUpdatePacket);
      break;
    }
  }

  PacketHeader* dest_header = (PacketHeader*)dest;
  dest_header->size = dest_end - dest;
  return dest_header->size;
}

// reads a packet from a preallocated buffer
PacketHeader* Packet_deserialize(const char* buffer, int size) {
  const PacketHeader* h = (PacketHeader*)buffer;
  switch (h->type) {
    case GetId:
    case GetTexture:
    case PostDisconnect:
    case GetElevation: {
      IdPacket* id_packet = (IdPacket*)malloc(sizeof(IdPacket));
      memcpy(id_packet, buffer, sizeof(IdPacket));
      return (PacketHeader*)id_packet;
    }
    case GetAudioInfo:
    case PostAudioInfo: {
      AudioInfoPacket* audio_packet =
          (AudioInfoPacket*)malloc(sizeof(AudioInfoPacket));
      memcpy(audio_packet, buffer, sizeof(AudioInfoPacket));
      return (PacketHeader*)audio_packet;
    }
    case ChatAuth: {
      MessageAuthPacket* mp = (MessageAuthPacket*)malloc(sizeof(MessageAuthPacket));
      memcpy(mp, buffer, sizeof(MessageAuthPacket));
      return (PacketHeader*)mp;
    }
    case ChatMessage: {
      MessagePacket* mp = (MessagePacket*)malloc(sizeof(MessagePacket));
      memcpy(mp, buffer, sizeof(MessagePacket));
      return (PacketHeader*)mp;
    }
    case ChatHistory: {
      MessageHistoryPacket* mh = (MessageHistoryPacket*)malloc(sizeof(MessageHistoryPacket));
      memcpy(mh, buffer, sizeof(MessageHistoryPacket));
      // we get the number of messages
      mh->messages = (MessageBroadcast*)malloc(mh->num_messages * sizeof(MessageBroadcast));
      buffer += sizeof(MessageHistoryPacket);
      memcpy(mh->messages, buffer, mh->num_messages * sizeof(MessageBroadcast));
      return (PacketHeader*)mh;
    }
    case PostTexture:
    case PostElevation: {
      ImagePacket* img_packet = (ImagePacket*)malloc(sizeof(ImagePacket));
      memcpy(img_packet, buffer, sizeof(ImagePacket));
      // the image is invalid, we need to read it from the buffer
      size -= sizeof(ImagePacket);
      buffer += sizeof(ImagePacket);
      img_packet->image = Image_deserialize(buffer, size);
      if (!img_packet->image) {
        free(img_packet);
        return 0;
      }
      return (PacketHeader*)img_packet;
    }
    case WorldUpdate: {
      WorldUpdatePacket* world_packet =
          (WorldUpdatePacket*)malloc(sizeof(WorldUpdatePacket));
      memcpy(world_packet, buffer, sizeof(WorldUpdatePacket));
      // we get the number of clients
      world_packet->updates = (ClientUpdate*)malloc(world_packet->num_update_vehicles *
                                                    sizeof(ClientUpdate));
#ifdef _USE_SERVER_SIDE_FOG_
      world_packet->status_updates = (ClientStatusUpdate*)malloc(
          world_packet->num_status_vehicles * sizeof(ClientStatusUpdate));
#endif

      buffer += sizeof(WorldUpdatePacket);
      memcpy(world_packet->updates, buffer,
             world_packet->num_update_vehicles * sizeof(ClientUpdate));
      buffer += world_packet->num_update_vehicles * sizeof(ClientUpdate);
#ifdef _USE_SERVER_SIDE_FOG_
      memcpy(world_packet->status_updates, buffer,
             world_packet->num_status_vehicles * sizeof(ClientStatusUpdate));
#endif
      return (PacketHeader*)world_packet;
    }
    case VehicleUpdate: {
      VehicleUpdatePacket* vehicle_packet =
          (VehicleUpdatePacket*)malloc(sizeof(VehicleUpdatePacket));
      memcpy(vehicle_packet, buffer, sizeof(VehicleUpdatePacket));
      return (PacketHeader*)vehicle_packet;
    }
  }
  return 0;
}

void Packet_free(PacketHeader* h) {
  switch (h->type) {
    case GetId:
    case PostDisconnect:
    case GetTexture:
    case GetElevation:
    case VehicleUpdate:
    case ChatAuth:
    case ChatMessage:
    case GetAudioInfo:
    case PostAudioInfo: {
      free(h);
      return;
    }
    case ChatHistory: {
      MessageHistoryPacket* mh = (MessageHistoryPacket*)h;
      if (mh->num_messages) free(mh->messages);
      free(mh);
      return;
    }
    case WorldUpdate: {
      WorldUpdatePacket* world_packet = (WorldUpdatePacket*)h;
      if (world_packet->num_update_vehicles) free(world_packet->updates);
#ifdef _USE_SERVER_SIDE_FOG_
      if (world_packet->num_status_vehicles) free(world_packet->status_updates);
#endif
      free(world_packet);
      return;
    }
    case PostTexture:
    case PostElevation: {
      ImagePacket* img_packet = (ImagePacket*)h;
      if (img_packet->image) {
        Image_free(img_packet->image);
      }
      free(img_packet);
    }
  }
}