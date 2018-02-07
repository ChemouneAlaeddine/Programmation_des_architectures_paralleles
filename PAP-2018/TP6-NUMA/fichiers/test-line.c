#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include <semaphore.h>
#include <sched.h>
#include <pthread.h>
#include <assert.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define TIME_DIFF(t1, t2) \
		((t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec))

#define MAXTHREADS 256
int NTHREADS = 0 ;
int res[MAXTHREADS];

struct thread_arg {
	int num;
	int cpu;
	volatile void *addr;
	sem_t sem;
};

void *thread_fun(void *_arg) {
	struct thread_arg *arg = _arg;
	int cpu = arg->cpu;
	int num = arg->num;
	volatile int *a = arg->addr;
	cpu_set_t mask;
	int i=0;
	struct timeval tvlast, tvnew;

	/* on a fini de copier les arguments */
	sem_post(&arg->sem);

	/* se placer sur le processeur demandé */
	CPU_ZERO(&mask);
	CPU_SET(cpu, &mask);
	if (sched_setaffinity(0, sizeof(mask), &mask)) {
		perror("sched_setaffinity");
		exit(1);
	}

	gettimeofday(&tvlast, NULL);
	while(1) {
		*a = 1000000;
		while (*a>0) {
			(*a)--;
		}
		i++;
		gettimeofday(&tvnew, NULL);
		if (TIME_DIFF(tvlast,tvnew)>1000000) {
			res[num] = i;
			if (!num) {
				int j;
				fprintf(stderr,"\r");
				for (j = 0; j<NTHREADS; j++)
					fprintf(stderr,"%d\t",res[j]);
			}
			i=0;
			memcpy(&tvlast,&tvnew,sizeof(tvlast));
		}
	}
	return NULL;
}

void lance_thread(int cpu, volatile void *addr) {
	struct thread_arg arg;
	static int num = 0;
	pthread_t t;
	assert(num<MAXTHREADS);
        NTHREADS++;
	sem_init(&arg.sem, 0, 0);
	arg.num = num;
	arg.cpu = cpu;
	arg.addr = addr;
	pthread_create(&t, NULL, thread_fun, &arg);
	/* attendre que le thread ait fini de copier les arguments */
	sem_wait(&arg.sem);
	num++;
}

volatile char tab[1024] __attribute__((aligned(128)));

void verrou(void) {
  mode_t oldmode;
  /* Utilisation d'un verrou inter-processus pour éviter que vous vous
   * marchiez les uns sur les autres */
  fprintf(stderr,"waiting for lock\n");
  oldmode = umask(0);
  int fd = open("/var/tmp/verrou-PAP", O_WRONLY|O_CREAT, 0777);
  if (fd < 0) {
    perror("open");
    exit(1);
  }
  umask(oldmode);
  if (lockf(fd, F_LOCK, 0)) {
    perror("lockf");
    exit(1);
  }
  fprintf(stderr,"got lock\n");
}

int main(int argc, char *argv[]) {

  verrou();

  if (argc < 3)
    {
      fprintf(stderr, "%s distance coeur#1 coeur#2 ... coeur#n\n", argv[0]);
    }
  int i;
  int distance = atoi(argv[1]); 
  

  for(i = 2; i < argc; i++)
    lance_thread( atoi(argv[i]), &tab[(i-2)*distance]);

  /* terminer le thread principal et laisser les autres tourner */
  pthread_exit(NULL);
}
