#include "local_mutex.h"

FILE * fptr;
extern pthread_mutex_t *m_LOCK;
extern int *s_shm, * c_shm;
struct timespec some_time;

int my_rand(int start, int range){
	struct timeval t;
	gettimeofday(&t, (struct timezone *)NULL);
	return (int) (start+(float)range*rand_r((unsigned *)&t.tv_usec)/(RAND_MAX+1.0));
}

void produce(){
	int err, * n;
	printf("%u \t P: attempting to produce \t %d\n",(unsigned)pthread_self(),getpid());
	fflush(stdout);

	if((err=pthread_mutex_trylock(m_LOCK))!=0){
		printf("%u \t P: lock busy(code %d)    \t %d\n",(unsigned)pthread_self(),err,(int)getpid());
		fflush(stdout);
		return;
	}

//	pthread_mutex_lock(m_LOCK);
	n=(int *)malloc(sizeof(int));
	(*n)=my_rand(1,MAX);
	fptr=fopen(BUFFER,"a"); 
//"a" means that open a file for appending. If this file doesn't exists, create a new file;if it exists
//the original file is preserved.
//	printf("the fptr for producer is %d\n",fptr);
	fwrite(n,sizeof(int),1,fptr);
	fclose(fptr);
	printf("%u  \t P: The # [%d] deposited   \t %d\n",(unsigned)pthread_self(),*n,getpid());
	fflush(stdout);
	free(n);
	some_time.tv_sec=0;
	some_time.tv_nsec=10000;
	nanosleep(&some_time,NULL);
	if((err=pthread_mutex_unlock(m_LOCK))!=0){
		printf("P: unlock failure %d\n",err);
		fflush(stdout);
		exit(102);
	}
}

void consume(){
	int err, *n;
	printf("%u  \t C: attempting to consume \t %d \n", (unsigned)pthread_self(),(int)getpid());
	fflush(stdout);
	if((err=pthread_mutex_trylock(m_LOCK))!=0){
		printf("%u\t C: lock busy(code %d)        \t %d\n",(unsigned)pthread_self(),err,(int)getpid());
		fflush(stdout);
		return;
	}

//	pthread_mutex_lock(m_LOCK);

	fptr=fopen(BUFFER,"r+" );
//"r+" means that open a file for  read and write. This file must exist.
//	printf("the fptr for consumer is %d\n",fptr);
	fseek(fptr,(*c_shm)*sizeof(int),SEEK_SET);
	n=(int *)malloc(sizeof(int));
	*n=0;
	fread(n,sizeof(int),1,fptr);
	if((*n)>0){
		printf("%u \t C: The # [%d] obtained    \t %d\n",(unsigned)pthread_self(),*n, getpid());
		fflush(stdout);
		fseek(fptr, (*c_shm)*sizeof(int),SEEK_SET);
		*n=-(*n);
		fwrite(n,sizeof(int),1,fptr);
		fclose(fptr);
		++*c_shm;
	}
	else{
		printf("%u \t C: No new # to consume   \t %d\n",(unsigned)pthread_self(),(int)getpid());
		fflush(stdout);
		fclose(fptr);
	}
	free(n);
	some_time.tv_sec=0;
	some_time.tv_nsec=10000;
	nanosleep(&some_time,NULL);
	if((err=pthread_mutex_unlock(m_LOCK))!=0){
		printf("C: unlock failure %d\n",err);
		exit(104);
	}
}

void do_work(){
	int i;
	if(!(*s_shm)){
		pthread_mutex_lock(m_LOCK);
		if(!(*s_shm)++){
			printf("%u   \t :clearing the buffer  \t %d\n",(unsigned)pthread_self(),(int)getpid());
			fptr=fopen(BUFFER, "w");// "w" means that open a new text file only for writing,
			//if this file already exists, delete it and create a new file
			fclose(fptr);
		}
		pthread_mutex_unlock(m_LOCK);
	}
	for(i=0; i<10;i++){
		some_time.tv_sec=0;
		some_time.tv_nsec=10000;
		nanosleep(&some_time, NULL);
		switch(my_rand(1,2)){
		case 1:
			produce();
			break;
		default:
			consume();
		}
	}
	
}

