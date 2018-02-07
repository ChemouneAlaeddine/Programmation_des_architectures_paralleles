P?=24
DIM?=8191
GRAIN?=32

FLAGS=-std=c99 -g -O3 -D DIM=$(DIM) -D GRAIN=$(GRAIN) -D P=$(P)

CC=gcc

all: premier-code worker with-depend seq-with-depend


worker: 
	$(CC) $(FLAGS) -lpthread ordonnanceur.c worker.c -o worker-$(DIM)-$(GRAIN)
	$(CC) $(FLAGS) -DFIRST_TOUCH -lpthread ordonnanceur.c worker.c -o worker-$(DIM)-$(GRAIN)-FT


with-depend: 
	$(CC) -fopenmp $(FLAGS) with-depend.c -o with-depend-$(DIM)-$(GRAIN)
	$(CC) -DFIRST_TOUCH -fopenmp $(FLAGS) with-depend.c -o with-depend-$(DIM)-$(GRAIN)-FT


seq-with-depend: with-depend.c
	$(CC) $(FLAGS) with-depend.c -o seq-with-depend-$(DIM)-$(GRAIN)

premier-code: 
	$(CC) $(FLAGS) premier-code.c -o premier-code-$(DIM)

clean:
	rm -rf *.o worker-* with-depend-* premier-code-* seq-with-depend-* *~
