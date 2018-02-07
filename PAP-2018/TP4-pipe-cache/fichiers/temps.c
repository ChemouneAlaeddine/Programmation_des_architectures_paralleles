#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

/*
 *
 * Mesure de temps à l'aide du compteur de cycles
 * (spécifique à x86 et x86_64)
 *
 */

typedef union {
	unsigned long long tick;
	struct {
		unsigned low;
		unsigned high;
	};
} tick_t;

#define GET_TICK(t) \
	   __asm__ volatile("rdtsc" : "=a" ((t).low), "=d" ((t).high))
#define TICK_DIFF(t1, t2) \
	   ((t2).tick - (t1).tick)

/*
 * 
 * Mesure de temps à l'aide de gettimeofday
 *
 */

#define TIME_DIFF(t1, t2) \
	((t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec))

int main(void) {
	tick_t t1,t2;
	struct timeval tv1,tv2;
	int i;
	float sum = 0;

	gettimeofday(&tv1,NULL);
	GET_TICK(t1);
	for (i=0; i<1000000000; i++)
		sum += i;
	GET_TICK(t2);
	gettimeofday(&tv2,NULL);

	printf("somme %.0f\n", sum);
	//printf("%f ms\n", ((float)TIME_DIFF(tv1,tv2)) / 1000);
	//printf("%lld cycles\n", TICK_DIFF(t1,t2));
	return 0;
}
