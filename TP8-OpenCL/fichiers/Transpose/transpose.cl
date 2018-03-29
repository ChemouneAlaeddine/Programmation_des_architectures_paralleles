
__kernel void transpose(__global float *in,
	  	     __global float *out)
{
  int x = get_global_id(0);
  int y = get_global_id(1);

  out[x * get_global_size(0) + y] = in[y * get_global_size(0) + x];
}
