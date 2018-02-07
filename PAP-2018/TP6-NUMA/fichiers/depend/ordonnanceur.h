#ifndef ORDONNANCEUR_H
#define ORDONNANCEUR_H



struct job { void *(*fun)(void *p); void *p;};

extern void task_wait();

extern void add_job(struct job todo, int w);

extern void stop_workers() ;

extern  void go(void *(*fun)(void *p),void *p, int nb);
  
#endif
