#include<stdbool.h>
#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<unistd.h>
#include<stdbool.h>
#include<time.h>

#include "../spmc_mpsc_queue.h"


time_t start_t, end_t;
pool_t pool;

typedef struct mydata {
	int data;
	/* NOTE: node should always be the last element */
	pool_node_t node;  		// node links request_data in list (reqpool, resourcepool)
} mydata_t;	


static void *consumer(void *param) {
	mydata_t *mydata = NULL;
	pool_t *pool = (pool_t*)param;
	pool_node_t *node = NULL;
	int data;
        double diff_t;
	time(&start_t);
	while (true) {
		node = dequeue_spmc(pool);
		if (node != NULL) {
			mydata = CAST_DATA(node, mydata_t);
			data = mydata->data;
			free(mydata);
			time(&end_t);
			diff_t = difftime(end_t, start_t);
			if (diff_t >= 10) {
				printf("req/sec: %f\n", data/diff_t);
				exit(0);
			}
		}
	}
}

static void *producer(void *param) {
	int counter = 1;
	pool_t *pool = (pool_t*)param;
	while(true) {
		mydata_t *mydata = (mydata_t *)malloc(sizeof(mydata_t));
		mydata->data = counter;
		enqueue_spmc(pool, &(mydata->node));
		counter++;
	}
}

int main(int argc, char **argv) {
	pthread_t c_thread[12];
	pthread_t p_thread;
	char *b;
	int i = 0;
	init_pool(&pool);
	pthread_create(&p_thread, NULL, producer, &pool);
    	for(i=0; i<12; i++) {
		pthread_create(&c_thread[i], NULL, consumer, &pool);
	}
	pthread_join(p_thread, (void**)&b);
    	for(i=0; i<12; i++) {
		pthread_join(c_thread[i], (void**)&b);
	}
}
