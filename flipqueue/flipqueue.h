typedef struct flipqueue {
	int size;
	time_t next_interval;
	pthread_mutex_lock flip_lock;
	void **part_a;
	int part_a_count;
	int part_a_start;
	int part_a_end;
	pthread_mutex_lock part_a_lock;
	void **part_b;
	int part_b_count;
	int part_b_start;
	int part_b_end;
	pthread_mutex_lock part_b_lock;
}t_flipqueue;

t_flipqueue * create_queue(int size, time_t interval);

bool enqueue(t_flipqueue *queue, void *data);

void* dequeue(t_flipqueue *queue);

int getcount(t_flipqueue *queue);
