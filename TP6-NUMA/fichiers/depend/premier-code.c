#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define TIME_DIFF(t1, t2) \
  ((t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec))

#define MAX(a,b) ((a)>(b)?(a):(b))


#ifndef DIM
#define DIM 8191
#endif

int image[DIM+1][DIM+1];

#include "spirale.c"

void first_touch()
{
  int i,j ;

  for(i = 0; i<=DIM ; i++)
    {
      for(j = 1; j<=DIM ; j+=512)
	image[i][j]= 0 ;
    }
}


void identifier_pixel()
{
  int i,j;
  for(i = 1; i<DIM ; i++)
    for(j = 1; j<DIM ; j++)
      image[i][j]=image[i][j]*(i*DIM+j);
}


void 
afficher()
{
  int i,j;
  for (i=0; i < DIM; i++)
    {
      for(j=0; j < DIM; j++)
	printf("%4d ", image[i][j]);
      putchar('\n');
    }
  
  putchar('\n');
}



int monter_max(int i_d, int j_d, int i_f, int j_f)
{
  int i,j;
  int changement = 0;
  for (i=i_d; i >= i_f; i--)
      for (j=j_d; j >= j_f; j--)
	if (image[i][j])
	  { 
	    int m;
	    m = MAX(image[i+1][j] , image[i][j+1]);
	    if (m > image[i][j])
	      {	    
		changement = 1;
		image[i][j] = m;
	      }
	  }
  return changement;
}


int descendre_max(int i_d, int j_d, int i_f, int j_f)
{
  int i,j;
  int changement = 0;
  for (i=i_d; i <= i_f; i++)
    for (j=j_d; j <= j_f; j++)
      { 
	if (image[i][j])
	  {
	    int m = MAX(image[i-1][j] , image[i][j-1]);
	    if (m > image[i][j])
	      {	    
		changement = 1;
		image[i][j] = m;
	      }
	  }
      }
  return changement;
}


int main()
{
  struct timeval tv1,tv2;
  regulier(1,DIM,1,DIM,2,100);
   identifier_pixel();

  int i=0;
  gettimeofday(&tv1,NULL);
  while(descendre_max(1,1,DIM,DIM) | monter_max(DIM,DIM,1,1))
    {
      //      printf("********* %d *********\n",i++);
      //      afficher();
    }
  gettimeofday(&tv2,NULL);
  printf("%f ms\n", ((float)TIME_DIFF(tv1,tv2)) / 1000);  
}
