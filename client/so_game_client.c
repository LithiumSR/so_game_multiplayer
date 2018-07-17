#include <GL/glut.h>
#include <arpa/inet.h>  // htons() and inet_addr()
#include <fcntl.h>
#include <math.h>
#include <netinet/in.h>  // struct sockaddr_in
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include "../av_framework/audio_context.h"
#include "../av_framework/image.h"
#include "../av_framework/surface.h"
#include "../av_framework/world.h"
#include "../av_framework/world_viewer.h"
#include "../common/common.h"
#include "../game_framework/so_game_protocol.h"
#include "../game_framework/vehicle.h"
#include "client_op.h"
#define UNTOUCHED 0
#define TOUCHED 1
#define SENDER_SLEEP 200 * 1000
#define RECEIVER_SLEEP 50 * 1000
#define MAX_FAILED_ATTEMPTS 20
// world related variables
int window;
World world;
Vehicle* vehicle;  // The vehicle
int id;
char username[32];
AudioContext* backgroud_track = NULL;
// flags and counters
char connectivity = 1;
char exchange_update = 1;
// networking
uint16_t port_number_no;
int socket_desc = -1;  // socket tcp
int socket_udp = -1;   // socket udp
struct timeval last_update_time;
struct timeval start_time;
pthread_mutex_t time_lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct localWorld {
  int ids[WORLDSIZE];
  int users_online;
  char has_vehicle[WORLDSIZE];
  struct timeval vehicle_login_time[WORLDSIZE];
  Vehicle** vehicles;
} localWorld;

typedef struct listenArgs {
  localWorld* lw;
  struct sockaddr_in server_addr;
  int socket_udp;
  int socket_tcp;
} udpArgs;

void handleSignal(int signal) {
  // Find out which signal we're handling
  switch (signal) {
    case SIGHUP:
      break;
    case SIGINT:
      connectivity = 0;
      exchange_update = 0;
      pthread_mutex_lock(&time_lock);
      if (last_update_time.tv_sec != 1) sendGoodbye(socket_desc, id);
      pthread_mutex_unlock(&time_lock);
      WorldViewer_exit(0);
      break;
    default:
      fprintf(stderr, "Caught wrong signal: %d\n", signal);
      return;
  }
}

//This method returns the index of the user in the array. If it returns -1 and existing_index is != -1 it means that the user was already in the array in the 
//position 'existing_index'. If the method returns -1 and existing_index==-1 it means that the world is full and no space can be found to save the new user.
int addUser(int ids[], int size, int id, int* existing_index, int* users_online) {
  if (*users_online == WORLDSIZE) {
    *existing_index = -1;
    return -1;
  }
  int ret = hasUser(ids, WORLDSIZE, id);
  if (ret != -1) return ret;
  for (int i = 0; i < size; i++) {
    if (ids[i] == -1) {
      ids[i] = id;
      *users_online += 1;
      *existing_index = i;
      break;
    }
  }
  return -1;
}

int sendUpdates(int socket_udp, struct sockaddr_in server_addr, int serverlen) {
  char buf_send[BUFFERSIZE];
  PacketHeader ph;
  ph.type = VehicleUpdate;
  VehicleUpdatePacket* vup =
      (VehicleUpdatePacket*)malloc(sizeof(VehicleUpdatePacket));
  vup->header = ph;
  gettimeofday(&vup->time, NULL);
  pthread_mutex_lock(&vehicle->mutex);
  Vehicle_getForcesIntention(vehicle, &(vup->translational_force),
                             &(vup->rotational_force));
  Vehicle_setForcesIntention(vehicle, 0, 0);
  pthread_mutex_unlock(&vehicle->mutex);
  vup->id = id;
  int size = Packet_serialize(buf_send, &vup->header);
  int bytes_sent =
      sendto(socket_udp, buf_send, size, 0,
             (const struct sockaddr*)&server_addr, (socklen_t)serverlen);
  debug_print(
      "[UDP_Sender] Sent a VehicleUpdatePacket of %d bytes with tf:%f rf:%f \n",
      bytes_sent, vup->translational_force, vup->rotational_force);
  Packet_free(&(vup->header));
  struct timeval current_time;
  gettimeofday(&current_time, NULL);
  pthread_mutex_lock(&time_lock);
  if (last_update_time.tv_sec == -1 &&
      current_time.tv_sec - start_time.tv_sec > MAX_TIME_WITHOUT_WORLDUPDATE) {
    connectivity = 0;
    exchange_update = 0;
    fprintf(stderr,
            "[WARNING] Server is not avaiable. Terminating the client now...");
    WorldViewer_exit(0);
  } else if (last_update_time.tv_sec != -1 &&
             current_time.tv_sec - last_update_time.tv_sec >
                 MAX_TIME_WITHOUT_WORLDUPDATE) {
    connectivity = 0;
    exchange_update = 0;
    fprintf(stderr,
            "[WARNING] Server is not avaiable. Terminating the client now...");
    WorldViewer_exit(0);
  }
  pthread_mutex_unlock(&time_lock);
  if (bytes_sent < 0) return -1;
  return 0;
}

void* messageSender(void* args) {
  udpArgs udp_args = *(udpArgs*)args;
  struct sockaddr_in server_addr = udp_args.server_addr;
  int socket_udp = udp_args.socket_udp;
  int serverlen = sizeof(server_addr);
  printf(
      "This is a chat over UDP. There is no guarantee that the messages will "
      "be broadcasted \n");
USERNAME:
  printf("Insert your name: ");
  char* chr = fgets(username, USERNAME_LEN, stdin);
  if (chr == NULL || (username[0] == '\n' && username[1] == '\0'))
    goto USERNAME;
  username[strcspn(username, "\n")] = 0;
  printf("Hello %s! You can now write your messages (MAX %d characters) \n",
         username, TEXT_LEN);
  int ret = joinChat(socket_desc, id, username);
  ERROR_HELPER(ret, "Can't send username over TCP!");
  // Get user messages
  while (connectivity) {
    char buf_send[BUFFERSIZE];
    PacketHeader ph;
    ph.type = ChatMessage;
    MessagePacket* mp = (MessagePacket*)malloc(sizeof(MessagePacket));
    mp->header = ph;
    mp->message.id = id;
    mp->message.type = Text;
    char* text = fgets(mp->message.text, TEXT_LEN, stdin);
    if (text == NULL ||
        (mp->message.text[0] == '\n' && mp->message.text[1] == '\0')) {
      Packet_free(&mp->header);
      continue;
    }
    int size = Packet_serialize(buf_send, &(mp->header));
    if (size <= 0) continue;
    int bytes_sent =
        sendto(socket_udp, buf_send, size, 0,
               (const struct sockaddr*)&server_addr, (socklen_t)serverlen);
    if (bytes_sent != size)
      debug_print("[ERROR] Message wasn't successfully sent");
    else
      Packet_free(&mp->header);
  }
  pthread_exit(NULL);
}

// Send vehicleUpdatePacket to server
void* UDPSender(void* args) {
  udpArgs udp_args = *(udpArgs*)args;
  struct sockaddr_in server_addr = udp_args.server_addr;
  int socket_udp = udp_args.socket_udp;
  int serverlen = sizeof(server_addr);
  while (connectivity && exchange_update) {
    int ret = sendUpdates(socket_udp, server_addr, serverlen);
    if (ret == -1)
      debug_print("[UDP_Sender] Cannot send VehicleUpdatePacket \n");
    usleep(SENDER_SLEEP);
  }
  pthread_exit(NULL);
}

// Receive and apply WorldUpdatePacket from server
void* UDPReceiver(void* args) {
  udpArgs udp_args = *(udpArgs*)args;
  struct sockaddr_in server_addr = udp_args.server_addr;
  int socket_udp = udp_args.socket_udp;
  socklen_t addrlen = sizeof(server_addr);
  localWorld* lw = udp_args.lw;
  int socket_tcp = udp_args.socket_tcp;
  while (connectivity && exchange_update) {
    char buf_rcv[BUFFERSIZE];
    int bytes_read = recvfrom(socket_udp, buf_rcv, BUFFERSIZE, 0,
                              (struct sockaddr*)&server_addr, &addrlen);
    if (bytes_read == -1) {
      debug_print("[UDP_Receiver] Can't receive Packet over UDP \n");
      usleep(RECEIVER_SLEEP);
      continue;
    }
    if (bytes_read == 0) {
      usleep(RECEIVER_SLEEP);
      continue;
    }

    debug_print("[UDP_Receiver] Received %d bytes over UDP\n", bytes_read);
    PacketHeader* ph = (PacketHeader*)buf_rcv;
    if (ph->size != bytes_read) {
      debug_print("[WARNING] Skipping partial UDP packet \n");
      usleep(RECEIVER_SLEEP);
      continue;
    }
    switch (ph->type) {
      case (PostDisconnect): {
        fprintf(stderr,
                "[WARNING] You were kicked out of the server for inactivity... "
                "Closing the client now \n");
        sendGoodbye(socket_desc, id);
        connectivity = 0;
        exchange_update = 0;
        WorldViewer_exit(0);
      }
      case (ChatHistory): {
        MessageHistory* mh =
            (MessageHistory*)Packet_deserialize(buf_rcv, bytes_read);
        for (int i = 0; i < mh->num_messages; i++) {
          struct tm* info;
          info = localtime(&mh->messages[i].time);
          if (mh->messages[i].id == id) continue;
          mh->messages[i].text[strcspn(mh->messages[i].text, "\n")] = 0;
          switch (mh->messages[i].type) {
            case (Text): {
              printf("%s (id %d): %s (%d:%d) \n", mh->messages[i].sender,
                     mh->messages[i].id, mh->messages[i].text, info->tm_hour,
                     info->tm_min);
              fflush(stdout);
              break;
            }
            case (Hello): {
              printf("[INFO] %s (id %d) joined the chat! (%d:%d)\n",
                     mh->messages[i].sender, mh->messages[i].id, info->tm_hour,
                     info->tm_min);
              fflush(stdout);
              break;
            }
            case (Goodbye): {
              printf("[INFO] %s (id %d) left the chat! (%d:%d)\n",
                     mh->messages[i].sender, mh->messages[i].id, info->tm_hour,
                     info->tm_min);
              fflush(stdout);
              break;
            }
          }
        }
        Packet_free(&mh->header);
        break;
      }
      case (WorldUpdate): {
        WorldUpdatePacket* wup =
            (WorldUpdatePacket*)Packet_deserialize(buf_rcv, bytes_read);
        pthread_mutex_lock(&time_lock);
        if (last_update_time.tv_sec != -1 &&
            timercmp(&last_update_time, &wup->time, >=)) {
          pthread_mutex_unlock(&time_lock);
          debug_print("[INFO] Ignoring a WorldUpdatePacket... \n");
          Packet_free(&wup->header);
          usleep(RECEIVER_SLEEP);
          continue;
        }

        debug_print("WorldUpdatePacket contains %d vehicles besides mine \n",
                    wup->num_vehicles - 1);
        last_update_time = wup->time;
        pthread_mutex_unlock(&time_lock);
        char mask[WORLDSIZE];
        for (int k = 0; k < WORLDSIZE; k++) mask[k] = UNTOUCHED;
        for (int i = 0; i < wup->num_vehicles; i++) {
          int new_position = -1;
          int id_struct = addUser(lw->ids, WORLDSIZE, wup->updates[i].id,
                                  &new_position, &(lw->users_online));
          if (wup->updates[i].id == id) {
            pthread_mutex_lock(&lw->vehicles[0]->mutex);
            // fprintf(stderr,"x: %f y: %f theta: %f tf: %f rf: %f
            // \n",wup->updates[i].x,
            // wup->updates[i].y,wup->updates[i].theta,wup->updates[i].translational_force,wup->updates[i].rotational_force);
            Vehicle_setXYTheta(lw->vehicles[0], wup->updates[i].x,
                               wup->updates[i].y, wup->updates[i].theta);
            Vehicle_setForcesUpdate(lw->vehicles[0],
                                    wup->updates[i].translational_force,
                                    wup->updates[i].rotational_force);
            World_manualUpdate(&world, lw->vehicles[0],
                               wup->updates[i].client_update_time);
            pthread_mutex_unlock(&lw->vehicles[0]->mutex);
          } else if (id_struct == -1) {
            if (new_position == -1) continue;
            mask[new_position] = TOUCHED;
            debug_print("New Vehicle with id %d and x: %f y: %f z: %f \n",
                        wup->updates[i].id, wup->updates[i].x,
                        wup->updates[i].y, wup->updates[i].theta);
            Image* img = getVehicleTexture(socket_tcp, wup->updates[i].id);
            if (img == NULL) continue;
            Vehicle* new_vehicle = (Vehicle*)malloc(sizeof(Vehicle));
            Vehicle_init(new_vehicle, &world, wup->updates[i].id, img);
            lw->vehicles[new_position] = new_vehicle;
            pthread_mutex_lock(&lw->vehicles[new_position]->mutex);
            Vehicle_setXYTheta(lw->vehicles[new_position], wup->updates[i].x,
                               wup->updates[i].y, wup->updates[i].theta);
            Vehicle_setForcesUpdate(lw->vehicles[new_position],
                                    wup->updates[i].translational_force,
                                    wup->updates[i].rotational_force);
            pthread_mutex_unlock(&lw->vehicles[new_position]->mutex);
            World_addVehicle(&world, new_vehicle);
            lw->has_vehicle[new_position] = 1;
            lw->vehicle_login_time[new_position] =
                wup->updates[i].client_creation_time;
          } else {
            mask[id_struct] = TOUCHED;
            if (timercmp(&wup->updates[i].client_creation_time,
                         &lw->vehicle_login_time[id_struct], !=)) {
              debug_print("[WARNING] Forcing refresh for client with id %d",
                          wup->updates[i].id);
              if (lw->has_vehicle[id_struct]) {
                Image* im = lw->vehicles[id_struct]->texture;
                World_detachVehicle(&world, lw->vehicles[id_struct]);
                Vehicle_destroy(lw->vehicles[id_struct]);
                if (im != NULL) Image_free(im);
                free(lw->vehicles[id_struct]);
              }
              Image* img = getVehicleTexture(socket_tcp, wup->updates[i].id);
              if (img == NULL) continue;
              Vehicle* new_vehicle = (Vehicle*)malloc(sizeof(Vehicle));
              Vehicle_init(new_vehicle, &world, wup->updates[i].id, img);
              lw->vehicles[id_struct] = new_vehicle;
              pthread_mutex_lock(&lw->vehicles[id_struct]->mutex);
              Vehicle_setXYTheta(lw->vehicles[id_struct], wup->updates[i].x,
                                 wup->updates[i].y, wup->updates[i].theta);
              Vehicle_setForcesUpdate(lw->vehicles[id_struct],
                                      wup->updates[i].translational_force,
                                      wup->updates[i].rotational_force);
              World_manualUpdate(&world, lw->vehicles[id_struct],
                                 wup->updates[i].client_update_time);
              pthread_mutex_unlock(&lw->vehicles[id_struct]->mutex);
              World_addVehicle(&world, new_vehicle);
              lw->has_vehicle[id_struct] = 1;
              lw->vehicle_login_time[id_struct] =
                  wup->updates[i].client_creation_time;
              continue;
            }
            debug_print("Updating Vehicle with id %d and x: %f y: %f z: %f \n",
                        wup->updates[i].id, wup->updates[i].x,
                        wup->updates[i].y, wup->updates[i].theta);
            pthread_mutex_lock(&lw->vehicles[id_struct]->mutex);
            Vehicle_setXYTheta(lw->vehicles[id_struct], wup->updates[i].x,
                               wup->updates[i].y, wup->updates[i].theta);
            Vehicle_setForcesUpdate(lw->vehicles[id_struct],
                                    wup->updates[i].translational_force,
                                    wup->updates[i].rotational_force);
            World_manualUpdate(&world, lw->vehicles[id_struct],
                               wup->updates[i].client_update_time);
            pthread_mutex_unlock(&lw->vehicles[id_struct]->mutex);
          }
        }
        for (int i = 0; i < WORLDSIZE; i++) {
          if (mask[i] == TOUCHED) continue;
          if (i == 0) continue;

          if (lw->ids[i] == id) continue;
          if (mask[i] == UNTOUCHED && lw->ids[i] != -1) {
            debug_print("[WorldUpdate] Removing Vehicles with ID %d \n",
                        lw->ids[i]);
            lw->users_online = lw->users_online - 1;
            if (!lw->has_vehicle[i]) continue;
            Image* im = lw->vehicles[i]->texture;
            World_detachVehicle(&world, lw->vehicles[i]);
            if (im != NULL) Image_free(im);
            Vehicle_destroy(lw->vehicles[i]);
            lw->ids[i] = -1;
            free(lw->vehicles[i]);
            lw->has_vehicle[i] = 0;
          }
        }
        Packet_free(&wup->header);
        break;
      }
      default: {
        debug_print(
            "[UDP_Receiver] Found an unknown udp packet. Terminating the "
            "client now... \n");
        sendGoodbye(socket_desc, id);
        connectivity = 0;
        exchange_update = 0;
        WorldViewer_exit(-1);
      }
    }
    usleep(RECEIVER_SLEEP);
  }
  pthread_exit(NULL);
}

int main(int argc, char** argv) {
  if (argc < 3) {
    printf("usage: %s <player texture> <port_number> \n", argv[1]);
    exit(-1);
  }
  fprintf(stdout, "[Main] loading vehicle texture from %s ... ", argv[1]);
  Image* my_texture = Image_load(argv[1]);
  if (my_texture) {
    printf("Done! \n");
  } else {
    printf("Fail! \n");
  }
  long tmp = strtol(argv[2], NULL, 0);

  if (tmp < 1024 || tmp > 49151) {
    fprintf(stderr, "Use a port number between 1024 and 49151.\n");
    exit(EXIT_FAILURE);
  }

  fprintf(stdout, "[Main] Starting... \n");
  last_update_time.tv_sec = -1;
  port_number_no = htons((uint16_t)tmp);  // we use network byte order
  socket_desc = socket(AF_INET, SOCK_STREAM, 0);
  in_addr_t ip_addr = inet_addr(SERVER_ADDRESS);
  ERROR_HELPER(socket_desc, "Cannot create socket \n");
  struct sockaddr_in server_addr = {
      0};  // some fields are required to be filled with 0
  server_addr.sin_addr.s_addr = ip_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = port_number_no;

  int reuseaddr_opt = 1;  // recover server if a crash occurs
  int ret = setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_opt,
                       sizeof(reuseaddr_opt));
  ERROR_HELPER(ret, "Can't set SO_REUSEADDR flag");

  ret = connect(socket_desc, (struct sockaddr*)&server_addr,
                sizeof(struct sockaddr_in));
  ERROR_HELPER(ret, "Cannot connect to remote server \n");
  debug_print("[Main] TCP connection established... \n");

  // seting signal handlers
  struct sigaction sa;
  sa.sa_handler = handleSignal;
  // Restart the system call, if at all possible
  sa.sa_flags = SA_RESTART;
  // Block every signal during the handler
  sigfillset(&sa.sa_mask);
  ret = sigaction(SIGHUP, &sa, NULL);
  ERROR_HELPER(ret, "Error: cannot handle SIGHUP");
  ret = sigaction(SIGINT, &sa, NULL);
  ERROR_HELPER(ret, "Error: cannot handle SIGINT");

  // setting up localWorld
  localWorld* local_world = (localWorld*)malloc(sizeof(localWorld));
  local_world->vehicles = (Vehicle**)malloc(sizeof(Vehicle*) * WORLDSIZE);
  for (int i = 0; i < WORLDSIZE; i++) {
    local_world->ids[i] = -1;
    local_world->has_vehicle[i] = 0;
#ifdef _USE_CACHE_TEXTURE_
    local_world->isDisabled[i] = 0;
#endif
  }

  // Talk with server
  fprintf(stdout, "[Main] Starting ID,map_elevation,map_texture requests \n");
  id = getID(socket_desc);
  local_world->ids[0] = id;
  fprintf(stdout, "[Main] ID number %d received \n", id);
  Image* surface_elevation = getElevationMap(socket_desc);
  fprintf(stdout, "[Main] Map elevation received \n");
  Image* surface_texture = getTextureMap(socket_desc);
  fprintf(stdout, "[Main] Map texture received \n");
  debug_print("[Main] Sending vehicle texture");
  sendVehicleTexture(socket_desc, my_texture, id);
  fprintf(stdout, "[Main] Client Vehicle texture sent \n");
  if (CLIENT_AUDIO) {
    backgroud_track = getAudioContext(socket_desc);
    fprintf(stdout, "[Main] Received track number \n");
  }
  // create Vehicle
  World_init(&world, surface_elevation, surface_texture, 0.5, 0.5, 0.5);
  vehicle = (Vehicle*)malloc(sizeof(Vehicle));
  Vehicle_init(vehicle, &world, id, my_texture);
  World_addVehicle(&world, vehicle);
  local_world->vehicles[0] = vehicle;
  local_world->has_vehicle[0] = 1;
  if (SINGLEPLAYER) goto SKIP;
  // UDP Init
  uint16_t port_number_udp =
      htons((uint16_t)UDPPORT);  // we use network byte order
  socket_udp = socket(AF_INET, SOCK_DGRAM, 0);
  ERROR_HELPER(socket_desc, "Can't create an UDP socket");
  struct sockaddr_in udp_server = {0};
  udp_server.sin_addr.s_addr = ip_addr;
  udp_server.sin_family = AF_INET;
  udp_server.sin_port = port_number_udp;
  debug_print("[Main] Socket UDP created and ready to work \n");
  gettimeofday(&start_time, NULL);  // Accounting

  // Create UDP Threads
  pthread_t UDP_sender, UDP_receiver;
  udpArgs udp_args;
  udp_args.socket_tcp = socket_desc;
  udp_args.server_addr = udp_server;
  udp_args.socket_udp = socket_udp;
  udp_args.lw = local_world;
  ret = pthread_create(&UDP_sender, NULL, UDPSender, &udp_args);
  PTHREAD_ERROR_HELPER(ret, "[MAIN] pthread_create on thread UDP_sender");
  ret = pthread_create(&UDP_receiver, NULL, UDPReceiver, &udp_args);
  PTHREAD_ERROR_HELPER(ret, "[MAIN] pthread_create on thread UDP_receiver");
  // Create message-sender thread
  pthread_t message_sender;
  ret = pthread_create(&message_sender, NULL, messageSender, &udp_args);
  PTHREAD_ERROR_HELPER(ret, "[MAIN] pthread_create on thread UDP_receiver");
// Disconnect from server if required by macro
SKIP:
  if (SINGLEPLAYER) sendGoodbye(socket_desc, id);
  WorldViewer_runGlobal(&world, vehicle, backgroud_track, &argc, argv);

  // Waiting threads to end and cleaning resources
  debug_print("[Main] Disabling and joining on UDP and TCP threads \n");
  connectivity = 0;
  exchange_update = 0;
  if (!SINGLEPLAYER) {
    ret = pthread_join(UDP_sender, NULL);
    PTHREAD_ERROR_HELPER(ret, "pthread_join on thread UDP_sender failed");
    ret = pthread_join(UDP_receiver, NULL);
    PTHREAD_ERROR_HELPER(ret, "pthread_join on thread UDP_receiver failed");
    ret = pthread_join(message_sender, NULL);
    PTHREAD_ERROR_HELPER(ret, "pthread_join on thread message_sender failed");
    ret = close(socket_udp);
    ERROR_HELPER(ret, "Failed to close UDP socket");
  }

  fprintf(stdout, "[Main] Cleaning up... \n");
  sendGoodbye(socket_desc, id);

  // Clean resources
  pthread_mutex_destroy(&time_lock);
  for (int i = 0; i < WORLDSIZE; i++) {
    if (local_world->ids[i] == -1) continue;
    if (i == 0) continue;
    local_world->users_online--;
    if (!local_world->has_vehicle[i]) continue;
    Image* im = local_world->vehicles[i]->texture;
    World_detachVehicle(&world, local_world->vehicles[i]);
    if (im != NULL) Image_free(im);
    Vehicle_destroy(local_world->vehicles[i]);
    free(local_world->vehicles[i]);
  }

  free(local_world->vehicles);
  free(local_world);
  ret = close(socket_desc);
  ERROR_HELPER(ret, "Failed to close TCP socket");
  if (!SINGLEPLAYER) {
    ret = close(socket_udp);
    ERROR_HELPER(ret, "Failed to close UDP socket");
  }
  // world cleanup
  World_destroy(&world);
  Image_free(surface_elevation);
  Image_free(surface_texture);
  Image_free(my_texture);
  exit(EXIT_SUCCESS);
}
