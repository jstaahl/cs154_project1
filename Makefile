all: project2

project2: functions.c main.c printP2.c
	gcc -o project2 functions.c main.c printP2.c

clean:
	rm -f project2
