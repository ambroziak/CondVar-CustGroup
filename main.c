/**
 * We just have an amazing restaurant newly opened in Madison!
 * However, this weird restaurant only serves group customers, not individual customers.
 * So these smart customers, who have studied computer science, decide to form groups
 * in the waiting room.
 * Here is the idea (assuming 8 seats in the waiting room):
 * - When a customer comes to the restaurant:
    - if there are fewer than 8 people in the waiting room, get in and take a seat
    - otherwise, wait outside the waiting room
 * - After taking a seat in the waiting room:
    - if there are exactly 8 people now, they form a group and leave the waiting room to have dinner
    - otherwise, wait inside the waiting room until there are enough people to form a group
 * This idea has been implmented below. However, it turns out it is very slow... 
 * Because it has too much busy waiting. Your job is to use conditional variables to
 * avoid busy waiting.
 * The places that you might need to change (two busy waiting while-loops) is marked by
 * "[!] DO SOMETHING SMART HERE".
 * You probably need to add two conditional variables. If you are doing it right, you
 * should be available to see significant speed up :)

 * More notes:
 * - run the unmodified version first to see the "correct" output. It should always be like 8 lines of "Customer [XX:YY] done waiting" followed by "=== Group [XX] all done waiting ===".
 * - By "significant speed up", I mean... really significant... It may take less than 5 second to finish if you use conditional variable correctly (assuming default 8/800 settings), while it can take minutes for the original version to finish.
 * - You may not set the number of customers too large... Otherwise repl.it platform may complain and have some weired behavior... After all, we are using their computional power for free. It is very nature to have some mechanism to stop us creating too many threads...
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

// default number of customer if there is no arg
#define NUM_CUSTOMERS_DEFAULT 800
// default number of seats
#define NUM_SEATS_DEFAULT 8
// group size is always same as number of seats
#define GROUP_SIZE_DEFAULT NUM_SEATS_DEFAULT

// real value (can be set from commandline)
int num_seats = NUM_SEATS_DEFAULT;
int group_size = GROUP_SIZE_DEFAULT;
int num_customers = NUM_CUSTOMERS_DEFAULT;
// number of available seats
int num_avail_seats = NUM_SEATS_DEFAULT;

// a counter of total number of threads that has left waiting room
// if num_total_left_seats % num_seats == 0, then the current thread
// is the last one of its group that leaves the waiting room
int num_total_left_waiting = 0;
pthread_mutex_t waiting_seats_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t full = PTHREAD_COND_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;

struct thread_meta {
	pthread_t tid;
	int cid; // customer id
};

void* customer_main(void* arg) {
	struct thread_meta my_meta = *(struct thread_meta*)arg;
	int cid = my_meta.cid;

	// first, try to find a seat in waiting room
	// but there are only "num_seats" seats
	// try to take a waiting seat
pthread_mutex_lock(&waiting_seats_mutex);
	while (num_avail_seats == 0) {
	  pthread_cond_wait(&empty, &waiting_seats_mutex);
	}

	// printf("Customer [%d] -> waiting\n", cid);

	// now we have taken the waiting seat
	// then wait for all the seats taken so we could form a group
	while (num_avail_seats > 0) { // [!] DO SOMETHING SMART HERE
    --num_avail_seats;
    if (num_avail_seats > 0)
      pthread_cond_wait(&full, &waiting_seats_mutex);
    else {
      for (int i = 0; i < num_seats-1; i++) 
        pthread_cond_signal(&full);
    }
    // now all seats has been taken, we form a six-customer group
    // we leave waiting room and enter dinning room
    ++num_total_left_waiting;
    int gid = (num_total_left_waiting - 1) / num_seats; // group id
    printf("Customer [%d:%d] done waiting\n", gid, cid);
    // if I am the last one leaving, set available numbers
    // so the next group could come and take the seats
    if (num_total_left_waiting % num_seats == 0) {
        printf("=== Group [%d] all done waiting ===\n", gid);
        num_avail_seats = num_seats;
        for (int i = 0; i < num_seats; i++)
          pthread_cond_signal(&empty);
    }
    break;
  }
	// Now I have entered the dinning room. Job done.
    // Start to do work...
  pthread_mutex_unlock(&waiting_seats_mutex);
	return NULL;
}

int main(int argc, char** argv) {
	if (argc == 3) {
		num_seats = atoi(argv[1]);
		num_customers = atoi(argv[2]);
		num_avail_seats = num_seats;
		if (num_seats == 0 || num_customers == 0) {
			fprintf(stderr, "Invalid parameter\n");
			exit(1);
		}
	} else if (argc == 1) {
		fprintf(stderr, "Using default parameter\n");
	} else {
		fprintf(stderr, "Usage: cust_group [num_seats] [num_customers]\n");
		exit(1);
	}

	int next_cid = 1;

	struct thread_meta* all_meta = malloc(num_customers * sizeof(struct thread_meta));
	memset(all_meta, 0, num_customers * sizeof(struct thread_meta));

	for (int i = 0; i < num_customers; ++i) {
		all_meta[i].cid = next_cid;
		++next_cid;
		pthread_create(&all_meta[i].tid, NULL, customer_main, &all_meta[i]);
	}

	for (int i = 0; i < num_customers; ++i)
	    pthread_join(all_meta[i].tid, NULL);

	pthread_mutex_destroy(&waiting_seats_mutex);

	return 0;
}
