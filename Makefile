CC ?= gcc
CFLAGS ?= -O3

default: camag

clean:
	rm *.o camag

%.o: %.c render.h capture.h capture-v4l2.h camag.h
	$(CC) $(CFLAGS) -c -o $@ $<

camag: camag.o capture.o capture-v4l2.o render.o render-scalar.o
	$(CC) -o camag $^ -lSDL -lv4l2 -lX11
