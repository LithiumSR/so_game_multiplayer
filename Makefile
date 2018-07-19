CCOPTS= -Wall -g -Wstrict-prototypes
LIBS= -lglut -lGLU -lGL -lm -lpthread -lopenal -lalut
CC=gcc -std=gnu99
AR=ar


BINS=libproto_game.a\
     protogame_server\
     protogame_client\
	test_packets_serialization\
	test_client_list\
	test_audio\
	test_message_list
	
OBJS = av_framework/vec3.o\
       av_framework/surface.o\
       av_framework/image.o\
       av_framework/audio_list.o\
       av_framework/world.o\
       av_framework/world_viewer.o\
       av_framework/audio_context.o\
       game_framework/linked_list.o\
       game_framework/vehicle.o\
       game_framework/protogame_protocol.o\
       game_framework/client_list.o\
	   game_framework/message_list.o\
       client/client_op.o\
       
HEADERS=av_framework/image.h\
	game_framework/linked_list.h\
	game_framework/protogame_protocol.h\
	game_framework/vehicle.h\
	game_framework/client_list.h\
	game_framework/message_list.o\
	av_framework/surface.h\
	av_framework/vec3.h\
	av_framework/audio_list.h\
	av_framework/world.h\
	av_framework/world_viewer.h\
	av_framework/audio_context.h\
	common/common.h\
	client/client_op.h\

%.o:	%.c $(HEADERS)
	$(CC) $(CCOPTS) -c -o $@ $<

.phony: clean all

all:	$(BINS)

libproto_game.a: $(OBJS)
	$(AR) -rcs $@ $^
	$(RM) $(OBJS)

protogame_client: client/protogame_client.c libproto_game.a
	$(CC) $(CCOPTS) -Ofast -o $@ $^ $(LIBS)

protogame_server: server/protogame_server.c libproto_game.a
	$(CC) $(CCOPTS) -Ofast -o $@ $^ $(LIBS)

test_packets_serialization: tests/test_packets_serialization.c libproto_game.a
	$(CC) $(CCOPTS) -Ofast -o $@ $^  $(LIBS)

test_client_list: tests/test_client_list.c libproto_game.a
	$(CC) $(CCOPTS) -Ofast -o $@ $^  $(LIBS)

test_audio: tests/test_audio.c libproto_game.a
	$(CC) $(CCOPTS) -Ofast -o $@ $^  $(LIBS)

test_message_list: tests/test_message_list.c libproto_game.a
	$(CC) $(CCOPTS) -Ofast -o $@ $^  $(LIBS)




