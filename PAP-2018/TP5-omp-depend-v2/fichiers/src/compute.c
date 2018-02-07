
#include "compute.h"
#include "graphics.h"
#include "debug.h"

propager_max_func_t propager_max[] = {
  propager_max_v0,
  propager_max_v1,
  propager_max_v2,
  propager_max_v3
};

char *version_propagation[] = {
  "Propagation séquentielle simple",
  "Propagation séquentielle par tuiles",
  "Propagation OpenMP avec omp for",
  "Propagation OpenMP avec tâches"
};

///////////////////////////// Version séquentielle simple

// NE PAS MODIFIER

int monter_max_seq (int i_d, int j_d, int i_f, int j_f)
{
  int changement = 0;

  for (int i = i_d; i >= i_f; i--)
    for (int j = j_d; j >= j_f; j--)
      if (img(i,j)) {
	Uint32 m = MAX(img(i+1,j), img(i,j+1));
	if (m > img(i,j)) {
	  changement = 1;
	  img(i,j) = m;
	}
      }

  return changement;
}

int descendre_max_seq (int i_d, int j_d, int i_f, int j_f)
{
  int changement = 0;

  for (int i = i_d; i <= i_f; i++)
    for (int j = j_d; j <= j_f; j++)
      if (img(i,j)) {
	Uint32 m = MAX(img(i-1,j), img(i,j-1));
	if (m > img(i,j)) {   
	  changement = 1;
	  img(i,j) = m;
	}
      }

  return changement;
}

int propager_max_v0 (void)
{
  return monter_max_seq (DIM-2, DIM-2, 0, 0) | descendre_max_seq (1, 1, DIM-1, DIM-1);
}

///////////////////////////// Version séquentielle par tuiles

#define GRAIN 32

// NE PAS MODIFIER (sauf GRAIN)

volatile int celluled[GRAIN][GRAIN];
volatile int cellulem[GRAIN][GRAIN];

volatile int cont = 0;

unsigned tranche = 0;

void lancer_descente (int i, int j)
{
  int i_d = (i == 0) ? 1 : i * tranche;
  int j_d = (j == 0) ? 1 : j * tranche;
  int i_f = (i == GRAIN-1) ? DIM-1 : (i+1) * tranche - 1;
  int j_f = (j == GRAIN-1) ? DIM-1 : (j+1) * tranche - 1;

  PRINT_DEBUG ('t', "Decente: bloc(%d,%d) couvre (%d,%d)-(%d,%d)\n", i, j, i_d, j_d, i_f, j_f);
  
  if (descendre_max_seq (i_d, j_d, i_f, j_f))
    cont = 1;
}

void lancer_monte (int i, int j)
{

  int i_d = (i == GRAIN-1) ? DIM-2 : (i+1) * tranche - 1;
  int j_d = (j == GRAIN-1) ? DIM-2 : (j+1) * tranche - 1;
  int i_f = i * tranche;
  int j_f = j * tranche;

  if (monter_max_seq (i_d, j_d, i_f, j_f))
    cont = 1;
}

int propager_max_v1 (void)
{    
  tranche = DIM / GRAIN;

  cont = 0;

  for (int i=0; i < GRAIN; i++)
    for (int j=0; j < GRAIN; j++)
      lancer_monte (GRAIN-i-1, GRAIN-j-1);

  for (int i=0; i < GRAIN; i++)
    for (int j=0; j < GRAIN; j++)
      lancer_descente (i, j);

  return cont;
}

///////////////////////////// Version OpenMP avec omp for

int monter_max (int i_d, int j_d, int i_f, int j_f)
{
  int changement = 0;

  // TODO
  for (int i = i_d; i >= i_f; i--)
    for (int j = j_d; j >= j_f; j--)
      if (img(i,j)) {
	Uint32 m = MAX(img(i+1,j), img(i,j+1));
	if (m > img(i,j)) {
	  changement = 1;
	  img(i,j) = m;
	}
      }

  return changement;
}

int descendre_max (int i_d, int j_d, int i_f, int j_f)
{
  int changement = 0;

  // TODO
  for (int i = i_d; i <= i_f; i++)
    for (int j = j_d; j <= j_f; j++)
      if (img(i,j)) {
	Uint32 m = MAX(img(i-1,j), img(i,j-1));
	if (m > img(i,j)) {   
	  changement = 1;
	  img(i,j) = m;
	}
      }

  return changement;
}

int propager_max_v2 (void)
{
  return monter_max (DIM-2, DIM-2, 0, 0) | descendre_max (1, 1, DIM-1, DIM-1);
}


///////////////////////////// Version OpenMP avec tâches


int propager_max_v3 (void)
{    
  tranche = DIM / GRAIN;

  cont = 0;

  // TODO
  {
  for (int i=0; i < GRAIN; i++)
    for (int j=0; j < GRAIN; j++)
      lancer_monte (GRAIN-i-1, GRAIN-j-1);

  
  for (int i=0; i < GRAIN; i++)
    for (int j=0; j < GRAIN; j++)
      lancer_descente (i, j);
  }
  
  return cont;
}
