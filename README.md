# so_game_multiplayer

This is a protype of a multiplayer video-game developed as part of the final exam of the operating systems course held a held by Giorgio Grisetti and Irvin Aloise at the Sapienza University in 2017-2018

## Getting Started

### Prerequisites

This application is written in C and can only to be compiled (and executed) on a environment that supports Glut, an OpenGL utility.

### How it works
so_game consists of a client and server.
- The client is responsible of sending and receiving updates to the server containing infos about the map and the other users and to execute the graphical representation.
- The server, instead, is supposed to handle the authentication of the new users and to distribute data packet to keep every client updated on what is going on in the map. The server is also responsible of removing inactive users.

### How to compile and execute
This project comes with a makefile that makes the building process pretty straightforward. Just use the `make` command inside the directory in which this project is stored.

The server can be started using `./so_game_server ./images/maze.pgm ./images/maze.ppm 8888` where the first two arguments are the map elevation and the map texture and the last one is the port number that is going to be used.

The client can be executed with `./so_game_client ./images/arrow-right.pgm 8888` where the first argument is the texture of the vehicle that will be visible by everyone and the latter is the port number that will be used during the connection to the server.
