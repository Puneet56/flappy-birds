main: main.c
	gcc -Wall -Wextra -pedantic $(shell pkg-config --cflags --libs raylib) -o main main.c
