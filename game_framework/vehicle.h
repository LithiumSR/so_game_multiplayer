#pragma once
#include <semaphore.h>
#include "../av_framework/image.h"
#include "../av_framework/surface.h"
#include "linked_list.h"

struct World;
struct Vehicle;
typedef void (*VehicleDtor)(struct Vehicle* v);

typedef struct Vehicle {
  ListItem list;
  int id;
  struct World* world;
  Image* texture;
  pthread_mutex_t mutex;
  // these are the forces that will be applied after the update and the critical
  // section
  float translational_force_update;
  float rotational_force_update;
  float x, y, z,
      theta;  // position and orientation of the vehicle, on the surface

  // dont' touch these
  char is_new, manual_updated;
  float temp_x, temp_y, temp_z;
  float prev_x, prev_y, prev_z,
      prev_theta;  // orientation of the vehicle, on the surface
  float translational_velocity;
  float rotational_velocity;
  float translational_viscosity;
  float rotational_viscosity;
  float world_to_camera[16];
  float camera_to_world[16];
  float mass, angular_mass;
  float rotational_force, max_rotational_force, min_rotational_force;
  float translational_force, max_translational_force, min_translational_force;
  struct timeval world_update_time;
  int gl_texture;
  int gl_list;
  VehicleDtor _destructor;
} Vehicle;

void Vehicle_init(Vehicle* v, struct World* w, int id, Image* texture);

void Vehicle_reset(Vehicle* v);

void Vehicle_getForcesUpdate(Vehicle* v, float* translational_update,
                             float* rotational_update);

void Vehicle_getXYTheta(Vehicle* v, float* x, float* y, float* theta);

void Vehicle_setForcesUpdate(Vehicle* v, float translational_update,
                             float rotational_update);

void Vehicle_setXYTheta(Vehicle* v, float x, float y, float theta);

void Vehicle_getTime(Vehicle* v, struct timeval* time);

void Vehicle_setTime(Vehicle* v, struct timeval time);

int Vehicle_update(Vehicle* v, float dt);

void Vehicle_destroy(Vehicle* v);

void Vehicle_increaseTranslationalForce(Vehicle* v,
                                        float translational_force_update);

void Vehicle_increaseRotationalForce(Vehicle* v, float rotational_force_update);

void Vehicle_decreaseRotationalForce(Vehicle* v, float rotational_force_update);

void Vehicle_decreaseTranslationalForce(Vehicle* v,
                                        float translational_force_update);

void Vehicle_decayForcesUpdate(Vehicle* v, float translational_update_decay,
                               float rotational_update_decay);

int Vehicle_fixCollisions(Vehicle* v, Vehicle* v2);