#include "global.h"
#include "compute.h"
#include "graphics.h"
#include "debug.h"
#include "ocl.h"
#include "scheduler.h"

#include <stdbool.h>

static long unsigned int *TABLE=NULL;

static volatile int changement ;

static unsigned long int max_grains;

#define table(i,j) TABLE[(i)*DIM+(j)]

#define RGB(r,v,b) (((r)<<24|(v)<<16|(b)<<8))

void sable_init()
{
  TABLE=calloc(DIM*DIM,sizeof(long unsigned int));
}

void sable_finalize()
 {
   free(TABLE);
 }


///////////////////////////// Production d'une image 
void sable_refresh_img()
{
  unsigned long int max = 0;
    for (int i = 1; i < DIM-1; i++)
      for (int j = 1; j < DIM-1; j++)
	{
	  int g =table(i,j);
	  int r,v,b;
	  r = v = b = 0;
	    if ( g == 1)
	      v=255;
	    else if (g == 2)
	      b = 255;
	    else if (g == 3)
	      r = 255;
	    else if (g == 4)
	      r = v = b = 255 ;
	    else if (g > 4)
	      r = b = 255 - (240 * ((double) g) / (double) max_grains);
	    
	    cur_img (i,j) = RGB(r,v,b);
	    if (g > max)
	      max = g;
	}
    max_grains = max;
}



///////////////////////////// Configurations initiales

static void sable_draw_4partout(void);

void sable_draw (char *param)
{
  char func_name [1024];
  void (*f)(void) = NULL;
  
  sprintf (func_name, "draw_%s", param);
  f = dlsym (DLSYM_FLAG, func_name);
  
  if (f == NULL) {
    printf ("Cannot resolve draw function: %s\n", func_name);
    f = sable_draw_4partout;
  }
  
  f ();
}


void sable_draw_4partout(void){
  max_grains = 8;
  for (int i=1; i < DIM-1; i++)
    for(int j=1; j < DIM-1; j++)
      table (i, j) = 4;
}


void sable_draw_DIM(void){
  max_grains = DIM;
   for (int i=DIM/4; i < DIM-1; i+=DIM/4)
     for(int j=DIM/4; j < DIM-1; j+=DIM/4)
       table (i, j) = i*j/4;
}


 void sable_draw_alea(void){
   max_grains = DIM;
 for (int i= 0; i < DIM>>2; i++)
   {
     table (1+random() % (DIM-2) , 1+ random() % (DIM-2)) = random() % (DIM);
   }
}


///////////////////////////// Version séquentielle simple (seq)
//#pragma GCC push_options
//#pragma GCC optimize ("unroll-all-loops")

static inline void compute_new_state (int y, int x)
{
  //if (table(y,x) >= 4){
    unsigned long int div4 = table(y,x) / 4;
    table(y,x-1)+=div4;
    table(y,x+1)+=div4;
    table(y-1,x)+=div4;
    table(y+1,x)+=div4;
    table(y,x)%=4;
    changement = 1;
  //}
}

static void traiter_tuile (int i_d, int j_d, int i_f, int j_f)
{
  PRINT_DEBUG ('c', "tuile [%d-%d][%d-%d] traitée\n", i_d, i_f, j_d, j_f);
  
  for (int i = i_d; i <= i_f; i++)
    for (int j = j_d; j <= j_f; j++){
      if (table(i,j) >= 4)
        compute_new_state (i,j);
    }
}

 static void traiter_tuile_v1 (int i_d, int j_d, int i_f, int j_f)
 {
   PRINT_DEBUG ('c', "tuile [%d-%d][%d-%d] traitée\n", i_d, i_f, j_d, j_f);
 
   int rest = (j_f - j_d + 1) % 2;
 
   for (int i = i_d; i <= i_f; i++){
     for (int j = j_d; j <= (j_f - rest); j+=2){
      if (table(i,j) >= 4)
       compute_new_state (i, j);
      if (table(i,j+1) >= 4)
       compute_new_state(i,j+1);
    }
    for(int j = (j_f - rest); j <= j_f; ++j)
      if (table(i,j) >= 4) 
        compute_new_state(i,j);
   }
 }

// Renvoie le nombre d'itérations effectuées avant stabilisation, ou 0
 unsigned sable_compute_seq (unsigned nb_iter)
{
  changement = 0;
  for (unsigned it = 1; it <= nb_iter; it ++) {
    // On traite toute l'image en un coup (oui, c'est une grosse tuile)
    traiter_tuile (1, 1, DIM - 2, DIM - 2);
    if(changement == 0)
      return it;
  }
  return 0;
}

unsigned sable_compute_seqv1 (unsigned nb_iter)
{
  changement = 0;
  for (unsigned it = 1; it <= nb_iter; it ++) {
    // On traite toute l'image en un coup (oui, c'est une grosse tuile)
    traiter_tuile_v1 (1, 1, DIM - 2, DIM - 2);
    if(changement == 0)
      return it;
  }
  return 0;
}
///////////////////////////// Version parallèl

static void traiter_tuile_omp (int i_d, int j_d, int i_f, int j_f)
{
  int mat[DIM][DIM+1];

  PRINT_DEBUG ('c', "tuile [%d-%d][%d-%d] traitée\n", i_d, i_f, j_d, j_f);
  #pragma omp parallel
  #pragma omp single
  {
    for (int i = i_d; i <= i_f; i++)
        for (int j = j_d; j <= j_f; j++){
            if (table(i,j) >= 4){
              #pragma omp task firstprivate(i,j) depend(in:mat[i-1][j],mat[i][j-1])  depend(out:mat[i][j])
              compute_new_state(i,j);
            }
          }
  }
}

static void traiter_tuile_omp_tuile (int i_d, int j_d, int i_f, int j_f)
{
  int M = 8;
  int mat[DIM/M][DIM/M];

  int rest = (DIM-2)%M;
  PRINT_DEBUG ('c', "tuile [%d-%d][%d-%d] traitée\n", i_d, i_f, j_d, j_f);
  #pragma omp parallel //for schedule(dynamic)
  #pragma omp single
  for(int I = i_d; I <= i_f-rest; I+=M)
    for(int J = j_d; J <= j_f-rest; J+=M)
      //#pragma omp task firstprivate(I,J) depend(in:mat[I-1][J],mat[I][J-1]) depend(out:mat[I][J])
      for (int i = I; i < I+M && i <= i_f; i++)
        for (int j = J; j < J+M && j <= j_f; j++){
          if (table(i,j) >= 4){
            #pragma omp task firstprivate(i,j) depend(in:mat[I-1][J],mat[I][J-1]) depend(out:mat[I][J])//depend(in:mat[i-1][j],mat[i][j-1]) depend(out:mat[i][j])
            compute_new_state(i,j);
          }
        }
  if(rest > 0){
    for (int j = j_d; j <= j_f; j++)
      for (int i = i_f-rest+1; i <= i_f; i++){
        if (table(j,i) >= 4)
          compute_new_state(j,i);
        if (table(i,j) >= 4)
          compute_new_state(i,j);
      }
  }
}


unsigned sable_compute_omp (unsigned nb_iter)
{
  changement = 0;
  for (unsigned it = 1; it <= nb_iter; it ++) {
    // On traite toute l'image en un coup (oui, c'est une grosse tuile)
    traiter_tuile_omp_tuile (1, 1, DIM - 2, DIM - 2);
    if(changement == 0)
      return it;
  }
  return 0;
}


///////////////////////////// Version séquentielle tuilée (tiled)


static unsigned tranche = 0;

unsigned sable_compute_tiled (unsigned nb_iter)
{
  tranche = DIM / GRAIN;
  
  for (unsigned it = 1; it <= nb_iter; it ++) {

    // On itére sur les coordonnées des tuiles
    for (int i=0; i < GRAIN; i++)
      for (int j=0; j < GRAIN; j++)
	{
	  traiter_tuile (i == 0 ? 1 : (i * tranche) /* i debut */,
			 j == 0 ? 1 : (j * tranche) /* j debut */,
			 (i + 1) * tranche - 1 - (i == GRAIN-1)/* i fin */,
			 (j + 1) * tranche - 1 - (j == GRAIN-1)/* j fin */);
	}
  }
  
  return 0;
}


///////////////////////////// Version utilisant un ordonnanceur maison (sched)

unsigned P;

void sable_init_sched ()
{
  sable_init();
  P = scheduler_init (-1);
}

void sable_finalize_sched ()
{
  sable_finalize();
  scheduler_finalize ();
}

static inline void *pack (int i, int j)
{
  uint64_t x = (uint64_t)i << 32 | j;
  return (void *)x;
}

static inline void unpack (void *a, int *i, int *j)
{
  *i = (uint64_t)a >> 32;
  *j = (uint64_t)a & 0xFFFFFFFF;
}

static inline unsigned cpu (int i, int j)
{
  return 1;
}

static inline void create_task (task_func_t t, int i, int j)
{
  scheduler_create_task (t, pack (i, j), cpu (i, j));
}

//////// First Touch

static void zero_seq (int i_d, int j_d, int i_f, int j_f)
{

  for (int i = i_d; i <= i_f; i++)
    for (int j = j_d; j <= j_f; j++)
      next_img (i, j) = cur_img (i, j) = 0 ;
}

static void first_touch_task (void *p, unsigned proc)
{
  int i, j;

  unpack (p, &i, &j);

  //PRINT_DEBUG ('s', "First-touch Task is running on tile (%d, %d) over cpu #%d\n", i, j, proc);
  zero_seq (i * tranche, j * tranche, (i + 1) * tranche - 1, (j + 1) * tranche - 1);
}

void sable_ft_sched (void)
{
  tranche = DIM / GRAIN;

  for (int i = 0; i < GRAIN; i++)
    for (int j = 0; j < GRAIN; j++)
      create_task (first_touch_task, i, j);

  scheduler_task_wait ();
}

//////// Compute

static void compute_task (void *p, unsigned proc)
{
  int i, j;

  unpack (p, &i, &j);
  
  //PRINT_DEBUG ('s', "Compute Task is running on tile (%d, %d) over cpu #%d\n", i, j, proc);
	  traiter_tuile (i == 0 ? 1 : (i * tranche) /* i debut */,
			 j == 0 ? 1 : (j * tranche) /* j debut */,
			 (i + 1) * tranche - 1 - (i == GRAIN-1)/* i fin */,
			 (j + 1) * tranche - 1 - (j == GRAIN-1)/* j fin */);
}

unsigned sable_compute_sched (unsigned nb_iter)
{
  tranche = DIM / GRAIN;

  for (unsigned it = 1; it <= nb_iter; it ++) {
    changement=0;
    for (int i = 0; i < GRAIN; i++)
      for (int j = 0; j < GRAIN; j++)
	create_task (compute_task, i, j);
    
    scheduler_task_wait ();

    if (changement == 0)
      return it;
  }
  
  return 0;
}

