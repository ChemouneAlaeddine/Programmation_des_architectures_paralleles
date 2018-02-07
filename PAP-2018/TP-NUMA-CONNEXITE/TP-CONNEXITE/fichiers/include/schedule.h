#ifndef SCHEDULE_IS_DEF
#define SCHEDULE_IS_DEF

typedef void (*job_func_t)(void *);

struct job {
  job_func_t fun;
  void *p;
};

extern void task_wait (void);

extern void add_job (struct job todo, int w);

extern void run_workers (int nb);

extern void stop_workers (void);


#endif
