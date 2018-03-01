
#include "global.h"
#include "compute.h"
#include "graphics.h"
#include "constants.h"
#include "debug.h"
#include "ocl.h"
#include "scheduler.h"
#include "error.h"

#include <stdbool.h>



///////////////////////////// Version séquentielle simple (seq)

static int monter_max_seq (int i_d, int j_d, int i_f, int j_f)
{
  int changement = 0;

  for (int i = i_d; i >= i_f; i--)
    for (int j = j_d; j >= j_f; j--)
      if (cur_img (i,j)) {
	Uint32 m = MAX (cur_img (i+1,j), cur_img (i,j+1));
	if (m > cur_img (i,j)) {
	  changement = 1;
	  cur_img (i,j) = m;
	}
      }

  return changement;
}

static int descendre_max_seq (int i_d, int j_d, int i_f, int j_f)
{
  int changement = 0;

  for (int i = i_d; i <= i_f; i++)
    for (int j = j_d; j <= j_f; j++)
      if (cur_img (i,j)) {
	Uint32 m = MAX (cur_img (i-1,j), cur_img (i,j-1));
	if (m > cur_img (i,j)) {   
	  changement = 1;
	  cur_img (i,j) = m;
	}
      }

  return changement;
}


// Renvoie le nombre d'itérations effectuées avant stabilisation, ou 0
unsigned max_compute_seq (unsigned nb_iter)
{
  for (unsigned it = 1; it <= nb_iter; it ++) {

    if ((descendre_max_seq (1, 1, DIM-1, DIM-1) | monter_max_seq (DIM-2, DIM-2, 0, 0)) == 0)
      return it;
  }

  return 0;
}


///////////////////////////// Version séquentielle tuilée (tiled)


volatile int cont = 0;

unsigned tranche = 0;

static void lancer_descente (int i, int j)
{
  int i_d = (i == 0) ? 1 : i * tranche;
  int j_d = (j == 0) ? 1 : j * tranche;
  int i_f = (i == GRAIN-1) ? DIM-1 : (i+1) * tranche - 1;
  int j_f = (j == GRAIN-1) ? DIM-1 : (j+1) * tranche - 1;

  PRINT_DEBUG ('c', "Descente: bloc(%d,%d) couvre (%d,%d)-(%d,%d)\n", i, j, i_d, j_d, i_f, j_f);
  
  if (descendre_max_seq (i_d, j_d, i_f, j_f))
    cont = 1;
}

static void lancer_monte (int i, int j)
{

  int i_d = (i == GRAIN-1) ? DIM-2 : (i+1) * tranche - 1;
  int j_d = (j == GRAIN-1) ? DIM-2 : (j+1) * tranche - 1;
  int i_f = i * tranche;
  int j_f = j * tranche;

  PRINT_DEBUG ('c', "Montée: bloc(%d,%d) couvre (%d,%d)-(%d,%d)\n", i, j, i_d, j_d, i_f, j_f);
  
  if (monter_max_seq (i_d, j_d, i_f, j_f))
    cont = 1;
}

int max_compute_tiled (unsigned nb_iter)
{    
  tranche = DIM / GRAIN;

  for (unsigned it = 1; it <= nb_iter; it ++) {

    cont = 0;

    for (int i=0; i < GRAIN; i++)
      for (int j=0; j < GRAIN; j++)
	lancer_descente (i, j);

    for (int i=0; i < GRAIN; i++)
      for (int j=0; j < GRAIN; j++)
	lancer_monte (GRAIN-i-1, GRAIN-j-1);

    if (!cont)
      return it;
  }
  
  return 0;
}


///////////////////////////// Version parallèle tuilée avec ordonnanceur maison (sched)



unsigned P;

volatile int * celluled;
volatile int * cellulem;

#define Cellulem(i,j) (cellulem[(i)*GRAIN+(j)])
#define Celluled(i,j) (celluled[(i)*GRAIN+(j)])


void max_init_sched ()
{
  P = scheduler_init (-1);
  celluled = calloc((GRAIN)*(GRAIN), sizeof (int));
  cellulem = calloc((GRAIN)*(GRAIN), sizeof (int));
}

void max_finalize_sched ()
{
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

static inline unsigned cpu (int i, int j, int sens)
{
  return i % P;
}

static inline void create_task (task_func_t t, int i, int j, int sens)
{
  scheduler_create_task (t, pack (i, j), cpu (i, j, sens));
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
  int tranche = DIM / GRAIN;
  unpack (p, &i, &j);

  //PRINT_DEBUG ('s', "First-touch Task is running on tile (%d, %d) over cpu #%d\n", i, j, proc);
  zero_seq (i * tranche, j * tranche, (i + 1) * tranche - 1, (j + 1) * tranche - 1);
}


void max_ft_sched (void)
{

  for (int i = 0; i < GRAIN; i++)
    for (int j = 0; j < GRAIN; j++)
      create_task (first_touch_task, i, j, 0 /* sens */);

  scheduler_task_wait ();
}

//////// Compute

void lancer_descente_sched (int i, int j);
void lancer_monte_sched (int i, int j);

volatile int changement ;

static void compute_descente (void *p, unsigned proc)
{
  int i, j;
  int tranche = DIM / GRAIN;
  
  unpack (p, &i, &j);
  
  //PRINT_DEBUG ('s', "Compute Task is running on tile (%d, %d) over cpu #%d\n", i, j, proc);
  changement |= descendre_max_seq (i * tranche, j * tranche, (i + 1) * tranche - 1, (j + 1) * tranche - 1);

  if (j < GRAIN -1)
    lancer_descente_sched (i, j+1);

  if (i < GRAIN - 1)
    lancer_descente_sched (i+1, j);
}


static void compute_monte (void *p, unsigned proc)
{
  int i, j;
  int tranche = DIM / GRAIN;
  
  unpack (p, &i, &j);
  
  //PRINT_DEBUG ('s', "Compute Task is running on tile (%d, %d) over cpu #%d\n", i, j, proc);
  changement |= monter_max_seq ((i+1) * tranche - 1, (j+1) * tranche - 1, (i ) * tranche , (j ) * tranche);

  if (j > 0)
    lancer_monte_sched (i, j-1);
  
  if (i > 0)
    lancer_monte_sched (i-1, j);
}


unsigned max_compute_sched (unsigned nb_iter)
{
  for (unsigned it = 1; it <= nb_iter; it ++) {
    
    changement = 0;
    
    create_task (compute_descente, 0, 0, 1);
    scheduler_task_wait ();
    create_task (compute_monte, GRAIN-1, GRAIN-1, 2);    
    scheduler_task_wait ();

    if (changement == 0)
          return it;
  }
  
  return 0;
}





void 
lancer_descente_sched (int i, int j)
{
  // celluled gere les dependances entre blocs pour la descente
  // celluled[x][y] == 0 <==> les blocs [x-1][y] et [x][y-1] ne sont pas termines
  // celluled[x][y] == 1 <==> un des blocs [x-1][y] et [x][y-1] a termine
  // celluled[x][y] == 2 <==> les blocs [x-1][y] et [x][y-1] sont termines

  int val = __sync_add_and_fetch(&Celluled(i,j),1);

  if ( val != 2 && i != 0 && j != 0)
    {
      return;
    }
  
  Celluled(i,j)= 0;
  create_task (compute_descente, i, j, 1);
}


void
lancer_monte_sched (int i, int j)
{

  int val = __sync_add_and_fetch(&Cellulem(i,j),1);
  
  if (val !=2 && i != GRAIN-1 && j != GRAIN-1)
    return ;
  
  Cellulem(i,j) = 0;
  create_task (compute_monte, i, j, 2);
}




///////////////////////////// Configuration initiale (NE PAS MODIFIER)

void spiral (unsigned twists);
void recolor (void);

void max_draw (char *param)
{
  unsigned n;

  n = atoi (param);
  if (n > 0)
    spiral (n);

  recolor ();
}

void recolor (void)
{
  unsigned nbits = 0;
  unsigned rb, bb, gb;
  unsigned r_lsb, g_lsb, b_lsb;
  unsigned r_shift, g_shift, b_shift;
  Uint8 r_mask, g_mask, b_mask;
  Uint8 red = 0, blue = 0, green = 0;
  
  // Calcul du nombre de bits nécessaires pour mémoriser une valeur
  // différente pour chaque pixel de l'image
  for (int i = DIM-1; i; i >>= 1)
    nbits++; // log2(DIM-1)
  nbits = nbits * 2;

  if (nbits > 24)
    exit_with_error ("DIM of %d is too large (suggested max: 4096)\n", DIM);
  
  gb = nbits / 3;
  bb = gb;
  rb = nbits - 2 * bb;

  r_shift = 8 - rb; r_lsb = (1 << r_shift) - 1; 
  g_shift = 8 - gb; g_lsb = (1 << g_shift) - 1;
  b_shift = 8 - bb; b_lsb = (1 << b_shift) - 1;

  r_mask = (1 << rb) - 1;
  g_mask = (1 << gb) - 1;
  b_mask = (1 << bb) - 1;

  PRINT_DEBUG ('g', "nbits : %d (r: %d, g: %d, b: %d)\n", nbits, rb, gb, bb);
  PRINT_DEBUG ('g', "r_shift: %d, r_lst: %d, r_mask: %d\n", r_shift, r_lsb, r_mask);
  PRINT_DEBUG ('g', "g_shift: %d, g_lst: %d, g_mask: %d\n", g_shift, g_lsb, g_mask);
  PRINT_DEBUG ('g', "b_shift: %d, b_lst: %d, b_mask: %d\n", b_shift, b_lsb, b_mask);
  
  for (unsigned y = 0; y < DIM; y++) {
    for (unsigned x = 0; x < DIM; x++) {
      unsigned alpha = cur_img (y, x) & 255;
      
      if (alpha == 0 || x == 0 || x == DIM-1 || y == 0 || y == DIM-1)
	cur_img (y, x) = 0;
      else {
	unsigned c = 255; // alpha
	c |= ((blue  << b_shift) | b_lsb) << 8;
	c |= ((green << g_shift) | g_lsb) << 16;
	c |= ((red   << r_shift) | r_lsb) << 24;
	cur_img (y, x) = c;
      }
	
      red = (red + 1) & r_mask;
      if (red == 0) {
	green = (green + 1) & g_mask;
	if (green == 0)
	  blue = (blue + 1) & b_mask;
      }
    }
  }
}

static unsigned couleur = 0xFFFF00FF; // Yellow

static void one_spiral (int x, int y, int pas, int nbtours)
{
  int i = x, j = y, tour;
  
  for (tour = 1; tour <= nbtours; tour++) {
    for (; i < x + tour*pas;i++)
      cur_img (i,j) = couleur;
    for (; j < y + tour*pas+1;j++)
      cur_img (i,j) = couleur;
    for (; i > x - tour*pas-1 ;i--)
      cur_img (i,j) = couleur;
    for (; j > y - tour*pas-1;j--)
      cur_img (i,j) = couleur;
  }
}

static void many_spirals (int xdebut, int xfin, int ydebut, int yfin, int pas, int nbtours)
{
  int i,j;
  int taille = nbtours * pas + 2;
  
  for (i = xdebut + taille; i < xfin - taille; i += 2*taille)
    for (j = ydebut + taille; j < yfin - taille; j += 2*taille)
      one_spiral (i,j, pas, nbtours); 
}

void spiral (unsigned twists)
{
  many_spirals (1, DIM-2, 1, DIM-2, 2, twists);
}

