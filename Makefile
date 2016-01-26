SOURCE_C = \
	main.c \
	scheduler.c \
	xxxws.c \
	cbuf.c \
	xxxws_port.c \
	os.c \
	resource.c \
	mvc.c \
	http.c \
	fs_ram.c \
	stats.c

UNAME := $(shell uname)
FLAGS=
ifeq ($(UNAME), MINGW32_NT-6.1)
	FLAGS=-lws2_32
endif
ifeq ($(UNAME), MINGW64_NT-6.1)
	FLAGS=-lws2_32
endif

all:
	gcc -g -Wall $(SOURCE_C) $(FLAGS) -o a.out
	size a.out
