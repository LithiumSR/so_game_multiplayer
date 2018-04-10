#pragma once
#include "vehicle.h"
#include "audio_context.h"
#include "world.h"
// call this to start the visualization of the stuff.
// This will block the program, and terminate when pressing esc on the viewport
void WorldViewer_runGlobal(World *world, Vehicle *self, AudioContext *ac,
                           int *argc, char **argv);

// Use this if you want to correctly kill the client's path of execution, glut
// loop and the viewer window
void WorldViewer_exit(int ret);
