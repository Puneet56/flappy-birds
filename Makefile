main: main.c
	gcc $(shell pkg-config --cflags --libs raylib) -o main main.c
