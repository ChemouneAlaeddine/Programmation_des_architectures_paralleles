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
  if (table(y,x) >= 4){
    unsigned long int div4 = table(y,x) / 4;
    table(y,x-1)+=div4;
    table(y,x+1)+=div4;
    table(y-1,x)+=div4;
    table(y+1,x)+=div4;
    table(y,x)%=4;
    changement = 1;
  }
}


static inline void compute_new_state_atomic (int y, int x)
{
  if (table(y,x) >= 4){
    unsigned long int div4 = table(y,x) / 4;
    #pragma omp atomic write
    table(y,x-1)=table(y,x-1)+div4;
    #pragma omp atomic write
    table(y,x+1)=table(y,x+1)+div4;
    #pragma omp atomic write
    table(y-1,x)=table(y-1,x)+div4;
    #pragma omp atomic write
    table(y+1,x)=table(y+1,x)+div4;
    #pragma omp atomic write
    table(y,x)=table(y,x)%4;
    #pragma omp atomic write
    changement = 1;
  }
}


static void traiter_tuile (int i_d, int j_d, int i_f, int j_f)
{
  PRINT_DEBUG ('c', "tuile [%d-%d][%d-%d] traitée\n", i_d, i_f, j_d, j_f);
  
  for (int i = i_d; i <= i_f; i++)
    for (int j = j_d; j <= j_f; j++){
        compute_new_state(i,j);
    }
}

static void traiter_tuile_atomic (int i_d, int j_d, int i_f, int j_f)
{
  PRINT_DEBUG ('c', "tuile [%d-%d][%d-%d] traitée\n", i_d, i_f, j_d, j_f);
  
  for (int i = i_d; i <= i_f; i++)
    for (int j = j_d; j <= j_f; j++){
        compute_new_state_atomic(i,j);
    }
}

//#pragma GCC push_options
//#pragma GCC optimize("unroll-all-loops")
static void traiter_tuile_double (int i_d, int j_d, int i_f, int j_f)
{
  PRINT_DEBUG ('c', "tuile [%d-%d][%d-%d] traitée\n", i_d, i_f, j_d, j_f);

  for (int i = i_d; i <= i_f/2; i++){
    for (int j = j_d; j <= j_f; j++){
        compute_new_state (i,j);
        compute_new_state (i_f-i+i_d,j);
    }
  }
}
//#pragma GCC pop_options

static void traiter_tuile_double_omp (int i_d, int j_d, int i_f, int j_f)
{
  PRINT_DEBUG ('c', "tuile [%d-%d][%d-%d] traitée\n", i_d, i_f, j_d, j_f);
  /*#pragma omp parallel num_threads(2)
  {
  #pragma omp single
  #pragma omp task*/
  int M = 3;
  int rest = DIM%M;
  int div = DIM/M;

  //#pragma omp parallel for schedule(dynamic)
  //#pragma omp single
  for(int a = 1; a <= div; a++){
    //#pragma omp task firstprivate(a)
    for (int i = i_d; i <= (i_d+(a*M))+(M) && i <= i_f; i++){
      for (int j = j_d; j <= j_f; j++){
        //#pragma omp critical
          compute_new_state (i,j);
      }
    }
  }

  if(rest > 0){
  for (int i = i_f-rest; i <= i_f; i++)
    for (int j = j_d; j <= j_f; j++){
        compute_new_state (i,j);
    }
  }
}

 static void traiter_tuile_deroule2 (int i_d, int j_d, int i_f, int j_f)
 {
   PRINT_DEBUG ('c', "tuile [%d-%d][%d-%d] traitée\n", i_d, i_f, j_d, j_f);
 
   int rest = (j_f - j_d + 1) % 2;
 
   for (int i = i_d; i <= i_f; i++){
     for (int j = j_d; j <= (j_f - rest); j+=2){
       compute_new_state(i, j);
       compute_new_state(i,j+1);
    }
    for(int j = (j_f - rest); j <= j_f; ++j)
        compute_new_state(i,j);
   }
 }

static void traiter_tuile_deroule3 (int i_d, int j_d, int i_f, int j_f)
 {
   PRINT_DEBUG ('c', "tuile [%d-%d][%d-%d] traitée\n", i_d, i_f, j_d, j_f);
 
   int rest = (j_f - j_d + 1) % 3;
 
   for (int i = i_d; i <= i_f; i++){
     for (int j = j_d; j <= (j_f - rest); j+=3){
       compute_new_state(i, j);
       compute_new_state(i,j+1);
       compute_new_state(i,j+2);
    }

    for(int j = (j_f - rest); j <= j_f; j++)
        compute_new_state(i,j);
   }
 }

  static void traiter_tuile_deroule4 (int i_d, int j_d, int i_f, int j_f)
 {
   PRINT_DEBUG ('c', "tuile [%d-%d][%d-%d] traitée\n", i_d, i_f, j_d, j_f);
 
   int rest = (j_f - j_d + 1) % 4;
 
   for (int i = i_d; i <= i_f; i++){
     for (int j = j_d; j <= (j_f - rest); j+=4){
       compute_new_state(i, j);
       compute_new_state(i,j+1);
       compute_new_state(i,j+2);
       compute_new_state(i,j+3);
    }
    for(int j = (j_f - rest); j <= j_f; ++j)
        compute_new_state(i,j);
   }
 }

 static void traiter_tuile_deroule5 (int i_d, int j_d, int i_f, int j_f)
 {
   PRINT_DEBUG ('c', "tuile [%d-%d][%d-%d] traitée\n", i_d, i_f, j_d, j_f);
 
   int rest = (j_f - j_d + 1) % 5;
 
   for (int i = i_d; i <= i_f; i++){
     for (int j = j_d; j <= (j_f - rest); j+=5){
       compute_new_state(i, j);
       compute_new_state(i,j+1);
       compute_new_state(i,j+2);
       compute_new_state(i,j+3);
       compute_new_state(i,j+4);
    }
    for(int j = (j_f - rest); j <= j_f; ++j)
        compute_new_state(i,j);
   }
 }

 static void traiter_tuile_task (int i_d, int j_d, int i_f, int j_f)
{
  PRINT_DEBUG ('c', "tuile [%d-%d][%d-%d] traitée\n", i_d, i_f, j_d, j_f);  

  #pragma omp parallel
  #pragma omp single
  {   
  int a,b,c;
        
  for (int j = j_d; j <= j_f-2; j=j+3){
      
      #pragma omp task firstprivate(j) depend(out:a)
      for (int i = i_d; i <= i_f; i++){
        compute_new_state (j, i);
      }
      
      #pragma omp task firstprivate(j) depend(in:a) depend(out:b)
      for (int i = i_d; i <= i_f; i++){
        compute_new_state (j+1, i);
      }
      
      #pragma omp task firstprivate(j) depend(in:a,b) depend(out:c)
      for (int i = i_d; i <= i_f; i++){
        compute_new_state (j+2, i);
      }
    } 
  }
}

static void traiter_tuile_for (int i_d, int j_d, int i_f, int j_f)
{
  PRINT_DEBUG ('c', "tuile [%d-%d][%d-%d] traitée\n", i_d, i_f, j_d, j_f);
  #pragma omp parallel for collapse(2)
  for (int i = i_d; i <= i_f; i++)
    for (int j = j_d; j <= j_f; j++){
      #pragma omp critical
        compute_new_state(i,j);
    }
}

static void traiter_tuile_alea_omp_for (int i_d, int j_d, int i_f, int j_f)
{
  PRINT_DEBUG ('c', "tuile [%d-%d][%d-%d] traitée\n", i_d, i_f, j_d, j_f);
  #pragma omp parallel for collapse(2) schedule(dynamic)
  for (int i = i_d; i <= i_f; i++)
    for (int j = j_d; j <= j_f; j++){
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

unsigned sable_compute_seq_double (unsigned nb_iter)
{
  changement = 0;
  for (unsigned it = 1; it <= nb_iter; it ++) {
    // On traite toute l'image en un coup (oui, c'est une grosse tuile)
    traiter_tuile_double (1, 1, DIM - 2, DIM - 2);
    if(changement == 0)
      return it;
  }
  return 0;
}

unsigned sable_compute_seq_double_omp (unsigned nb_iter)
{
  changement = 0;
  for (unsigned it = 1; it <= nb_iter; it ++) {
    // On traite toute l'image en un coup (oui, c'est une grosse tuile)
    traiter_tuile_double_omp (1, 1, DIM - 2, DIM - 2);
    if(changement == 0)
      return it;
  }
  return 0;
}


 unsigned sable_compute_alea_omp_for (unsigned nb_iter)
{
  changement = 0;
  for (unsigned it = 1; it <= nb_iter; it ++) {
    // On traite toute l'image en un coup (oui, c'est une grosse tuile)
    traiter_tuile_alea_omp_for (1, 1, DIM - 2, DIM - 2);
    if(changement == 0)
      return it;
  }
  return 0;
}


unsigned sable_compute_seq_deroule2 (unsigned nb_iter)
{
  changement = 0;
  for (unsigned it = 1; it <= nb_iter; it ++) {
    // On traite toute l'image en un coup (oui, c'est une grosse tuile)
    traiter_tuile_deroule2 (1, 1, DIM - 2, DIM - 2);
    if(changement == 0)
      return it;
  }
  return 0;
}

unsigned sable_compute_seq_deroule3 (unsigned nb_iter)
{
  changement = 0;
  for (unsigned it = 1; it <= nb_iter; it ++) {
    // On traite toute l'image en un coup (oui, c'est une grosse tuile)
    traiter_tuile_deroule3 (1, 1, DIM - 2, DIM - 2);
    if(changement == 0)
      return it;
  }
  return 0;
}

unsigned sable_compute_seq_deroule4 (unsigned nb_iter)
{
  changement = 0;
  for (unsigned it = 1; it <= nb_iter; it ++) {
    // On traite toute l'image en un coup (oui, c'est une grosse tuile)
    traiter_tuile_deroule4 (1, 1, DIM - 2, DIM - 2);
    if(changement == 0)
      return it;
  }
  return 0;
}

unsigned sable_compute_seq_deroule5 (unsigned nb_iter)
{
  changement = 0;
  for (unsigned it = 1; it <= nb_iter; it ++) {
    // On traite toute l'image en un coup (oui, c'est une grosse tuile)
    traiter_tuile_deroule5 (1, 1, DIM - 2, DIM - 2);
    if(changement == 0)
      return it;
  }
  return 0;
}
///////////////////////////// Version parallèl


//========================== max =================================


static void traiter_tuile_omp_for (int i_d, int j_d, int i_f, int j_f){
        
    int reste = DIM%3;  
    #pragma omp parallel for schedule(dynamic)
      for (int i = i_d; i <= i_f-2; i=i+3){
        for (int j = j_d; j <= j_f; j++){
          compute_new_state (i, j);
        }
      }
      
      #pragma omp parallel for schedule(dynamic)
      for (int i = i_d+1; i <= i_f-2; i=i+3){
        for (int j = j_d; j <= j_f; j++){
          compute_new_state (i, j);
        }
      }
      
      #pragma omp parallel for schedule(dynamic)
      for (int i = i_d+2; i <= i_f-2; i=i+3){
        for (int j = j_d; j <= j_f; j++){
          compute_new_state (i, j);
        }
      }
          
      for(int i=i_f-reste;i<= i_f; i++){
          for (int j = j_d; j <= j_f; j++){
            compute_new_state (i, j);
          }
      } 
  
}

static void traiter_tuile_omp_task (int i_d, int j_d, int i_f, int j_f)
{
  
  PRINT_DEBUG ('c', "tuile [%d-%d][%d-%d] traitée\n", i_d, i_f, j_d, j_f);  

  #pragma omp parallel
  #pragma omp single
  {   
    int reste = DIM%3;
    int a,b,c;
    for (int j = j_d; j <= j_f-2; j=j+3){
      #pragma omp task depend(out:a)
      for (int i = i_d; i <= i_f; i++){
        compute_new_state (j, i);
      }
      
      #pragma omp task depend(in:a) depend(out:b)
      for (int i = i_d; i <= i_f; i++){
        compute_new_state (j+1, i);
      }
      
      #pragma omp task depend(in:a,b) depend(out:c)
      for (int i = i_d; i <= i_f; i++){
        compute_new_state (j+2, i);
      }
    }
    
    #pragma omp taskwait
    
    for(int i=i_f-reste;i<= i_f; i++){
      for (int j = j_d; j <= j_f; j++){
        compute_new_state (i, j);
      }
    }
  }     
}


//================================================================




static void traiter_tuile_seq_diagonal (int i_d, int j_d, int i_f, int j_f)
{
  PRINT_DEBUG ('c', "tuile [%d-%d][%d-%d] traitée\n", i_d, i_f, j_d, j_f);
  int k,j,l,e;
    for (int i = i_d; i <= i_f; i++){
      k = i;
      j = j_d;
      l = i_f-i+i_d;
      e = j_f;
      while(k >= i_d && l <= i_f && j <= j_f && e >= j_d){
        compute_new_state(j,k);
        compute_new_state(e,l);
        compute_new_state(e,k);
        compute_new_state(j,l);
        k--;
        j++;
        l++;
        e--;
      }
    }
}


unsigned sable_compute_seq_diagonal (unsigned nb_iter)
{
  changement = 0;
  for (unsigned it = 1; it <= nb_iter; it ++) {
    // On traite toute l'image en un coup (oui, c'est une grosse tuile)
    traiter_tuile_seq_diagonal (1, 1, DIM - 2, DIM - 2);
    if(changement == 0)
      return it;
  }
  return 0;
}


unsigned sable_compute_omp_par (unsigned nb_iter)
{
  changement = 0;
  for (unsigned it = 1; it <= nb_iter; it ++) {
    // On traite toute l'image en un coup (oui, c'est une grosse tuile)
    traiter_tuile_seq_diagonal (1, 1, DIM - 2, DIM - 2);
    if(changement == 0)
      return it;
  }
  return 0;
}

 unsigned sable_compute_omp_task (unsigned nb_iter)
{
  changement = 0;
  for (unsigned it = 1; it <= nb_iter; it ++) {
    // On traite toute l'image en un coup (oui, c'est une grosse tuile)
    traiter_tuile_omp_task (1, 1, DIM - 2, DIM - 2);
    if(changement == 0)
      return it;
  }
  return 0;
}

 unsigned sable_compute_omp_for (unsigned nb_iter)
{
  changement = 0;
  for (unsigned it = 1; it <= nb_iter; it ++) {
    // On traite toute l'image en un coup (oui, c'est une grosse tuile)
    traiter_tuile_omp_for (1, 1, DIM - 2, DIM - 2);
    if(changement == 0)
      return it;
  }
  return 0;
}


///////////////////////////// Version séquentielle tuilée (tiled)


static unsigned tranche = 0;

unsigned sable_compute_tiled (unsigned nb_iter)
{
  changement = 0;
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
  if(changement == 0)
      return it;
  }
  
  return 0;
}

unsigned sable_compute_tiled22 (unsigned nb_iter)
{
  changement = 0;
  tranche = DIM / GRAIN;
  for (unsigned it = 1; it <= nb_iter; it ++) {

    // On itére sur les coordonnées des tuiles
    for (int i=1; i <= GRAIN; i++)
      for (int j=1; j <= GRAIN; j++)
  {
    traiter_tuile (i * tranche/* i debut */,
       j * tranche,
       (i) * tranche + GRAIN/* i fin */,
       (j) * tranche + GRAIN/* j fin */);
  }
  if(changement == 0)
      return it;
  }
  
  return 0;
}

unsigned sable_compute_tiled_double (unsigned nb_iter)
{
  changement = 0;
  tranche = DIM / GRAIN;
  for (unsigned it = 1; it <= nb_iter; it ++) {

    // On itére sur les coordonnées des tuiles
    for (int i=0; i < GRAIN; i++)
      for (int j=0; j < GRAIN; j++)
  {
    traiter_tuile_double (i == 0 ? 1 : (i * tranche) /* i debut */,
       j == 0 ? 1 : (j * tranche) /* j debut */,
       (i + 1) * tranche - 1 - (i == GRAIN-1)/* i fin */,
       (j + 1) * tranche - 1 - (j == GRAIN-1)/* j fin */);
  }
  if(changement == 0)
      return it;
  }
  
  return 0;
}

unsigned sable_compute_tiled_deroule2 (unsigned nb_iter)
{
  changement = 0;
  tranche = DIM / GRAIN;
  for (unsigned it = 1; it <= nb_iter; it ++) {

    // On itére sur les coordonnées des tuiles
    for (int i=0; i < GRAIN; i++)
      for (int j=0; j < GRAIN; j++)
  {
    traiter_tuile_deroule2 (i == 0 ? 1 : (i * tranche) /* i debut */,
       j == 0 ? 1 : (j * tranche) /* j debut */,
       (i + 1) * tranche - 1 - (i == GRAIN-1)/* i fin */,
       (j + 1) * tranche - 1 - (j == GRAIN-1)/* j fin */);
  }
  if(changement == 0)
      return it;
  }
  
  return 0;
}

unsigned sable_compute_tiled_deroule3 (unsigned nb_iter)
{
  changement = 0;
  tranche = DIM / GRAIN;
  for (unsigned it = 1; it <= nb_iter; it ++) {

    // On itére sur les coordonnées des tuiles
    for (int i=0; i < GRAIN; i++)
      for (int j=0; j < GRAIN; j++)
  {
    traiter_tuile_deroule3 (i == 0 ? 1 : (i * tranche) /* i debut */,
       j == 0 ? 1 : (j * tranche) /* j debut */,
       (i + 1) * tranche - 1 - (i == GRAIN-1)/* i fin */,
       (j + 1) * tranche - 1 - (j == GRAIN-1)/* j fin */);
  }
  if(changement == 0)
      return it;
  }
  
  return 0;
}

unsigned sable_compute_tiled_deroule4 (unsigned nb_iter)
{
  changement = 0;
  tranche = DIM / GRAIN;
  for (unsigned it = 1; it <= nb_iter; it ++) {

    // On itére sur les coordonnées des tuiles
    for (int i=0; i < GRAIN; i++)
      for (int j=0; j < GRAIN; j++)
  {
    traiter_tuile_deroule4 (i == 0 ? 1 : (i * tranche) /* i debut */,
       j == 0 ? 1 : (j * tranche) /* j debut */,
       (i + 1) * tranche - 1 - (i == GRAIN-1)/* i fin */,
       (j + 1) * tranche - 1 - (j == GRAIN-1)/* j fin */);
  }
  if(changement == 0)
      return it;
  }
  
  return 0;
}

unsigned sable_compute_tiled_deroule5 (unsigned nb_iter)
{
  changement = 0;
  tranche = DIM / GRAIN;
  for (unsigned it = 1; it <= nb_iter; it ++) {

    // On itére sur les coordonnées des tuiles
    for (int i=0; i < GRAIN; i++)
      for (int j=0; j < GRAIN; j++)
  {
    traiter_tuile_deroule5 (i == 0 ? 1 : (i * tranche) /* i debut */,
       j == 0 ? 1 : (j * tranche) /* j debut */,
       (i + 1) * tranche - 1 - (i == GRAIN-1)/* i fin */,
       (j + 1) * tranche - 1 - (j == GRAIN-1)/* j fin */);
  }
  if(changement == 0)
      return it;
  }
  
  return 0;
}

unsigned sable_compute_tiled_diagonal (unsigned nb_iter)
{
  changement = 0;
  tranche = DIM / GRAIN;
  for (unsigned it = 1; it <= nb_iter; it ++) {

    // On itére sur les coordonnées des tuiles
    for (int i=0; i < GRAIN; i++)
      for (int j=0; j < GRAIN; j++)
  {
    traiter_tuile_seq_diagonal (i == 0 ? 1 : (i * tranche) /* i debut */,
       j == 0 ? 1 : (j * tranche) /* j debut */,
       (i + 1) * tranche - 1 - (i == GRAIN-1)/* i fin */,
       (j + 1) * tranche - 1 - (j == GRAIN-1)/* j fin */);
  }
  if(changement == 0)
      return it;
  }
  
  return 0;
}




unsigned sable_compute_tiled_task (unsigned nb_iter){
  changement = 0;
  tranche = DIM / GRAIN;
  unsigned it;
  int mat[tranche][tranche];
  for (it = 1; it <= nb_iter; it ++) {
    #pragma omp parallel
    #pragma omp single
    // On itére sur les coordonnées des tuiles
    for (int i=0; i < GRAIN; i++){
      for (int j=0; j < GRAIN; j++){
        if(i==0 && j==0){
          #pragma omp task firstprivate(i,j,changement) depend(out:table(i,j),mat[1][1])
    traiter_tuile (i == 0 ? 1 : (i * tranche) /* i debut */,
       j == 0 ? 1 : (j * tranche) /* j debut */,
       (i + 1) * tranche - 1 - (i == GRAIN-1)/* i fin */,
       (j + 1) * tranche - 1 - (j == GRAIN-1)/* j fin */);
        }else{
          if(i==0){
            #pragma omp task firstprivate(i,j,changement) depend(in:table(i,j-1),mat[i][j-1]) depend(out:table(i,j),mat[i][j])
    traiter_tuile (i == 0 ? 1 : (i * tranche) /* i debut */,
       j == 0 ? 1 : (j * tranche) /* j debut */,
       (i + 1) * tranche - 1 - (i == GRAIN-1)/* i fin */,
       (j + 1) * tranche - 1 - (j == GRAIN-1)/* j fin */);
        }else{        
          if(j==0){
            #pragma omp task firstprivate(i,j,changement) depend(in:table(i-1,j),mat[i-1][j]) depend(out:table(i,j),mat[i-1][j])
    traiter_tuile (i == 0 ? 1 : (i * tranche) /* i debut */,
       j == 0 ? 1 : (j * tranche) /* j debut */,
       (i + 1) * tranche - 1 - (i == GRAIN-1)/* i fin */,
       (j + 1) * tranche - 1 - (j == GRAIN-1)/* j fin */);
        }else{
            #pragma omp task firstprivate(i,j,changement) depend(in:table(i-1,j),table(i,j-1),mat[i][j-1],mat[i-1][j]) depend(out:table(i,j),mat[i][j])
    traiter_tuile (i == 0 ? 1 : (i * tranche) /* i debut */,
       j == 0 ? 1 : (j * tranche) /* j debut */,
       (i + 1) * tranche - 1 - (i == GRAIN-1)/* i fin */,
       (j + 1) * tranche - 1 - (j == GRAIN-1)/* j fin */);
        }
      }
    }
  }
}
  #pragma omp taskwait
  }
  if(changement == 0)
    return it;
  return 0;
}



unsigned sable_compute_tiled_omp_for (unsigned nb_iter)
{
  changement = 0;
  tranche = DIM / GRAIN;
  //int I,J;
  for (unsigned it = 1; it <= nb_iter; it ++) {
    #pragma omp parallel for collapse(2) schedule(dynamic)
    for (int i=0; i < GRAIN; i++)
      for (int j=0; j < GRAIN; j++)
  {
    traiter_tuile (i == 0 ? 1 : (i * tranche) /* i debut */,
       j == 0 ? 1 : (j * tranche) /* j debut */,
       (i + 1) * tranche - 1 - (i == GRAIN-1)/* i fin */,
       (j + 1) * tranche - 1 - (j == GRAIN-1)/* j fin */);
  }
  if(changement == 0)
      return it;
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
  //#pragma omp parallel for collapse(2)
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
  //#pragma omp parallel for collapse(2)
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
  //#pragma omp critical
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