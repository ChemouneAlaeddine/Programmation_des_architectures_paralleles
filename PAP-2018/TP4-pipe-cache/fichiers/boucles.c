#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include <time.h>

#define TIME_DIFF(t1, t2) \
		((t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec))

#define MESURES 10
#define N 256
typedef float type;
#define format "%f"
int main(void) {
	type sum, sum2;
	type *c, *d;
	int i, j, mesure;
	struct timeval tv1,tv2;

	c = malloc(N*sizeof(*c));
	d = malloc(N*sizeof(*d));
	for (i=0; i<N; i++)
		c[i] = random();
	for (i=0; i<N; i++)
		d[i] = random();
	
	for (mesure=0; mesure<MESURES; mesure++) {
		gettimeofday(&tv1,NULL);
		sum = 0;
		sum2 = 0;
		for (j=0; j<2000000; j++) {
			for (i=0; i<N; i++)
				sum += c[i];
			for (i=0; i<N; i++)
				sum2 += d[i];
		}
		gettimeofday(&tv2,NULL);
		printf("sum "format" sum2 "format" %.3f sec\n", sum, sum2, ((float)(TIME_DIFF(tv1,tv2)))/1000000);
	}

	return 0;
}
