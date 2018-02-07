#include <stdio.h>
#include <stdlib.h>

#include <sys/time.h>
/* macro de mesure de temps, retourne une valeur en µsecondes */
#define TIME_DIFF(t1, t2) \
  ((t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec))


#define MAX(a,b) ((a)>(b)?(a):(b))


#ifndef DIM
#define DIM 8191
#endif

#ifndef GRAIN
#define GRAIN 32
#endif


int image[DIM+1][DIM+1];


volatile int celluled[GRAIN][GRAIN];
volatile int cellulem[GRAIN][GRAIN];

volatile int cont = 0;

#include "spirale.c"


void first_touch()
{
  int i,j ;

#pragma omp parallel for
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

  printf("CONTINUE : %d",cont);
  
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


#define TRANCHE ((DIM+1)/GRAIN)


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


void 
lancer_descente(int i, int j)
{
  int i_d = (i == 0) ? 1 : i * TRANCHE ;
  int j_d = (j == 0) ? 1 : j * TRANCHE ;
  int i_f = (i == GRAIN-1) ? DIM-1 : (i+1) * TRANCHE-1 ;
  int j_f = (j == GRAIN-1) ? DIM-1 : (j+1) * TRANCHE-1 ;

  if(descendre_max(i_d,j_d,i_f,j_f))
    cont = 1;
}

void
lancer_monte(int i, int j)
{

  int i_d = (i == GRAIN-1) ? DIM-1 : (i+1) * TRANCHE-1 ;
  int j_d = (j == GRAIN-1) ? DIM-1 : (j+1) * TRANCHE-1 ;
  int i_f = (i == 0) ? 1 : i * TRANCHE ;
  int j_f = (j == 0) ? 1 : j * TRANCHE ;
  
  if (monter_max(i_d,j_d,i_f,j_f))
	cont = 1;
}


int 
main()
{


  unsigned long temps;
  struct timeval t1, t2;
    
  int parite = 0;
#ifdef FIRST_TOUCH
  first_touch();
#endif
  regulier(1,DIM,1,DIM,2,100);
  identifier_pixel();
  gettimeofday(&t1,NULL);
  
#pragma omp parallel shared(cont) // DESCENTE
#pragma omp single
  do {

      cont = 0; 
      for (int i=0; i < GRAIN; i++ )
	for (int j=0; j < GRAIN; j++ )
	  if (i==0 && j==0)
#pragma omp task depend(out : celluled[0][0])  firstprivate(i,j)
	    lancer_descente(i,j);
	  else if (j == 0)
#pragma omp task depend(in : celluled[i-1][0]) depend(out:celluled[i][j])  firstprivate(i,j)
	    lancer_descente(i,j);
	  else if (i == 0)
#pragma omp task depend(in : celluled[0][j-1]) depend(out:celluled[i][j])  firstprivate(i,j)
	    lancer_descente(i,j);
      	  else if (i == GRAIN -1 && j == GRAIN -1) 
#pragma omp task depend(in : celluled[i-1][j],celluled[i][j-1])   firstprivate(i,j)
	    lancer_descente(i,j);
	  else 
#pragma omp task depend(in : celluled[i-1][j],celluled[i][j-1]) depend(out:celluled[i][j])  firstprivate(i,j)
	    lancer_descente(i,j);
#pragma omp taskwait
      
      for (int i=0; i < GRAIN; i++ )
	for (int j=0; j < GRAIN; j++ )
	  if (i==0 && j==0)
#pragma omp task depend(out : celluled[0][0])  firstprivate(i,j)
	    lancer_monte(GRAIN-i-1,GRAIN-j-1);
	  else if (j == 0)
#pragma omp task depend(in : celluled[i-1][0]) depend(out:celluled[i][j])  firstprivate(i,j)
	    lancer_monte(GRAIN-i-1,GRAIN-j-1);
	  else if (i == 0)
#pragma omp task depend(in : celluled[0][j-1]) depend(out:celluled[i][j])  firstprivate(i,j)
	    lancer_monte(GRAIN-i-1,GRAIN-j-1);
      	  else if (i == GRAIN -1 && j == GRAIN -1) 
#pragma omp task depend(in : celluled[i-1][j],celluled[i][j-1])   firstprivate(i,j)
	    lancer_monte(GRAIN-i-1,GRAIN-j-1);
	  else
#pragma omp task depend(in : celluled[i-1][j],celluled[i][j-1]) depend(out:celluled[i][j])  firstprivate(i,j)
	    lancer_monte(GRAIN-i-1,GRAIN-j-1);
#pragma omp taskwait
   
}  while(cont);

gettimeofday(&t2,NULL);
temps = TIME_DIFF(t1,t2);

printf("time = %ld.%03ldms", temps/1000, temps%1000);

return 0;
}
