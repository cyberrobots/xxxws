SOURCE_C = \
	main.c \
	scheduler.c \
	xxxws.c \
	cbuf.c \
	xxxws_port.c \
	os.c \
	fstream.c \
	mvc.c \
	http.c \
	stats.c

all:
	gcc -g -Wall $(SOURCE_C) -o a.out
	size a.out
