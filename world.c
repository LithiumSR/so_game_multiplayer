#include "world.h"
#include <GL/glut.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include "surface.h"
#include "vehicle.h"
#include "image.h"
#include <sys/time.h>
#include <assert.h>
#include <semaphore.h>

void World_destroy(World* w) {
  Surface_destroy(&w->ground);
  sem_t sem=w->vehicles.sem;
  sem_wait(&(sem));
  ListItem* item=w->vehicles.first;
  while(item){
    Vehicle* v=(Vehicle*)item;
    Vehicle_destroy(v);
    item=item->next;
    free(v);
  }
  sem_post(&(sem));
  sem_destroy(&sem);
}

int World_init(World* w,
	       Image* surface_elevation,
	       Image* surface_texture,
	       float x_step,
	       float y_step,
	       float z_step) {
  List_init(&w->vehicles);
  Image* float_image = Image_convert(surface_elevation, FLOATMONO);

  if (! float_image)
    return 0;

  Surface_fromMatrix(&w->ground,
		     (float**) float_image->row_data,
		     float_image->rows,
		     float_image->cols,
		     .5, .5, 5);
  w->ground.texture=surface_texture;
  Image_free(float_image);
  w->dt = 1;
  w->time_scale = 10;
  gettimeofday(&w->last_update, 0);
  return 1;
}

void World_update(World* w) {
  struct timeval current_time;
  gettimeofday(&current_time, 0);

  struct timeval dt;
  timersub(&current_time, &w->last_update, &dt);
  float delta = dt.tv_sec+1e-6*dt.tv_usec;
  sem_t sem=w->vehicles.sem;
  sem_wait(&sem);
  ListItem* item=w->vehicles.first;
  while(item){
    Vehicle* v=(Vehicle*)item;
    sem_wait(&v->ext_sem);
    if (! Vehicle_update(v, delta*w->time_scale)){
      Vehicle_reset(v);
    }
    sem_post(&v->ext_sem);
    item=item->next;
  }
  sem_post(&sem);
  w->last_update = current_time;
}

void World_manualUpdate(World* w, Vehicle* v, struct timeval update_time){

	struct timeval current_time;
	gettimeofday(&current_time, 0);
	struct timeval dt;
	timersub(&current_time, &update_time, &dt);
	float delta = dt.tv_sec+1e-6*dt.tv_usec;
	if (! Vehicle_update(v, delta*w->time_scale)){
      		Vehicle_reset(v);
    	}
  }
	
Vehicle* World_getVehicle(World* w, int vehicle_id){
  sem_t sem=w->vehicles.sem;
  sem_wait(&sem);
  ListItem* item=w->vehicles.first;
  while(item){
    Vehicle* v=(Vehicle*)item;
    if(v->id==vehicle_id){
      sem_post(&sem);
      return v;
    }
    item=item->next;
  }
  sem_post(&sem);
  return 0;
}

Vehicle* World_addVehicle(World* w, Vehicle* v){
  assert(!World_getVehicle(w,v->id));
  return (Vehicle*)List_insert(&w->vehicles, w->vehicles.last, (ListItem*)v);
}

Vehicle* World_detachVehicle(World* w, Vehicle* v){
  return (Vehicle*)List_detach(&w->vehicles, (ListItem*)v);
}
