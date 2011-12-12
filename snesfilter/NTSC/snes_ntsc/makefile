all: benchmark demo

benchmark: benchmark.c snes_ntsc.c
	$(CC) -lm -o benchmark benchmark.c snes_ntsc.c

demo: demo.c snes_ntsc.c
	$(CC) -lm -o demo $(shell sdl-config --cflags) $(shell sdl-config --libs) demo.c snes_ntsc.c

clean:
	-$(RM) demo benchmark

