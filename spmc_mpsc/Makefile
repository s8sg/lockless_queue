build:
	gcc tests/spmc_queue_test.c -g -lpthread -o lock_spmc
	gcc tests/spmc_queue_test.c -g -D LOCKLESS_QUEUE -lpthread -o lockless_spmc
	gcc tests/mpsc_queue_test.c -g -lpthread -o lock_mpsc
	gcc tests/mpsc_queue_test.c -g -D LOCKLESS_QUEUE -lpthread -o lockless_mpsc
test: build
	./lock_spmc
	./lockless_spmc
	./lock_mpsc
	./lockless_mpsc	
clean:
	rm -f lock_spmc
	rm -f lockless_spmc
	rm -f lock_mpsc
	rm -f lockless_mpsc
all: clean test
