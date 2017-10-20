#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<unistd.h>
#include "flipqueue.h"

t_flipqueue *queue;

static void *consumer(void *param) {
	t_bucket *bucket = NULL;
	while (true) {
		bucket = get_bucket_to_read(queue);
		if (bucket == NULL) {
			sleep(1);
			continue;
		}
		printf("Data: %d\n", *((int *)bucket->data));
		mark_empty(bucket);
		release_bucket(bucket);
		sleep(1);
	}
}


static void *producer(void *param) {
	int counter = 0;
	t_bucket *bucket = NULL;
	while(true) {
		bucket = get_bucket_to_write(queue);
		if (bucket == NULL) {
			continue;
		}
		*((int*)(bucket->data)) = counter;
		mark_full(bucket);
		release_bucket(bucket);
		counter++;
	}
}

int main(int argc, char **argv) {
	pthread_t p_thread;
	pthread_t c0_thread;
	pthread_t c1_thread;
	pthread_t c2_thread;
	pthread_t c3_thread;
	pthread_t c4_thread;
	char *b;
	queue = create_fqueue(6, sizeof(int)); 
	pthread_create(&p_thread, NULL, producer, NULL);
	pthread_create(&c0_thread, NULL, consumer, NULL);
	pthread_create(&c1_thread, NULL, consumer, NULL);
	pthread_create(&c2_thread, NULL, consumer, NULL);
	pthread_create(&c3_thread, NULL, consumer, NULL);
	pthread_create(&c4_thread, NULL, consumer, NULL);
	pthread_join(p_thread, (void**)&b);
	pthread_join(c0_thread, (void**)&b);
	pthread_join(c1_thread, (void**)&b);
	pthread_join(c2_thread, (void**)&b);
	pthread_join(c3_thread, (void**)&b);
	pthread_join(c4_thread, (void**)&b);
}
