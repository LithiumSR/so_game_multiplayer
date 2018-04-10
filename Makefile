CCOPTS= -Wall -g -Wstrict-prototypes
LIBS= -lglut -lGLU -lGL -lm -lpthread -lopenal -lalut
CC=gcc -std=gnu99
AR=ar


BINS=libso_game.a\
     so_game_server\
     so_game_client\
	test_packets_serialization\
	test_client_list
OBJS = vec3.o\
       linked_list.o\
       surface.o\
       image.o\
       vehicle.o\
       world.o\
       world_viewer.o\
       so_game_server.o\
       so_game_client.o\
       client_op.o\
       so_game_protocol.o\
       client_list.o\
       audio_context.o\
       
HEADERS=helpers.h\
	image.h\
	linked_list.h\
	so_game_protocol.h\
	surface.h\
	vec3.h\
	vehicle.h\
	world.h\
	world_viewer.h\
	common.h\
	client_op.h\
	client_list.h\
	audio_context.h\

%.o:	%.c $(HEADERS)
	$(CC) $(CCOPTS) -c -o $@ $<

.phony: clean all

all:	$(BINS)

libso_game.a: $(OBJS)
	$(AR) -rcs $@ $^
	$(RM) $(OBJS)

so_game_client: client/so_game_client.c libso_game.a
	$(CC) $(CCOPTS) -Ofast -o $@ $^ $(LIBS)

so_game_server: server/so_game_server.c libso_game.a
	$(CC) $(CCOPTS) -Ofast -o $@ $^ $(LIBS)

test_packets_serialization: tests/test_packets_serialization.c libso_game.a
	$(CC) $(CCOPTS) -Ofast -o $@ $^  $(LIBS)

test_client_list: tests/test_client_list.c libso_game.a
	$(CC) $(CCOPTS) -Ofast -o $@ $^  $(LIBS)




