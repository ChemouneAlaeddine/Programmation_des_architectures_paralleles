
#ifndef COMPUTE_IS_DEF
#define COMPUTE_IS_DEF

typedef int (*propager_max_func_t) (void);

int propager_max_v0 (void);
int propager_max_v1 (void);
int propager_max_v2 (void);
int propager_max_v3 (void);

extern propager_max_func_t propager_max[];
extern char *version_propagation[];

#endif
