#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include <time.h>

#define TIME_DIFF(t1, t2) \
		((t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec))

#define MESURES 10
#define N 2048
int main(void) {
	long int sum;
	long int *c;
	int i, j, mesure;
	struct timeval tv1,tv2;

	c = malloc(N*sizeof(*c));
	for (i=0; i<N; i++)
		c[i] = random();
	
	for (mesure=0; mesure<MESURES; mesure++) {
		gettimeofday(&tv1,NULL);
		sum = 0;
		for (j=0; j<2000000; j++) {
#define D 1
			for (i=0; i<N; i+=D) {
				sum += c[i];
			}
		}
		gettimeofday(&tv2,NULL);
		printf("sum %ld %.3f sec\n", sum, ((float)(TIME_DIFF(tv1,tv2)))/1000000);
	}

	return 0;
}
