#include "vehicle.h"
#include <GL/gl.h>
#include <math.h>
#include "world.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

int Vehicle_update(Vehicle* v, float dt){
	int ret;
	#ifdef _USE_VEHICLE_SEM_
    ret= sem_wait(&(v->vsem));
    #endif
    if (ret==-1) debug_print("Wait on vsem didn't worked as expected");
    float tf=v->translational_force_update;
    float rf=v->rotational_force_update;
    if (tf > v->max_translational_force) tf = v->max_translational_force;
    if (tf < -v->max_translational_force) tf = -v->max_translational_force;
    if (rf > v->max_rotational_force) rf = v->max_rotational_force;
    if (rf < -v->max_rotational_force) rf = -v->max_rotational_force;
    float x,y,theta;
    x=v->x;
    y=v->y;
    theta=v->theta;
    #ifdef _USE_VEHICLE_SEM_
    ret=sem_post(&(v->vsem));
    #endif
    if (ret==-1)debug_print("Post on vsem didn't worked as expected.");
    // retrieve the position of the vehicle
    if(! Surface_getTransform(v->camera_to_world, &v->world->ground, x,y, 0, theta, 0)) {
        v->translational_velocity = 0;
        v->rotational_velocity = 0;
        return 0;
    }

    // compute the new pose of the vehicle, based on the velocities
    // vehicle moves only along the x axis!
    
    #ifdef _USE_VEHICLE_SEM_
    ret= sem_wait(&(v->vsem));
    if (ret==-1)debug_print("Wait on vsem didn't worked as expected");
	#endif
	
    float nx = v->camera_to_world[12] + v->camera_to_world[0] * v->translational_velocity * dt;
    float ny = v->camera_to_world[13] + v->camera_to_world[1] * v->translational_velocity * dt;
    
    #ifdef _USE_VEHICLE_SEM_
    ret=sem_post(&(v->vsem));
    if (ret==-1)debug_print("Post on vsem didn't worked as expected");
	#endif
	
    if(! Surface_getTransform(v->camera_to_world, &v->world->ground, nx, ny, 0, v->theta, 0)) {
        return 0;
    }
    
	#ifdef _USE_VEHICLE_SEM_
    ret= sem_wait(&(v->vsem));
    #endif
    
    if (ret==-1)debug_print("Wait on vsem didn't worked as expected");
    v->x=v->camera_to_world[12];
    v->y=v->camera_to_world[13];
    v->z=v->camera_to_world[14];
    v->theta += v->rotational_velocity * dt;
    theta=v->theta;
    
    #ifdef _USE_VEHICLE_SEM_
    ret=sem_post(&(v->vsem));
    if (ret==-1)debug_print("Post on vsem didn't worked as expected");
	#endif
	
    if(! Surface_getTransform(v->camera_to_world, &v->world->ground, nx, ny, 0, theta, 0)){
        return 0;
    }

    // compute the accelerations
    #ifdef _USE_VEHICLE_SEM_
    ret= sem_wait(&(v->vsem));
    if (ret==-1)debug_print("Wait on vsem didn't worked as expected");
	#endif
	
    float global_tf=(-9.8 * v->camera_to_world[2] + tf);
    if ( fabs(global_tf)<v->min_translational_force)
    global_tf = 0;
    v->translational_velocity += global_tf * dt;

    if ( fabs(rf)<v->min_rotational_force)
    rf = 0;
    v->rotational_velocity += rf * dt;
    v->translational_velocity *= v->translational_viscosity;
    v->rotational_velocity *= v->rotational_viscosity;
    #ifdef _USE_VEHICLE_SEM_
    ret=sem_post(&(v->vsem));
    if (ret==-1)debug_print("Post on vsem didn't worked as expected");
	#endif
    Surface_getTransform(v->world_to_camera, &v->world->ground, nx, ny, 0, theta, 1);
    return 1;
}

void Vehicle_init(Vehicle* v, World* w, int id, Image* texture){
	int ret;
	ret= pthread_mutex_init(&(v->mutex), NULL);
    if (ret==-1) debug_print("Mutex init for vehicle was not successfuf");
    
	#ifdef _USE_VEHICLE_SEM_
    ret = sem_init(&(v->vsem), 0, 1);
    if (ret==-1) debug_print("Sem init for vehicle was not successfuf");
    ret= sem_wait(&(v->vsem));
    if (ret==-1)debug_print("Wait on vsem didn't worked as expected");
	#endif
	
    v->world= w;
    v->id=id;
    v->texture=texture;
    v->theta = 0;
    v->list.next=v->list.prev=0;
    v->x = v->world->ground.rows/2 * v->world->ground.row_scale;
    v->y = v->world->ground.cols/2 * v->world->ground.col_scale;
    v->translational_force_update=0;
    v->rotational_force_update=0;
    v->max_rotational_force=0.5;
    v->max_translational_force=10;
    v->min_rotational_force=0.05;
    v->min_translational_force=0.05;
    v->translational_velocity=0;
    v->rotational_velocity=0;
    gettimeofday(&v->world_update_time,NULL);
    #ifdef _USE_VEHICLE_SEM_
    ret=sem_post(&(v->vsem));
    if (ret==-1)debug_print("Post on vsem didn't worked as expected");
	#endif
	
    Vehicle_reset(v);
    
	#ifdef _USE_VEHICLE_SEM_
    ret= sem_wait(&(v->vsem));
    if (ret==-1)debug_print("Wait on vsem didn't worked as expected");
    #endif
    
    v->gl_texture = -1;
    v->gl_list = -1;
    v->_destructor=0;
    
    #ifdef _USE_VEHICLE_SEM_
    ret=sem_post(&(v->vsem));
    if (ret==-1)debug_print("Post on vsem didn't worked as expected");
    #endif
}


void Vehicle_reset(Vehicle* v){
	#ifdef _USE_VEHICLE_SEM_
    int ret= sem_wait(&(v->vsem));
    if (ret==-1)debug_print("Wait on vsem didn't worked as expected");
	#endif
    v->rotational_force=0;
    v->translational_force=0;
    v->x = v->world->ground.rows/2 * v->world->ground.row_scale;
    v->y = v->world->ground.cols/2 * v->world->ground.col_scale;
    v->theta = 0;
    v->translational_viscosity = 0.5;
    v->rotational_viscosity = 0.5;
    float x,y,theta;
    x=v->x;
    y=v->y;
    theta=v->theta;
    #ifdef _USE_VEHICLE_SEM_
    ret=sem_post(&(v->vsem));
    if (ret==-1)debug_print("Post on vsem didn't worked as expected");
    #endif
    if(! Surface_getTransform(v->camera_to_world, &v->world->ground, x, y, 0, theta, 0)) return;
    return;
}

void Vehicle_getXYTheta(Vehicle* v,float* x, float* y, float* theta){
	#ifdef _USE_VEHICLE_SEM_
    int ret= sem_wait(&(v->vsem));
    if (ret==-1)debug_print("Wait on vsem didn't worked as expected");
    #endif
    *x=v->x;
    *y=v->y;
    *theta=v->theta;
    #ifdef _USE_VEHICLE_SEM_
    ret=sem_post(&(v->vsem));
    if (ret==-1)debug_print("Post on vsem didn't worked as expected3");
    #endif
}

void Vehicle_setXYTheta(Vehicle* v, float x, float y, float theta){
	#ifdef _USE_VEHICLE_SEM_
    int ret= sem_wait(&(v->vsem));
    if (ret==-1)debug_print("Wait on vsem didn't worked as expected");
    #endif
    v->x=x;
    v->y=y;
    v->theta=theta;
    #ifdef _USE_VEHICLE_SEM_
    ret=sem_post(&(v->vsem));
    if (ret==-1)debug_print("Post on vsem didn't worked as expected");
    #endif
}


void Vehicle_getForcesUpdate(Vehicle* v, float* translational_update, float* rotational_update){
	#ifdef _USE_VEHICLE_SEM_
    int ret= sem_wait(&(v->vsem));
    if (ret==-1) debug_print("Wait on vsem didn't worked as expected");
    #endif
    *translational_update=v->translational_force_update;
    *rotational_update=v->rotational_force_update;
    #ifdef _USE_VEHICLE_SEM_
    ret=sem_post(&(v->vsem));
    if (ret==-1) debug_print("Post on vsem didn't worked as expected");
    #endif
}

void Vehicle_setForcesUpdate(Vehicle* v, float translational_update, float rotational_update){
	#ifdef _USE_VEHICLE_SEM_
    int ret= sem_wait(&(v->vsem));
    if (ret==-1)debug_print("Wait on vsem didn't worked as expected");
    #endif
    v->translational_force_update=translational_update;
    v->rotational_force_update=rotational_update;
    #ifdef _USE_VEHICLE_SEM_
    ret=sem_post(&(v->vsem));
    if (ret==-1)debug_print("Post on vsem didn't worked as expected");
    #endif
}

void Vehicle_setTime(Vehicle* v, struct timeval time){
	#ifdef _USE_VEHICLE_SEM_
    int ret= sem_wait(&(v->vsem));
    if (ret==-1)debug_print("Wait on vsem didn't worked as expected");
    #endif
    v->world_update_time=time;
    #ifdef _USE_VEHICLE_SEM_
    ret=sem_post(&(v->vsem));
    if (ret==-1)debug_print("Post on vsem didn't worked as expected");
    #endif
}

void Vehicle_getTime(Vehicle* v, struct timeval* time){
	#ifdef _USE_VEHICLE_SEM_
    int ret= sem_wait(&(v->vsem));
    if (ret==-1)debug_print("Wait on vsem didn't worked as expected");
    #endif
    *time=v->world_update_time;
    #ifdef _USE_VEHICLE_SEM_
    ret=sem_post(&(v->vsem));
    if (ret==-1)debug_print("Post on vsem didn't worked as expected");
    #endif
}

	
void Vehicle_destroy(Vehicle* v){
	int ret;
	#ifdef _USE_VEHICLE_SEMAPHORE_
    ret= sem_destroy(&(v->vsem));
    if (ret==-1)debug_print("Vehicle semaphore wasn't successfully destroyed");
    #endif
    ret=pthread_mutex_destroy(&(v->mutex));
    if (ret==-1)debug_print("Vehicle's mutex wasn't successfully destroyed");
	if (v->_destructor) (*v->_destructor)(v);
}
