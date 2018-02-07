
#include "global.h"
#include "compute.h"
#include "graphics.h"
#include "debug.h"
#include "ocl.h"
#include "scheduler.h"

#include <stdbool.h>


///////////////////////////// Version séquentielle simple (seq)


// Renvoie le nombre d'itérations effectuées avant stabilisation, ou 0
unsigned scrollup_compute_seq (unsigned nb_iter)
{
  for (unsigned it = 1; it <= nb_iter; it ++) {

    for (int i = 0; i < DIM; i++)
      for (int j = 0; j < DIM; j++)
   	next_img (i, j) = (i == DIM - 1) ? cur_img (0, j) : cur_img (i + 1, j);

    swap_images ();
  }

  return 0;
}
