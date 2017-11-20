#include<stdbool.h>
#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<unistd.h>
#include<stdbool.h>
#include<time.h>

#include "../spmc_mpsc_queue.h"


time_t start_t, end_t;

typedef struct mydata {
	int data;
	/* NOTE: node should always be the last element */
	pool_node_t node;  		// node links request_data in list (reqpool, resourcepool)
} mydata_t;	


static void *consumer(void *param) {
	mydata_t *mydata = NULL;
	pool_t *pool = (pool_t*)param;
	pool_node_t *node = NULL;
        double diff_t;
	int count = 0;
	time(&start_t);
	while (true) {
		node = dequeue_mpsc(pool);
		if (node != NULL) {
			mydata = CAST_DATA(node, mydata_t);
			count++;
			time(&end_t);
			diff_t = difftime(end_t, start_t);
			if (diff_t >= 10) {
				printf("req/sec: %f\n", count/diff_t);
				exit(0);
			}
			free(mydata);
		}
	}
}

static void *producer(void *param) {
	int counter = 1;
	pool_t *pool = (pool_t*)param;
	while(true) {
		mydata_t *mydata = (mydata_t *)malloc(sizeof(mydata_t));
		mydata->data = counter;
		enqueue_mpsc(pool, &(mydata->node));
		counter++;
	}
}

int main(int argc, char **argv) {
	pool_t pool;
	pthread_t p_thread[12];
	pthread_t c_thread;
	char *b;
	int i = 0;
	init_pool(&pool);
	pthread_create(&c_thread, NULL, consumer, &pool);
    	for(i=0; i<12; i++) {
		pthread_create(&p_thread[i], NULL, producer, &pool);
	}
	pthread_join(c_thread, (void**)&b);
    	for(i=0; i<12; i++) {
		pthread_join(p_thread[i], (void**)&b);
	}
}
