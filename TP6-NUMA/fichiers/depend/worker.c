#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "ordonnanceur.h"

#ifndef P
#define P 24
#endif

#ifndef DIM
#define DIM 8191
#endif

#ifndef GRAIN
#define GRAIN 64
#endif

int image[DIM+1][DIM+1];


volatile int celluled[GRAIN][GRAIN];
volatile int cellulem[GRAIN][GRAIN];

volatile int cont = 1;

#include "spirale.c"

#define MAX(a,b) ((a)>(b)?(a):(b))


#include <sys/time.h>
/* macro de mesure de temps, retourne une valeur en micro secondes */
#define TIME_DIFF(t1, t2) \
  ((t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec))


void ajouter_job(int i, int j, int sens);
void lancer_descente(int i, int j);
void lancer_monte(int i, int j);


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
  
}

#define TRANCHE ((DIM+1)/GRAIN)

void *monter_max(void *p)
{
  int ii = *(int*)p;
  int jj = *((int*)p +1);

  free(p);

  int i_d = (ii == GRAIN-1) ? DIM-1 : (ii+1) * TRANCHE-1 ;
  int j_d = (jj == GRAIN-1) ? DIM-1 : (jj+1) * TRANCHE-1 ;
  int i_f = (ii == 0) ? 1 : ii * TRANCHE ;
  int j_f = (jj == 0) ? 1 : jj * TRANCHE ;
  
  int changement = 0;
  int i,j;
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
  if(changement)
    cont = 1;

  if (jj > 0)
    lancer_monte(ii,jj-1);
  
  if (ii > 0)
    lancer_monte(ii-1,jj);

  return NULL;
}



void 
lancer_firstTouch(int i, int j)
{

  int val = __sync_add_and_fetch(&celluled[i][j],1);
    
  if ( val != 2 && i != 0 && j != 0)
    {
      return;
    }
  
  celluled[i][j]= 0;
  ajouter_job(i,j,2);
}

void *firstTouch(void *p)
{
  int ii = *(int*)p;
  int jj = *((int*)p +1);
  
  free(p);
  
  int i_d = (ii == 0) ? 1 : ii * TRANCHE ;
  int j_d = (jj == 0) ? 1 : jj * TRANCHE ;
  int i_f = (ii == GRAIN-1) ? DIM-1 : (ii+1) * TRANCHE-1 ;
  int j_f = (jj == GRAIN-1) ? DIM-1 : (jj+1) * TRANCHE-1 ;

  int i,j;
  for (i=i_d; i <= i_f; i++)
    for (j=j_d; j <= j_f; j++)
	image[i][j]=0;
  
  if (jj < GRAIN -1)
    lancer_firstTouch(ii,jj+1);
  if (ii < GRAIN - 1)
    lancer_firstTouch(ii+1,jj);
  return NULL;
}



void *descendre_max(void *p)
{
  int ii = *(int*)p;
  int jj = *((int*)p +1);

  free(p);
  
  int i_d = (ii == 0) ? 1 : ii * TRANCHE ;
  int j_d = (jj == 0) ? 1 : jj * TRANCHE ;
  int i_f = (ii == GRAIN-1) ? DIM-1 : (ii+1) * TRANCHE-1 ;
  int j_f = (jj == GRAIN-1) ? DIM-1 : (jj+1) * TRANCHE-1 ;

  int changement = 0;

  int i,j;
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
  
  if(changement)
    cont = 1;
  
  if (jj < GRAIN -1)
    lancer_descente(ii,jj+1);

  if (ii < GRAIN - 1)
    lancer_descente(ii+1,jj);
  return NULL;
}

void ajouter_job(int i, int j, int sens)
{
  struct job todo;
  int coeur = i % P;
  int *couple = malloc (2 * sizeof(int));
  couple[0]=i;
  couple[1]=j;

  todo.p=couple;

  if (sens == 0)
    todo.fun = descendre_max;
  else if (sens == 1)
    todo.fun = monter_max;
  else
    todo.fun = firstTouch;

  add_job(todo,coeur);
}


void 
lancer_descente(int i, int j)
{
  // celluled gere les dependances entre blocs pour la descente
  // celluled[x][y] == 0 <==> les blocs [x-1][y] et [x][y-1] ne sont pas termines
  // celluled[x][y] == 1 <==> un des blocs [x-1][y] et [x][y-1] a termine
  // celluled[x][y] == 2 <==> les blocs [x-1][y] et [x][y-1] sont termines

  int val = __sync_add_and_fetch(&celluled[i][j],1);

  if ( val != 2 && i != 0 && j != 0)
    {
      return;
    }
  
  celluled[i][j]= 0;
  ajouter_job(i,j,0);
}

void
lancer_monte(int i, int j)
{

  int val = __sync_add_and_fetch(&cellulem[i][j],1);
  
  if (val !=2 && i != GRAIN-1 && j != GRAIN-1)
    return ;
  
  cellulem[i][j]= 0;
  ajouter_job(i,j,1);
}

void * 
my_main(void *p)
{
  struct timeval t1, t2;
#ifdef FIRST_TOUCH
  lancer_firstTouch(0,0);
  task_wait();
#endif
  regulier(1,DIM,1,DIM,2,100);
  identifier_pixel();
  int etape=0;
  gettimeofday(&t1,NULL);
  do {
    cont = 0;
    lancer_monte(GRAIN-1,GRAIN-1);
    task_wait();
    lancer_descente(0,0);
    task_wait();
    //afficher();
    printf("step %d\n",++etape);
  }  while(cont);
    gettimeofday(&t2,NULL);
    long temps = TIME_DIFF(t1,t2);

printf("time = %ld.%03ldms", temps/1000, temps%1000);

  return 0;
}


int main()
{
  
  go(my_main,NULL,P);
  
  return 0;
}
