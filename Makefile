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

# windows build
#FLAGS=-lws2_32

# unix build
FLAGS=

all:
	gcc -g -Wall $(SOURCE_C) $(FLAGS) -o a.out
	size a.out
