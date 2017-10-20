#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sched.h>
#include "flipqueue.h"

#define acquire_bucket(bucket) __sync_bool_compare_and_swap(&bucket->locked, UNLOCKED, LOCKED)
#define LOCK_FLIP(queue)  while(!__sync_bool_compare_and_swap(&queue->flip_lock, UNLOCKED, LOCKED)) \
						        pthread_yield();                                
#define UNLOCK_FLIP(queue) queue->flip_lock = UNLOCKED
#define LOCK_CONSUMER(queue) while(!__sync_bool_compare_and_swap(&queue->consumer_lock, UNLOCKED, LOCKED)) \
								pthread_yield();
#define UNLOCK_CONSUMER(queue) queue->consumer_lock = UNLOCKED
#define INIT_LOCK(lock) lock = UNLOCKED
#define BUCKET(buckets, pos, bsize, bucket) char *ptr = (char*)buckets; \
									 ptr = ptr + (pos * bsize); \
									 bucket = (t_bucket *)ptr;


static void FLIP(t_flipqueue *queue) {
	 LOCK_FLIP(queue);
	 void *temp = queue->readerbuckets; 
	 char *ptr = NULL;
	 for (ptr = (char*)queue->readerbuckets; \
		  ptr < (char*)(queue->readerbuckets + (queue->bucket_size * queue->size)); \
		             ptr = (char*)(ptr + queue->bucket_size)) {
		t_bucket *bucket = (t_bucket*)ptr;
		while(!acquire_bucket(bucket)) {}
		release_bucket(bucket);
	 }
	 queue->readerbuckets = queue->consumerbuckets; 
     queue->consumerbuckets = temp;
	 queue->readerpos = 0;
     queue->consumerpos = 0;
	 UNLOCK_FLIP(queue);
}

t_flipqueue * create_fqueue(int size, int datasize) {
	t_flipqueue * flipqueue = NULL;
	char *ptr = NULL;
	flipqueue = (t_flipqueue *)malloc(sizeof(t_flipqueue));
	flipqueue->size = size;
	flipqueue->bucket_size = sizeof(t_bucket) + datasize;
	INIT_LOCK(flipqueue->flip_lock);
	flipqueue->readerbuckets = malloc(flipqueue->bucket_size * size);
	flipqueue->readerpos = 0;
	flipqueue->consumerbuckets = malloc(flipqueue->bucket_size * size);
	flipqueue->consumerpos = 0;
	INIT_LOCK(flipqueue->consumer_lock);
	for (ptr = (char*)flipqueue->readerbuckets; \
		  ptr < (char*)(flipqueue->readerbuckets + (flipqueue->bucket_size * size)); \
		             ptr = (char*) (ptr + flipqueue->bucket_size)) {
		t_bucket *bucket = (t_bucket*)ptr;
		INIT_LOCK(bucket->locked);
		mark_empty(bucket);
	}
	for (ptr = (char*)flipqueue->consumerbuckets; \
		  ptr < (char*)(flipqueue->consumerbuckets + (flipqueue->bucket_size * size)); \
		             ptr = (char*) (ptr + flipqueue->bucket_size)) {
		t_bucket *bucket = (t_bucket*)ptr;
		INIT_LOCK(bucket->locked);
		mark_empty(bucket);
	}
	return flipqueue;
}

t_bucket * get_bucket_to_write(t_flipqueue *queue) {
	t_bucket *bucket = NULL;
	LOCK_FLIP(queue);
	int pos = queue->readerpos;	
	while (true) {
		if (pos >= queue->size) {
			UNLOCK_FLIP(queue);
			return NULL;
		}
		// As flip doesn't require all consumer buckets to be released
		BUCKET(queue->readerbuckets, pos, queue->bucket_size, bucket); 
		if (!acquire_bucket(bucket)) {
			pos++;
			continue; 
		}
		break;
	}
	queue->readerpos = pos + 1;
	UNLOCK_FLIP(queue);
	return bucket;
}

t_bucket * get_bucket_to_read(t_flipqueue *queue) {
	t_bucket *bucket = NULL;
	LOCK_CONSUMER(queue);
	int pos = queue->consumerpos;	
	while(true) {
		if (pos >= queue->size) {
			FLIP(queue);
			pos = 0;
			continue;
		}
		BUCKET(queue->consumerbuckets, pos, queue->bucket_size, bucket);
		if (bucket->empty) {
			bucket = NULL;
			break;
		}
		acquire_bucket(bucket);
		break;
	}
	queue->consumerpos = pos + 1;
	UNLOCK_CONSUMER(queue);
	return bucket;
}
