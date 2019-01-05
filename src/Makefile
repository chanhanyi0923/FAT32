OPTIONS := -ansi -g
DEFINE := PC_ENV

main: main.o fat.o utils.o pc_env.o usr.o
	gcc main.o fat.o utils.o pc_env.o usr.o -o main

main.o:
	gcc $(OPTIONS) -D$(DEFINE) main.c -c

fat.o:
	gcc $(OPTIONS) -D$(DEFINE) fat.c -c

usr.o:
	gcc $(OPTIONS) -D$(DEFINE) usr.c -c

utils.o:
	gcc $(OPTIONS) -D$(DEFINE) utils.c -c

pc_env.o:
	gcc $(OPTIONS) -D$(DEFINE) pc_env.c -c

clean:
	rm -f *.o main
