#include <stdbool.h>
#define UNLOCKED false
#define LOCKED true

typedef struct bucket {
	//pthread_mutex_lock lock;
	bool locked;
	bool empty;
	char data[];
}t_bucket;

/* a single reader multi consumer queue */
typedef struct flipqueue {
	int size;                         //  no of buckets (totalBuckets = size * 2)
	int bucket_size;                  //  the size of each bucket -> sizeof(t_bucket) + maxdatasize 
	bool flip_lock;                   //  the lock that allow to flip
	void * readerbuckets;             //  [D] [D] [L] [L] [ ] [ ]
	int readerpos;                    //                  ^
	void * consumerbuckets;           //  [ ] [ ] [0] [0] [ ] [D]
	int consumerpos;                  //                      ^
	bool consumer_lock; //  the lock that allows multiple consumer to read 
}t_flipqueue;


/* Create a flipqueue based baed on length and data size */
t_flipqueue * create_fqueue(int size, int datasize);
/* Get a bucket to write data */
t_bucket * get_bucket_to_write(t_flipqueue *queue);
/* Get a bucket to read data from */ 
t_bucket * get_bucket_to_read(t_flipqueue *queue);
/* release a bucket when read/write is done */
#define release_bucket(bucket) bucket->locked = UNLOCKED
#define mark_empty(bucket) bucket->empty = true;
#define mark_full(bucket) bucket->empty = false;
