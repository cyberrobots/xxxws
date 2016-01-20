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
	ramfs.c \
	stats.c

all:
	gcc -g -Wall $(SOURCE_C) -o a.out
	size a.out
