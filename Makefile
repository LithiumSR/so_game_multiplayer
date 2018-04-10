CCOPTS= -Wall -g -Wstrict-prototypes
LIBS= -lglut -lGLU -lGL -lm -lpthread -lopenal -lalut
CC=gcc -std=gnu99
AR=ar


BINS=libso_game.a\
     so_game_server\
     so_game_client\
	test_packets_serialization\
	test_client_list
OBJS = av_framework/vec3.o\
       game_framework/linked_list.o\
       av_framework/surface.o\
       av_framework/image.o\
       game_framework/vehicle.o\
       av_framework/world.o\
       av_framework/world_viewer.o\
       server/so_game_server.o\
       client/so_game_client.o\
       client/client_op.o\
       game_framework/so_game_protocol.o\
       game_framework/client_list.o\
       av_framework/audio_context.o\
       
HEADERS=av_framework/image.h\
	game_framework/linked_list.h\
	game_framework/so_game_protocol.h\
	av_framework/surface.h\
	av_framework/vec3.h\
	game_framework/vehicle.h\
	av_framework/world.h\
	av_framework/world_viewer.h\
	common/common.h\
	client/client_op.h\
	game_framework/client_list.h\
	av_framework/audio_context.h\

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




