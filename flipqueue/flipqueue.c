#include <flipqueue.h>

t_flipqueue * create_queue(int size, time_t interval) {
	t_flipqueue *flipqueue = (t_flipqueue*)malloc(sizeof(t_flpqueue));
	if flipqueue == NULL {
		return false;
	}
	flipqueue->size = size;

}
