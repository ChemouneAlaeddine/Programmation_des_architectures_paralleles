#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

typedef union u_tick {
  unsigned long long tick;
  struct {
    unsigned low;
    unsigned high;
  }z;
} tick_t;

#define GET_TICK(t) \
	   __asm__ volatile("rdtsc" : "=a" ((t).z.low), "=d" ((t).z.high))
#define TICK_DIFF(t1, t2) \
	   ((t2).tick - (t1).tick)

#define N 128
#define N1 1024
#define N2 1280
#define N1N2 (1280 * 1024)

typedef long matrix[N][N1][N2];

long int diff[N];

matrix film;

void fillmatrix(matrix A) {
  int i,j,n;
  for (n=0;n<N;n++)
	for (i=0;i<N1;i++)
		for (j=0;j<N2;j++)
		  A[n][i][j] = i+j+i % (n+1);
}


void difference(){
  for(int n=0; n < N-1; n++)
    for(int i=0; i < 1024; i++)
      for(int j=0; j < 1280; j++)
 	  if (film[n][i][j] != film[n+1][i][j])
	    diff[n]++;
}



int main(void) {
	tick_t t1,t2;

	fillmatrix(film);

	GET_TICK(t1);
	difference();
	GET_TICK(t2);
	printf("original %.3f Mcycles\n",((float)TICK_DIFF(t1,t2))/1000000);


	GET_TICK(t1);
	// placer ici votre appel
	GET_TICK(t2);
	printf("Optimisation :   %.3f Mcycles \n",((float)TICK_DIFF(t1,t2))/1000000);

	return 0;
}
