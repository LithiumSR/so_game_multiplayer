#pragma once
#include <sys/time.h>
#include "../av_framework/image.h"
#include "../av_framework/surface.h"
#include "linked_list.h"
#include "vehicle.h"

typedef struct World {
  ListHead vehicles;  // list of vehicles
  Surface ground;     // surface

  // stuff
  float dt;
  struct timeval last_update;
  float time_scale;
  char disable_collisions;
  char disable_decay;
  pthread_mutex_t update_mutex;
} World;

int World_init(World* w, Image* surface_elevation, Image* surface_texture,
               float x_step, float y_step, float z_step);

void World_destroy(World* w);

void World_update(World* w);

void World_decayUpdate(World* w);

Vehicle* World_getVehicle(World* w, int vehicle_id);

Vehicle* World_addVehicle(World* w, Vehicle* v);

Vehicle* World_detachVehicle(World* w, Vehicle* v);

void World_manualUpdate(World* w, Vehicle* v, struct timeval update_time);

void World_disableVehicleCollisions(World* w);

void World_disableDecay(World* w);