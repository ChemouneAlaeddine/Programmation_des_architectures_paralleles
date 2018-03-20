/* ne *pas* mettre de -O, sinon gcc arrive à éliminer les latences (et puis il optimise mes opérations débiles */
#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <numa.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <signal.h>
#include <sched.h>
#include <math.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>
#include <malloc.h>
#include <pthread.h>
#include <inttypes.h>
#include <sys/file.h>
#include <fcntl.h>
#include <time.h>

#define TIME_DIFF(t1, t2) \
	((t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec))


#define MAXMEMSHIFT (6+20)
#define STARTMEMSHIFT (10)
#define MEMSHIFT 1
#define MEMSIZE (1<<MAXMEMSHIFT)
#define STARTSIZE (1<<STARTMEMSHIFT)

#define NLOOPS 4
#define LOOPS 2000000



unsigned long readtime;


void generateTab(volatile char* t, int N) {
	int i;
	for(i=0;i<N;i++)
	  t[i]=i+1;
}

int sumTab(volatile char* a, int N, float gold) {
        int i,j;
	int masque=N-1; // 0..01..11
        j=0;


	for(i=0;i<LOOPS;i++){
		j+=a[j]+gold;
		j&=masque;
	}
	return j;
}


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


int main(int argc, char*argv[]) {
	int mems,m,cpu;
	long mem;
	volatile char *c;
	char sum=0;
	struct timeval tv1,tv2;
	unsigned int gold;
	int lastnode;
	int node;
#ifdef __linux__
	cpu_set_t cpusbind;
#endif
#ifdef __osf__
	radset_t radset;
	memalloc_attr_t mattr;
#endif


	if (argc<3) {
		printf("usage: %s cpu node\n",argv[0]);
		exit(EXIT_FAILURE);
	}

	cpu = atoi(argv[1]);
	node = atoi(argv[2]);

	/*************************************/
	/* Afficher le nombre de noeuds NUMA */
	/*************************************/
#ifdef __linux__
	/* Version linux */
	if (numa_available()<0) {
		fprintf(stderr,"numa not available\n");
		exit(EXIT_FAILURE);
	}
	numa_set_bind_policy(1);
	numa_set_strict(1);
	lastnode = numa_max_node();
#endif
#ifdef __osf__
	/* Version OSF */
	lastnode = rad_get_num()-1;
#endif
	printf("%d node%s\n", lastnode + 1, lastnode?"s":"");



	verrou();

	/***********************************/
	/* Migrer vers le processeur donné */
	/***********************************/
	printf("moving on cpu %d\n",cpu);
#ifdef __linux__
	/* Version linux */
	CPU_ZERO(&cpusbind);
	CPU_SET(cpu,&cpusbind);
	if (sched_setaffinity(0, sizeof(cpusbind), &cpusbind)) {
		perror("sched_setaffinity");
		exit(EXIT_FAILURE);
	}
#endif
#ifdef __osf__
	/* Version OSF */
	radsetcreate(&radset);
	rademptyset(radset);
	radaddset(radset,cpu);
	if (pthread_rad_attach(pthread_self(), radset, RAD_INSIST)) {
		perror("pthread_rad_attach");
		exit(EXIT_FAILURE);
	}
#endif


	/***************************/
	/* Allouer la zone mémoire */
	/***************************/
	printf("allocating on node %d\n",node);
#ifdef __linux__
	/* Version linux */
	c = numa_alloc_onnode(MEMSIZE, node);
#endif
#ifdef __osf__
	/* Version OSF */
	memset(&mattr, 0, sizeof(mattr));
	mattr.mattr_policy = MPOL_DIRECTED;
	if (!VALIDATE_POLICY(mattr.mattr_policy))
		fprintf(stderr,"pas valide\n");
	mattr.mattr_rad = node;
	radsetcreate(&mattr.mattr_radset);
	rademptyset(mattr.mattr_radset);
	radaddset(mattr.mattr_radset,node);
	if ((c = nmmap(NULL, MEMSIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0, &mattr)) == (void*)-1) {
		perror("nmmap");
		exit(EXIT_FAILURE);
	}
#endif
	/* Toucher vraiment la zone mémoire allouée */
	generateTab(c,MEMSIZE);
	//	memset((void*)c,0,MEMSIZE);
	//	memcpy((void*)c,(void*)c,MEMSIZE);

	/*******************************/
	/* boucle sur la taille buffer */
	/*******************************/

	for (mems=STARTMEMSHIFT,mem=1<<mems;mems<=MAXMEMSHIFT;mems+=MEMSHIFT,mem<<=MEMSHIFT) {
		gold = ((float)mem*((sqrt(5)-1.)/2.));
		for (m=0;m<NLOOPS;m++) {
			gettimeofday(&tv1,NULL);
			sum+=sumTab(c,mem,gold);
			gettimeofday(&tv2,NULL);
			readtime = (TIME_DIFF(tv1,tv2)*1000UL/LOOPS);
		}
		printf("%9lu %4lu\n", mem, readtime);
	}
	return sum;
}
