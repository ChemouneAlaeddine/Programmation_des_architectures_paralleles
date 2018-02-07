
//#ifdef cl_khr_fp64
//    #pragma OPENCL EXTENSION cl_khr_fp64 : enable
//#elif defined(cl_amd_fp64)
//    #pragma OPENCL EXTENSION cl_amd_fp64 : enable
//#else
//    #warning "Double precision floating point not supported by OpenCL implementation."
//#endif


// NE PAS MODIFIER
static unsigned color_mean (unsigned c1, unsigned c2)
{
  uchar4 c;

  c.x = ((unsigned)(((uchar4 *) &c1)->x) + (unsigned)(((uchar4 *) &c2)->x)) / 2;
  c.y = ((unsigned)(((uchar4 *) &c1)->y) + (unsigned)(((uchar4 *) &c2)->y)) / 2;
  c.z = ((unsigned)(((uchar4 *) &c1)->z) + (unsigned)(((uchar4 *) &c2)->z)) / 2;
  c.w = ((unsigned)(((uchar4 *) &c1)->w) + (unsigned)(((uchar4 *) &c2)->w)) / 2;

  return (unsigned) c;
}

// NE PAS MODIFIER
static int4 color_to_int4 (unsigned c)
{
  uchar4 ci = *(uchar4 *) &c;
  return convert_int4 (ci);
}

// NE PAS MODIFIER
static unsigned int4_to_color (int4 i)
{
  return (unsigned) convert_uchar4 (i);
}


////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// scrollup
////////////////////////////////////////////////////////////////////////////////

__kernel void scrollup (__global unsigned *in, __global unsigned *out)
{
  int y = get_global_id (1);
  int x = get_global_id (0);
  unsigned couleur;

  couleur = in [y * DIM + x];

  y = (y ? y - 1 : get_global_size (1) - 1);

  out [y * DIM + x] = couleur;
}



////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// mandelbrot
////////////////////////////////////////////////////////////////////////////////

static unsigned mandel_iter2color (unsigned iter)
{
  unsigned r = 0, g = 0, b = 0;

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

  //return 0xFFFF00FF;
    return (r << 24) | (g << 16) | (b << 8) | 255 /* alpha */;
}


__kernel void mandel (__global unsigned *img,
		      float leftX, float xstep,
		      float topY, float ystep,
		      unsigned MAX_ITERATIONS)
{
  int i = get_global_id (1);
  int j = get_global_id (0);

  float xc = leftX + xstep * j;
  float yc = topY - ystep * i;
  float x = 0.0, y = 0.0;	/* Z = X+I*Y */

  unsigned iter;

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

  img [i * DIM + j] = (iter < MAX_ITERATIONS)
    ? mandel_iter2color (iter)
    : 0x000000FF; // black
}



// NE PAS MODIFIER
static float4 color_scatter (unsigned c)
{
  uchar4 ci;

  ci.s0123 = (*((uchar4 *) &c)).s3210;
  return convert_float4 (ci) / (float4) 255;
}

// NE PAS MODIFIER: ce noyau est appelé lorsqu'une mise à jour de la
// texture de l'image affichée est requise
__kernel void update_texture (__global unsigned *cur, __write_only image2d_t tex)
{
  int y = get_global_id (1);
  int x = get_global_id (0);
  int2 pos = (int2)(x, y);
  unsigned c;

  c = cur [y * DIM + x];

  write_imagef (tex, pos, color_scatter (c));
}
