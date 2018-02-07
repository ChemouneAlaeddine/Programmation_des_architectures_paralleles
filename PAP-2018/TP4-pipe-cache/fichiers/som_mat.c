#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>

#define N 8192


#define TIME_DIFF(t1, t2) \
  ((double)((t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec)))

char a[N][N];
char b[N][N];
char c[N][N];

void somMat1() {
	int i,j;
	for(j=0;j<N;j++) {
		for(i=0;i<N;i++) {
			c[i][j]=a[i][j]&b[i][j];			
		}
	}
}


void somMat2() {
	int i,j;
	for(j=0;j<N;j++) {
	  for(i=0;i<N;i++) {
	    c[i][j]=a[i][j]&b[i][j];			
		}
	}
}


int main(int argc, char ** argv){

  struct timeval t1,t2,t3;
	int i,j;
	for(i=0;i<N;i++)
		for(j=0;j<N;j++){			
			a[i][j]=i+j;
			b[i][j]=i-j;
		}

        gettimeofday(&t1,NULL);
	somMat1();
        gettimeofday(&t2,NULL);
	somMat2();
        gettimeofday(&t3,NULL);

	printf("%g / %g  = acceleration = %g\n", TIME_DIFF(t1,t2) / 1000,  TIME_DIFF(t2,t3) / 1000, TIME_DIFF(t1,t2)/TIME_DIFF(t2,t3));

	return 0;
}
