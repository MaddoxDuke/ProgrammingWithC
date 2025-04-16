#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>

#define THREAD_COUNT 5

int upper_limit;
int count = 3;
int shared_array[THREAD_COUNT] = {0};
int done = 0;
int sum = 2;

//Psuedocode:
//	
//
//
//
//
//


// declare threads to manipulate
pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t array_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t array_cond = PTHREAD_COND_INITIALIZER;


//Function to check if num is prime.

int is_prime(int n) {
	if (n < 2) return 0;
	if (n %2 == 0 && n != 2) return 0;
	for (int i = 3; i <= sqrt(n); i += 2)
		if (n % i == 0) return 0;
	return 1;
}

void* producer_thread(void* arg) {
	int id = *(int*)arg;
	while (1) {
		int current;
		pthread_mutex_lock(&count_mutex);

		if (count > upper_limit) {
			pthread_mutex_unlock(&count_mutex);
		}

		current = count;
		count += 2;
		pthread_mutex_unlock(&count_mutex);

		if (is_prime(current)) {
			while (1) {
				pthread_mutex_lock(&array_mutex);
				if (shared_array[id] == 0) {
					shared_array[id] = current;
					pthread_cond_signal(&array_cond);
					pthread_mutex_unlock(&array_mutex);

					break;
				}
				pthread_mutex_unlock(&array_mutex);
				usleep(100); // wait before we retry
			}
		}
	}
	return NULL;
}

//Consumer Thread
void* consumer_thread(void* arg) {
		while (1) {
			pthread_mutex_lock(&array_mutex);
			int found = 0;
			for (int i = 0; i < THREAD_COUNT; i++) {
				if (shared_array[i] != 0) {
					sum += shared_array[i];
					shared_array[i] = 0;
					found = 1;
				}
			}
			if (!found) {
				if (done) {
					pthread_mutex_unlock(&array_mutex);
					break;
				}
				pthread_cond_wait(&array_cond, &array_mutex);
			} else {
				pthread_cond_broadcast(&array_cond);
			}
			pthread_mutex_unlock(&array_mutex);
		}
		return NULL;
}

int main(int argc, char* argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <max>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	upper_limit = atoi(argv[1]);
	if (upper_limit < 2) {
		fprintf(stderr, "Please enter a value >= 2 \n");
		exit(EXIT_FAILURE);
	}
	
	pthread_t producers[THREAD_COUNT];
	pthread_t consumer;
	int ids[THREAD_COUNT];

		//User consumer
	if (pthread_create(&consumer, NULL, consumer_thread, NULL)) {
		perror("Failed to create consumer");
		exit(EXIT_FAILURE);
	}

	//producer
	for (int i = 0; i < THREAD_COUNT; i++) {
		ids[i] = i;
		if (pthread_create(&producers[i], NULL, producer_thread, &ids[i])) {
			perror("Failed to create producer");
			exit(EXIT_FAILURE);
		}
	}
	for (int i = 0; i < THREAD_COUNT; i++) {
		pthread_join(producers[i], NULL);
	}

	//finish
	pthread_mutex_lock(&array_mutex);
	done = 1;
	pthread_cond_signal(&array_cond);
	pthread_mutex_unlock(&array_mutex);

	pthread_join(consumer, NULL);

	printf("Sum of primes up to %d: %d\n", upper_limit, sum);
	return 0;
	}
