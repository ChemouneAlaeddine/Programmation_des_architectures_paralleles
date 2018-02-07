
#ifndef COMPUTE_IS_DEF
#define COMPUTE_IS_DEF

typedef void (*void_func_t) (void);
typedef unsigned (*int_func_t) (unsigned);

extern void_func_t the_first_touch;
extern void_func_t the_init;
extern void_func_t the_finalize;
extern int_func_t the_compute;

extern unsigned opencl_used;
extern char *version;

#endif
