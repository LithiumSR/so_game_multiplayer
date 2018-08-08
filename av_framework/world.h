#pragma once
#include <sys/time.h>
#include "../game_framework/linked_list.h"
#include "../game_framework/vehicle.h"
#include "image.h"
#include "surface.h"

typedef struct World {
  ListHead vehicles;  // list of vehicles
  Surface ground;     // surface

  // stuff
  float dt;
  struct timeval last_update;
  float time_scale;
  char disable_collisions;
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