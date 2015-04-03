all: project1

project1: functions.c main.c
	gcc -o project1 functions.c main.c

clean:
	rm -f project1
