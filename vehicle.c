#include "vehicle.h"
#include <GL/gl.h>
#include <math.h>
#include "world.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
int Vehicle_update(Vehicle* v, float dt){
    int ret= sem_wait(&(v->vsem));
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
    ret=sem_post(&(v->vsem));
    if (ret==-1)debug_print("Post on vsem didn't worked as expected.");
    // retrieve the position of the vehicle
    if(! Surface_getTransform(v->camera_to_world, &v->world->ground, x,y, 0, theta, 0)) {
        v->translational_velocity = 0;
        v->rotational_velocity = 0;
        return 0;
    }

    // compute the new pose of the vehicle, based on the velocities
    // vehicle moves only along the x axis!
    ret= sem_wait(&(v->vsem));
    if (ret==-1)debug_print("Wait on vsem didn't worked as expected");

    float nx = v->camera_to_world[12] + v->camera_to_world[0] * v->translational_velocity * dt;
    float ny = v->camera_to_world[13] + v->camera_to_world[1] * v->translational_velocity * dt;
    ret=sem_post(&(v->vsem));
    if (ret==-1)debug_print("Post on vsem didn't worked as expected");

    if(! Surface_getTransform(v->camera_to_world, &v->world->ground, nx, ny, 0, v->theta, 0)) {
        return 0;
    }

    ret= sem_wait(&(v->vsem));
    if (ret==-1)debug_print("Wait on vsem didn't worked as expected");
    v->x=v->camera_to_world[12];
    v->y=v->camera_to_world[13];
    v->z=v->camera_to_world[14];
    v->theta += v->rotational_velocity * dt;
    theta=v->theta;
    ret=sem_post(&(v->vsem));
    if (ret==-1)debug_print("Post on vsem didn't worked as expected");

    if(! Surface_getTransform(v->camera_to_world, &v->world->ground, nx, ny, 0, theta, 0)){
        return 0;
    }

    // compute the accelerations
    ret= sem_wait(&(v->vsem));
    if (ret==-1)debug_print("Wait on vsem didn't worked as expected");

    float global_tf=(-9.8 * v->camera_to_world[2] + tf);
    if ( fabs(global_tf)<v->min_translational_force)
    global_tf = 0;
    v->translational_velocity += global_tf * dt;

    if ( fabs(rf)<v->min_rotational_force)
    rf = 0;
    v->rotational_velocity += rf * dt;
    v->translational_velocity *= v->translational_viscosity;
    v->rotational_velocity *= v->rotational_viscosity;
    ret=sem_post(&(v->vsem));
    if (ret==-1)debug_print("Post on vsem didn't worked as expected");

    Surface_getTransform(v->world_to_camera, &v->world->ground, nx, ny, 0, theta, 1);
    return 1;
}

void Vehicle_init(Vehicle* v, World* w, int id, Image* texture){
    int ret = sem_init(&(v->vsem), 0, 1);
    if (ret==-1) debug_print("Sem init for vehicle was not successfuf");
    ret= sem_wait(&(v->vsem));
    if (ret==-1)debug_print("Wait on vsem didn't worked as expected");

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
    ret=sem_post(&(v->vsem));
    if (ret==-1)debug_print("Post on vsem didn't worked as expected");

    Vehicle_reset(v);

    ret= sem_wait(&(v->vsem));
    if (ret==-1)debug_print("Wait on vsem didn't worked as expected");
    v->gl_texture = -1;
    v->gl_list = -1;
    v->_destructor=0;
    ret=sem_post(&(v->vsem));
    if (ret==-1)debug_print("Post on vsem didn't worked as expected");

}


void Vehicle_reset(Vehicle* v){
    int ret= sem_wait(&(v->vsem));
    if (ret==-1)debug_print("Wait on vsem didn't worked as expected");

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
    ret=sem_post(&(v->vsem));
    if (ret==-1)debug_print("Post on vsem didn't worked as expected");
    if(! Surface_getTransform(v->camera_to_world, &v->world->ground, x, y, 0, theta, 0)) return;
    return;
}

void Vehicle_getXYTheta(Vehicle* v,float* x, float* y, float* theta){
    int ret= sem_wait(&(v->vsem));
    if (ret==-1)debug_print("Wait on vsem didn't worked as expected");
    *x=v->x;
    *y=v->y;
    *theta=v->theta;
    ret=sem_post(&(v->vsem));
    if (ret==-1)debug_print("Post on vsem didn't worked as expected3");
}

void Vehicle_setXYTheta(Vehicle* v, float x, float y, float theta){
    int ret= sem_wait(&(v->vsem));
    if (ret==-1)debug_print("Wait on vsem didn't worked as expected");
    v->x=x;
    v->y=y;
    v->theta=theta;
    ret=sem_post(&(v->vsem));
    if (ret==-1)debug_print("Post on vsem didn't worked as expected");
}


void Vehicle_getForcesUpdate(Vehicle* v, float* translational_update, float* rotational_update){
    int ret= sem_wait(&(v->vsem));
    if (ret==-1) debug_print("Wait on vsem didn't worked as expected");
    *translational_update=v->translational_force_update;
    *rotational_update=v->rotational_force_update;
    ret=sem_post(&(v->vsem));
    if (ret==-1) debug_print("Post on vsem didn't worked as expected");
}

void Vehicle_setForcesUpdate(Vehicle* v, float translational_update, float rotational_update){
    int ret= sem_wait(&(v->vsem));
    if (ret==-1)debug_print("Wait on vsem didn't worked as expected");
    v->translational_force_update=translational_update;
    v->rotational_force_update=rotational_update;
    ret=sem_post(&(v->vsem));
    if (ret==-1)debug_print("Post on vsem didn't worked as expected");
}




void Vehicle_destroy(Vehicle* v){
    int ret= sem_destroy(&(v->vsem));
    if (ret==-1)debug_print("Vehicle semaphore wasn't successfully destroyed");
  if (v->_destructor)
    (*v->_destructor)(v);
}
