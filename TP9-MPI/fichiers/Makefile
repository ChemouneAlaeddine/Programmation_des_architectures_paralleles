CC=mpicc.mpich
MPIRUN=mpirun.mpich 
MACHINES=mymachines
CFLAGS=-g -Wall
SOURCE=$(wildcard *.c)
OBJ=$(SOURCE:.c=.o)
PROG?=hellow
N?=2

all: $(PROG)

clean:
	rm -f $(OBJ) $(PROG)

run: $(PROG)
	$(MPIRUN) --hostfile $(MACHINES) --map-by node -np $N ./$(PROG)
