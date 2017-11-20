/* NOTE: The design decision of the req pool is tuned to get the 
 *       best possible performance in GSLB. Below point describes 
 *       the design decisions: 
 *       >> reqpool is a buffer queue where the producer adds at the start 
 *          and consumers consume from the end
 *       >> it is strictly one consumer and multiple producers queue
 *       >> It is unbounded queue and avoids any resource allocation on heap
 *       >> It is lockless and use atomic operation to avoid race condition
 *          for consumers
 */

#include <stdbool.h>

/*  pool_node_t must be the last member of data item that needs
 *  to be added in the queue:
 *
 *  typedef struct mydata {
 *      ....
 *      pool_node_t node;
 *  } mydata_t;
 */
typedef struct pool_node {
	struct pool_node *next;
	struct pool_node *prev;
} pool_node_t;

/* Cast a data out to its initial type from node reference 
 * mydata_t *mydata = CAST_DATA(node, mydata_t)
 */ 
#define CAST_DATA(node, datatype) (datatype *)((char*)node - (sizeof(datatype) - sizeof(pool_node_t)))

#ifdef LOCKLESS_QUEUE
struct pool {
	pool_node_t *start;
	pool_node_t *end;
};
typedef struct pool pool_t;

/* As there are multiplle producer we need atomic operation
 * while dequeueing */
#define __SET_END(pool, old_node, node) __sync_bool_compare_and_swap(&(pool->end), old_node, node)
#define __SET_START(pool, old_node, node) __sync_bool_compare_and_swap(&(pool->start), old_node, node)
#define __INIT_NODE(node) do{node->next=NULL;node->prev=NULL;}while(0)

/* Cast a data out to its initial type from node reference 
 * mydata_t *mydata = CAST_DATA(node, mydata_t)
 */ 
#define CAST_DATA(node, datatype) (datatype *)((char*)node - (sizeof(datatype) - sizeof(pool_node_t)))

// Init a pool
static inline void init_pool(pool_t *pool) {
	pool->start = NULL;
	pool->end = NULL;
	printf("Lockless\n");
}

/*** Request Poll - Single Producer Multiple Consumer (SPMC) ***/

/* Example:
 *
 *  init:
 *  -----
 *  reqpool_t pool;
 *  init_pool(&pool);
 *
 *  enqueue (1):
 *  -----------
 *  mydata_t *mydata = (mydata_t*)malloc(sizeof(mydata_t));
 *  enqueue_spmc(pool, &mydata->node); 
 *
 *  dequeue (N):
 *  -----------
 *  mydata_t *mydata;
 *  pool_node_t *node = dequeue_spmc(pool);
 *  mydata = CAST_DATA(node, mydata_t);
 */

// Add node to the pool (strictly single producer)
static inline void enqueue_spmc(pool_t *pool, pool_node_t *node) {
	__INIT_NODE(node);
	pool_node_t *start = NULL;
	/* only successful if end is NULL */
	if(!__SET_END(pool, NULL, node)) {
		start = pool->start;
		if(start != NULL && start != node)
			start->prev = node;
		else
			__SET_START(pool, NULL, node);
	}
}

// Dequeue node from pool
static inline pool_node_t * dequeue_spmc(pool_t *pool) {
	pool_node_t *node = NULL;
	node = pool->end;
	if(node != NULL) {
		if(__SET_END(pool, node, node->prev)) {
			__SET_START(pool, node, NULL);
			__INIT_NODE(node);
		}else
			node = NULL;
	}
	return node;
}

/*** Resource Pool - Multiple Producer Single Consumer (MPSC) ***/

/* Example:
 *
 *  init:
 *  -----
 *  reqpool_t pool;
 *  init_pool(&pool);
 *
 *  enqueue (N):
 *  -----------
 *  mydata_t *mydata = (mydata_t*)malloc(sizeof(mydata_t));
 *  enqueue_mpsc(pool, &mydata->node); 
 *
 *  dequeue (1):
 *  -----------
 *  mydata_t *mydata;
 *  pool_node_t *node = dequeue_mpsc(pool);
 *  mydata = CAST_DATA(node, mydata_t);
 */

// Add node to the pool
static inline void enqueue_mpsc(pool_t *pool, pool_node_t *node) {
	pool_node_t *start = NULL;
	__INIT_NODE(node);
	do {
		start = pool->start;
		//node->next = start;
		if(__SET_START(pool, start, node)) {
			if(!__SET_END(pool, NULL, node) && start != NULL) {
				start->prev = node;
			}
		}else {
			__INIT_NODE(node);
			start = NULL;
			continue;
		}	
	}while(0);
}

// Dequeue node from pool (strictly single consumer)
static inline pool_node_t * dequeue_mpsc(pool_t *pool) {
	pool_node_t *node = NULL;
	node = pool->end;
	if (node != NULL) {
		if (__SET_END(pool, node, node->prev)) { 
			__SET_START(pool, node, NULL);
			__INIT_NODE(node);
		}else 
			node = NULL;
	}
	return node;
}

/* LOCKLESS_QUEUE */

#else /* LOCK_QUEUE */

#include <pthread.h>

struct pool {
	pool_node_t *start;
	pool_node_t *end;
	pthread_mutex_t mutex;
};
typedef struct pool pool_t;

/* As there are multiplle producer we need atomic operation
 * while dequeueing */
#define __SET_END(pool, old_node, node) __sync_bool_compare_and_swap(&(pool->end), old_node, node)
#define __SET_START(pool, old_node, node) __sync_bool_compare_and_swap(&(pool->start), old_node, node)
#define __INIT_NODE(node) do{node->next=NULL;node->prev=NULL;}while(0)

// Init a pool
static inline void init_pool(pool_t *pool) {
	pool->start = NULL;
	pool->end = NULL;
	pthread_mutex_init(&pool->mutex, NULL);
	printf("Lock\n");
}

/* In Locked Queue there is no difference between SPMC and MPMC it is basically MPMC 
 * although we keep the name same to avoid confusion */

#define enqueue_spmc(pool, node) enqueue(pool, node)
#define dequeue_spmc(pool) dequeue(pool)
#define enqueue_mpsc(pool, node) enqueue(pool, node)
#define dequeue_mpsc(pool) dequeue(pool)

/* Example:
 *
 *  init:
 *  -----
 *  reqpool_t pool;
 *  init_pool(&pool);
 *
 *  enqueue (N):
 *  -----------
 *  mydata_t *mydata = (mydata_t*)malloc(sizeof(mydata_t));
 *  enqueue(pool, &mydata->node); 
 *
 *  dequeue (N):
 *  -----------
 *  mydata_t *mydata;
 *  pool_node_t *node = dequeue(pool);
 *  mydata = CAST_DATA(node, mydata_t);
 */

// Add node to the pool (strictly single producer)
static inline void enqueue(pool_t *pool, pool_node_t *node) {
	__INIT_NODE(node);
	pthread_mutex_lock(&pool->mutex);
	if(pool->start != NULL)
		pool->start->prev = node;
	node->next=pool->start; 
	pool->start=node; 
	if(pool->end == NULL)
		pool->end = node;
	pthread_mutex_unlock(&pool->mutex);
}

// Dequeue node from pool
static inline pool_node_t * dequeue(pool_t *pool) {
	pool_node_t *node = NULL;
	pthread_mutex_lock(&pool->mutex);
	node = pool->end;
	if(node != NULL) {
		pool->end = node->prev;
		if (pool->end != NULL)
			pool->end->next = NULL;
		else
			pool->start = NULL;
		__INIT_NODE(node);
	}
	pthread_mutex_unlock(&pool->mutex);
	return node;
}
#endif
/* LOCK_QUEUE */
