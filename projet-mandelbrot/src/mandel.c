
#include "global.h"
#include "compute.h"
#include "graphics.h"
#include "debug.h"
#include "ocl.h"
#include "scheduler.h"

#include <stdbool.h>

#define MAX_ITERATIONS 4096
#define ZOOM_SPEED -0.01

static unsigned iteration_to_color (unsigned iter)
{
  unsigned r = 0, g = 0, b = 0;

  if (iter < MAX_ITERATIONS) {
    if (iter < 64) {
      r = iter * 2;    /* 0x0000 to 0x007E */
    } else if (iter < 128) {
      r = (((iter - 64) * 128) / 126) + 128;    /* 0x0080 to 0x00C0 */
    } else if (iter < 256) {
      r = (((iter - 128) * 62) / 127) + 193;    /* 0x00C1 to 0x00FF */
    } else if (iter < 512) {
      r = 255;
      g = (((iter - 256) * 62) / 255) + 1;    /* 0x01FF to 0x3FFF */
    } else if (iter < 1024) {
      r = 255;
      g = (((iter - 512) * 63) / 511) + 64;   /* 0x40FF to 0x7FFF */
    } else if (iter < 2048) {
      r = 255;
      g = (((iter - 1024) * 63) / 1023) + 128;   /* 0x80FF to 0xBFFF */     
    } else if (iter < 4096) {
      r = 255;
      g = (((iter - 2048) * 63) / 2047) + 192;   /* 0xC0FF to 0xFFFF */
    } else {
      r = 255;
      g = 255;
    }
  }
  return (r << 24) | (g << 16) | (b << 8) | 255 /* alpha */;
}


// Cadre initial
#if 1
static float leftX = -0.744;
static float rightX = -0.7439;
static float topY = .146;
static float bottomY = .1459;
#endif

#if 0
static float leftX = -0.2395;
static float rightX = -0.2275;
static float topY = .660;
static float bottomY = .648;
#endif

#if 0
static float leftX = -0.13749;
static float rightX = -0.13715;
static float topY = .64975;
static float bottomY = .64941;
#endif

static float xstep;
static float ystep;

static void zoom (void)
{
  float xrange = (rightX - leftX);
  float yrange = (topY - bottomY);
  
  leftX += ZOOM_SPEED * xrange;
  rightX -= ZOOM_SPEED * xrange;
  topY -= ZOOM_SPEED * yrange;
  bottomY += ZOOM_SPEED * yrange;

  xstep = (rightX - leftX) / DIM;
  ystep = (topY - bottomY) / DIM;
}


static unsigned compute_one_pixel (int i, int j)
{
  float xc = leftX + xstep * j;
  float yc = topY - ystep * i;
  float x = 0.0, y = 0.0;	/* Z = X+I*Y */

  int iter;

  // Pour chaque pixel, on calcule les termes d'une suite, et on
  // s'arrête lorsque |Z| > 2 ou lorsqu'on atteint MAX_ITERATIONS
  for (iter = 0; iter < MAX_ITERATIONS; iter++) {
    float x2 = x*x;
    float y2 = y*y;

    /* Stop iterations when |Z| > 2 */
    if (x2 + y2 > 4.0)
      break;
	
    float twoxy = (float)2.0 * x * y;
    /* Z = Z^2 + C */
    x = x2 - y2 + xc;
    y = twoxy + yc;
  }

  return iter;
}

///////////////////////////// Version séquentielle simple (seq)


void mandel_init_seq ()
{
  xstep = (rightX - leftX) / DIM;
  ystep = (topY - bottomY) / DIM;
}

// Renvoie le nombre d'itérations effectuées avant stabilisation, ou 0
unsigned mandel_compute_seq (unsigned nb_iter)
{
  for (unsigned it = 1; it <= nb_iter; it ++) {

    for (int i = 0; i < DIM; i++)
      for (int j = 0; j < DIM; j++)
	cur_img (i, j) = iteration_to_color (compute_one_pixel (i, j));

    zoom ();
  }

  return 0;
}

///////////////////////////// Version séquentielle tuilée (tiled)

#define GRAIN 8

static unsigned tranche = 0;

void mandel_init_tiled ()
{
  xstep = (rightX - leftX) / DIM;
  ystep = (topY - bottomY) / DIM;
}

static void traiter_tuile (int i_d, int j_d, int i_f, int j_f)
{
  PRINT_DEBUG ('c', "tuile [%d-%d][%d-%d] traitée\n", i_d, i_f, j_d, j_f);
  
  for (int i = i_d; i <= i_f; i++)
    for (int j = j_d; j <= j_f; j++)
	cur_img (i, j) = iteration_to_color (compute_one_pixel (i, j));
}

unsigned mandel_compute_tiled (unsigned nb_iter)
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

    zoom ();
  }

  return 0;
}

///////////////////////////// Version OpenMP avec omp for (omp)


unsigned mandel_compute_omp (unsigned nb_iter)
{
  // TODO
  return 0;
}  


///////////////////////////// Version utilisant un ordonnanceur maison (sched)

static unsigned proc_color [7] = {
  0x00F9F9FF,           // Cyan
  0xAE4AFFFF,           // Purple
  0x66CCFFFF,           // Sky Blue
  0xFF6666FF,           // Salmon
  0xFFFF00FF,           // Yellow
  0xFF7426FF,           // Orange
  0x00FF00FF,           // Green
};

unsigned P;

void mandel_init_sched ()
{
  xstep = (rightX - leftX) / DIM;
  ystep = (topY - bottomY) / DIM;

  P = scheduler_init (-1);
}

void mandel_finalize_sched ()
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

static inline unsigned cpu (int i, int j)
{
  return -1; // was: i % P
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
      cur_img (i, j) = 0 ;
}

static void first_touch_task (void *p, unsigned proc)
{
  int i, j;

  unpack (p, &i, &j);

  //PRINT_DEBUG ('s', "First-touch Task is running on tile (%d, %d) over cpu #%d\n", i, j, proc);
  zero_seq (i * tranche, j * tranche, (i + 1) * tranche - 1, (j + 1) * tranche - 1);
}

void mandel_ft_sched (void)
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
  traiter_tuile (i * tranche, j * tranche, (i + 1) * tranche - 1, (j + 1) * tranche - 1);
#if 1
  // For debugging purpose
  for (int line = i * tranche; line <= i * tranche + 5; line++)
    for (int col = j * tranche; col <= j * tranche + 5; col++)
      cur_img (line, col) = proc_color [proc % 7];
#endif
}

unsigned mandel_compute_sched (unsigned nb_iter)
{
  tranche = DIM / GRAIN;

  for (unsigned it = 1; it <= nb_iter; it ++) {

    for (int i = 0; i < GRAIN; i++)
      for (int j = 0; j < GRAIN; j++)
	create_task (compute_task, i, j);

    scheduler_task_wait ();

    zoom ();
  }
  
  return 0;
}

//////////////////////////////////////////////////////////////////////////
///////////////////////////// Version OpenCL

void mandel_init_ocl ()
{
  xstep = (rightX - leftX) / DIM;
  ystep = (topY - bottomY) / DIM;
}

unsigned mandel_compute_ocl (unsigned nb_iter)
{
  size_t global[2] = { SIZE, SIZE };  // global domain size for our calculation
  size_t local[2]  = { TILEX, TILEY };  // local domain size for our calculation
  cl_int err;
  unsigned max_iter = MAX_ITERATIONS;
  
  for (unsigned it = 1; it <= nb_iter; it ++) {
    
    // Set kernel arguments
    //
    err = 0;
    err |= clSetKernelArg (compute_kernel, 0, sizeof (cl_mem), &cur_buffer);
    err |= clSetKernelArg (compute_kernel, 1, sizeof (float), &leftX);
    err |= clSetKernelArg (compute_kernel, 2, sizeof (float), &xstep);
    err |= clSetKernelArg (compute_kernel, 3, sizeof (float), &topY);
    err |= clSetKernelArg (compute_kernel, 4, sizeof (float), &ystep);
    err |= clSetKernelArg (compute_kernel, 5, sizeof (unsigned), &max_iter);

    check (err, "Failed to set kernel arguments");

    err = clEnqueueNDRangeKernel (queue, compute_kernel, 2, NULL, global, local,
				  0, NULL, NULL);
    check (err, "Failed to execute kernel");

    zoom ();
  }

  return 0;
}
