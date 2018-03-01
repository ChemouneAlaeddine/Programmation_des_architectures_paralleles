
#include "global.h"
#include "compute.h"
#include "graphics.h"
#include "debug.h"
#include "ocl.h"
#include "scheduler.h"

#include <stdbool.h>


///////////////////////////// Version séquentielle simple (seq)


// Renvoie le nombre d'itérations effectuées avant stabilisation, ou 0
unsigned transpose_compute_seq (unsigned nb_iter)
{
  for (unsigned it = 1; it <= nb_iter; it ++) {

    for (int i = 0; i < DIM; i++)
      for (int j = 0; j < DIM; j++)
	next_img (i, j) = cur_img (j, i);

    swap_images ();
  }

  return 0;
}

///////////////////////////// Version séquentielle tuilée (tiled)

#define GRAIN 16

static unsigned tranche = 0;

static void traiter_tuile (int i_d, int j_d, int i_f, int j_f)
{
  PRINT_DEBUG ('c', "tuile [%d-%d][%d-%d] traitée\n", i_d, i_f, j_d, j_f);
  
  for (int i = i_d; i <= i_f; i++)
    for (int j = j_d; j <= j_f; j++)
      next_img (i, j) = cur_img (j, i);
}

unsigned transpose_compute_tiled (unsigned nb_iter)
{
  tranche = DIM / GRAIN;
  
  for (unsigned it = 1; it <= nb_iter; it ++) {

    // On itére sur les coordonnées des tuiles
    for (int i=0; i < GRAIN; i++)
      for (int j=0; j < GRAIN; j++)
	traiter_tuile (i * tranche /* i debut */,
		       j * tranche /* j debut */,
		       (i + 1) * tranche - 1 /* i fin */,
		       (j + 1) * tranche - 1 /* j fin */);

    swap_images ();
  }
  
  return 0;
}

///////////////////////////// Version OpenMP avec omp for (omp)

void transpose_ft_omp ()
{
  int i, j;

#pragma omp parallel for
  for(i=0; i<DIM ; i++) {
    for(j=0; j < DIM ; j += 512)
      next_img (i, j) = cur_img (i, j) = 0;
  }
}

unsigned transpose_compute_omp (unsigned nb_iter)
{
  for (unsigned it = 1; it <= nb_iter; it ++) {
    #pragma omp parallel for collapse(2)
    for (int i = 0; i < DIM; i++)
      for (int j = 0; j < DIM; j++)
  next_img (i, j) = cur_img (j, i);

    swap_images ();
  }

  return 0;
}


///////////////////////////// Version OpenMP avec tuiles (omp_tiled)


unsigned transpose_ft_omp_tiled (unsigned nb_iter)
{
  // TODO
  return 0;
}


unsigned transpose_compute_omp_tiled (unsigned nb_iter)
{
  tranche = DIM / GRAIN;
  
  for (unsigned it = 1; it <= nb_iter; it ++) {
    #pragma omp parallel for collapse(2)
    // On itére sur les coordonnées des tuiles
    for (int i=0; i < GRAIN; i++)
      for (int j=0; j < GRAIN; j++){
        #pragma omp critical
  traiter_tuile (i * tranche /* i debut */,
           j * tranche /* j debut */,
           (i + 1) * tranche - 1 /* i fin */,
           (j + 1) * tranche - 1 /* j fin */);
      }

    swap_images ();
  }
  
  return 0;
}
