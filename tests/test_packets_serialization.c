#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../game_framework/so_game_protocol.h"
#if SERVER_SIDE_POSITION_CHECK == 1
#define _USE_SERVER_SIDE_FOG_
#endif

int main(int argc, char const* argv[]) {
  // id packet
  printf("allocate an IDPacket\n");
  IdPacket* id_packet = (IdPacket*)malloc(sizeof(IdPacket));

  PacketHeader id_head;
  id_head.type = GetId;

  id_packet->header = id_head;
  id_packet->id = 14;

  printf("build packet with:\ntype\t%d\nsize\t%d\nid\t%d\n",
         id_packet->header.type, id_packet->header.size, id_packet->id);

  printf("serialize!\n");
  char id_packet_buffer[1000000];  // ia how much space does the buffer needs??
  // can I pass the id_packet oppure the header??
  int id_packet_buffer_size =
      Packet_serialize(id_packet_buffer, &id_packet->header);
  printf("bytes written in the buffer: %d\n", id_packet_buffer_size);

  printf("serialized, now deserialize\n");
  IdPacket* deserialized_packet =
      (IdPacket*)Packet_deserialize(id_packet_buffer, id_packet_buffer_size);
  printf("deserialized\n");
  printf("deserialized packet with:\ntype\t%d\nsize\t%d\nid\t%d\n",
         deserialized_packet->header.type, deserialized_packet->header.size,
         deserialized_packet->id);

  printf("delete the packets\n");
  // can I pass the id_packet oppure the header??
  Packet_free(&id_packet->header);
  Packet_free(&deserialized_packet->header);
  printf("done\n");

  // image packet
  printf("\n\nallocate an ImagePacket\n");
  ImagePacket* image_packet = (ImagePacket*)malloc(sizeof(ImagePacket));

  PacketHeader im_head;
  im_head.type = PostTexture;

  printf("loading an image\n");
  Image* im;
  im = Image_load("./resources/images/test.pgm");
  printf("loaded\n");

  image_packet->header = im_head;
  image_packet->image = im;

  Image_save(image_packet->image, "in.pgm");

  printf("image_packet with:\ntype\t%d\nsize\t%d\n", image_packet->header.type,
         image_packet->header.size);

  printf("serialize!\n");
  char image_packet_buffer[1000000];  // ia how much space does the buffer
                                      // needs??
  int image_packet_buffer_size =
      Packet_serialize(image_packet_buffer, &image_packet->header);
  printf("bytes written in the buffer: %d\n", image_packet_buffer_size);

  printf("deserialize\n");
  ImagePacket* deserialized_image_packet = (ImagePacket*)Packet_deserialize(
      image_packet_buffer, image_packet_buffer_size);

  printf("deserialized packet with:\ntype\t%d\nsize\t%d\n",
         deserialized_image_packet->header.type,
         deserialized_image_packet->header.size);

  Image_save(deserialized_image_packet->image, "out.pgm");

  Packet_free(&deserialized_image_packet->header);
  Packet_free(&deserialized_image_packet->header);
  printf("done\n");

  // world
  printf("\n\nallocate an WorldPacket\n");
  ClientUpdate* update_block = (ClientUpdate*)malloc(sizeof(ClientUpdate));
  update_block->id = 10;
  update_block->x = 4.4;
  update_block->y = 6.4;
  update_block->theta = 90;
  WorldUpdatePacket* world_packet =
      (WorldUpdatePacket*)malloc(sizeof(WorldUpdatePacket));
  PacketHeader w_head;
  w_head.type = WorldUpdate;
  world_packet->header = w_head;
  world_packet->num_vehicles = 1;
  world_packet->updates = update_block;

  printf("world_packet with:\ntype\t%d\nsize\t%d\nnum_v\t%d\n",
         world_packet->header.type, world_packet->header.size,
         world_packet->num_vehicles);
  printf("update_block:\nid\t\t%d\n(x,y,theta)\t(%f,%f,%f)\n",
         world_packet->updates->id, world_packet->updates->x,
         world_packet->updates->y, world_packet->updates->theta);

  printf("serialize\n");
  char world_buffer[1000000];
  int world_buffer_size = Packet_serialize(world_buffer, &world_packet->header);
  printf("bytes written in the buffer: %i\n", world_buffer_size);

  printf("deserialize\n");
  WorldUpdatePacket* deserialized_wu_packet =
      (WorldUpdatePacket*)Packet_deserialize(world_buffer, world_buffer_size);

  printf("deserialized packet with:\ntype\t%d\nsize\t%d\nnum_v\t%d\n",
         deserialized_wu_packet->header.type,
         deserialized_wu_packet->header.size,
         deserialized_wu_packet->num_vehicles);
  printf("update_block:\nid\t\t%d\n(x,y,theta)\t(%f,%f,%f)\n",
         deserialized_wu_packet->updates->id,
         deserialized_wu_packet->updates->x, deserialized_wu_packet->updates->y,
         deserialized_wu_packet->updates->theta);
  Packet_free(&world_packet->header);
  Packet_free(&deserialized_wu_packet->header);
  printf("done\n");

  // vehicle
  printf("\n\nallocate a VehicleUpdatePacket\n");
  VehicleUpdatePacket* vehicle_packet =
      (VehicleUpdatePacket*)malloc(sizeof(VehicleUpdatePacket));
  PacketHeader v_head;
  v_head.type = VehicleUpdate;

  vehicle_packet->header = v_head;
  vehicle_packet->id = 24;
  vehicle_packet->rotational_force = 9.0;
  vehicle_packet->translational_force = 9.0;

  printf(
      "vehicle_packet packet "
      "with:\ntype\t%d\nsize\t%d\nid\t%d\ntforce\t%f\nrot\t%f\n",
      vehicle_packet->header.type, vehicle_packet->header.size,
      vehicle_packet->id, vehicle_packet->translational_force,
      vehicle_packet->rotational_force);

  printf("serialize\n");
  char vehicle_buffer[1000000];
  int vehicle_buffer_size =
      Packet_serialize(vehicle_buffer, &vehicle_packet->header);
  printf("bytes written in the buffer: %i\n", vehicle_buffer_size);

  printf("deserialize\n");
  VehicleUpdatePacket* deserialized_vehicle_packet =
      (VehicleUpdatePacket*)Packet_deserialize(vehicle_buffer,
                                               vehicle_buffer_size);

  printf(
      "deserialized packet "
      "with:\ntype\t%d\nsize\t%d\nid\t%d\ntforce\t%f\nrot\t%f\n",
      deserialized_vehicle_packet->header.type,
      deserialized_vehicle_packet->header.size, deserialized_vehicle_packet->id,
      deserialized_vehicle_packet->translational_force,
      deserialized_vehicle_packet->rotational_force);

  Packet_free(&vehicle_packet->header);
  Packet_free(&deserialized_vehicle_packet->header);
  printf("done\n");

  // chat
  printf("\n\nallocate a MessagePacket \n");
  PacketHeader message_header;
  message_header.type = ChatMessage;
  MessagePacket* mp_pckt = (MessagePacket*)malloc(sizeof(MessagePacket));
  mp_pckt->header = message_header;
  char* text = "Hello World";
  strncpy(mp_pckt->message.text, text, TEXT_LEN);
  mp_pckt->message.id = 10;
  mp_pckt->message.type = Text;
  time(&mp_pckt->message.time);
  struct tm* info;
  info = localtime(&mp_pckt->message.time);
  printf(
      "message packet "
      "with:\ntype\t%d\nsize\t%d\nid\t%d\nmessageType\t%d\ntext\t%s\ntime:\t%d:"
      "%d\n",
      mp_pckt->header.type, mp_pckt->header.size, mp_pckt->message.id,
      mp_pckt->message.type, mp_pckt->message.text, info->tm_hour,
      info->tm_min);
  printf("serialize\n");
  char message_buffer[1000000];
  int message_size = Packet_serialize(message_buffer, &mp_pckt->header);
  printf("deserialize\n");
  MessagePacket* deserialized_message_packet =
      (MessagePacket*)Packet_deserialize(message_buffer, message_size);
  info = localtime(&deserialized_message_packet->message.time);
  printf(
      "deserialized message packet "
      "with:\ntype\t%d\nsize\t%d\nid\t%d\nmessageType\t%d\ntext\t%s\ntime:\t%d:"
      "%d\n",
      deserialized_message_packet->header.type,
      deserialized_message_packet->header.size,
      deserialized_message_packet->message.id,
      deserialized_message_packet->message.type,
      deserialized_message_packet->message.text, info->tm_hour, info->tm_min);
  Packet_free(&deserialized_packet->header);
  Packet_free(&mp_pckt->header);

  printf("\n\nallocate a MessageAuth packet \n");
  PacketHeader auth_header;
  auth_header.type = ChatAuth;
  MessageAuth* auth_pckt = (MessageAuth*)malloc(sizeof(MessageAuth));
  auth_pckt->header = auth_header;
  char* username = "username_test";
  strncpy(auth_pckt->username, username, USERNAME_LEN);
  auth_pckt->id = 10;
  printf(
      "messageAuth packet "
      "with:\ntype\t%d\nsize\t%d\nid\t%d\nusername\t%s\n",
      auth_pckt->header.type, auth_pckt->header.size, auth_pckt->id,
      auth_pckt->username);
  printf("serialize\n");
  int auth_size = Packet_serialize(message_buffer, &auth_pckt->header);
  printf("deserialize\n");
  MessageAuth* deserialized_auth_packet =
      (MessageAuth*)Packet_deserialize(message_buffer, auth_size);
  printf(
      "deserialized message packet "
      "with:\ntype\t%d\nsize\t%d\nid\t%d\nusername\t%s\n",
      deserialized_auth_packet->header.type,
      deserialized_auth_packet->header.size, deserialized_auth_packet->id,
      deserialized_auth_packet->username);
  Packet_free(&deserialized_auth_packet->header);
  Packet_free(&auth_pckt->header);

  printf("\n\nallocate a MessageHistory packet \n");
  PacketHeader history_header;
  history_header.type = ChatHistory;
  MessageHistory* history_pckt =
      (MessageHistory*)malloc(sizeof(MessageHistory));
  history_pckt->header = history_header;
  history_pckt->num_messages = 1;
  MessageBroadcast* messages = (MessageBroadcast*)malloc(
      sizeof(MessageBroadcast) * history_pckt->num_messages);
  strncpy(messages->sender, username, USERNAME_LEN);
  strncpy(messages->text, text, TEXT_LEN);
  messages->id = 10;
  messages->type = Text;
  time(&messages->time);
  info = localtime(&messages->time);
  history_pckt->messages = messages;

  printf(
      "MessageHistory packet "
      "with:\ntype\t%d\nsize\t%d\nnum_messages\t%d\nusername\t%s\ntext\t%"
      "s\nmessageType\t%d\ntime\t%d:%d\n",
      history_pckt->header.type, history_pckt->header.size,
      history_pckt->num_messages, history_pckt->messages->sender,
      history_pckt->messages->text, history_pckt->messages->type, info->tm_hour,
      info->tm_min);
  printf("serialize\n");
  int history_size = Packet_serialize(message_buffer, &history_pckt->header);
  printf("deserialize\n");
  MessageHistory* deserialized_history_packet =
      (MessageHistory*)Packet_deserialize(message_buffer, history_size);
  info = localtime(&deserialized_history_packet->messages->time);
  printf(
      "MessageHistory packet "
      "with:\ntype\t%d\nsize\t%d\nnum_messages\t%d\nusername\t%s\ntext\t%"
      "s\nmessageType\t%d\ntime\t%d:%d\n",
      deserialized_history_packet->header.type,
      deserialized_history_packet->header.size,
      deserialized_history_packet->num_messages,
      deserialized_history_packet->messages->sender,
      deserialized_history_packet->messages->text,
      deserialized_history_packet->messages->type, info->tm_hour, info->tm_min);
  Packet_free(&deserialized_history_packet->header);
  Packet_free(&history_pckt->header);
  return 0;
}
