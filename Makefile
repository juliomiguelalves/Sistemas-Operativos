CC= gcc

CFLAGS= -I -Wall 

all: 	Argus Argusd 
		mkfifo fifo
		mkfifo wr
		touch fileTarefa.txt
		echo "0" > fileTarefa.txt

	 
functions.o:  functions.c argus.h
		gcc $(CFLAGS) -c functions.c -o functions.o

Argus: 
		$(CC) $(CFLAGS) argus.c -o argus

Argusd:  functions.o
		$(CC) $(CFLAGS) argusd.c functions.o -o argusd
		

clean:
	rm -f *.o argus argusd TarefasTerminadas.txt logs.txt log.idx fileTarefa.txt
	unlink fifo
	unlink wr
