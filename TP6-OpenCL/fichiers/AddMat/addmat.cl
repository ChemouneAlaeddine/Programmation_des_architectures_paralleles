
__kernel void addmat(__global float *A,
		     __global float *B,
		     __global float *C)
{
	int x = get_global_id(0);
	int y = get_global_id(1);

	C[y*1024+x] = A[x]+B[y];
}
