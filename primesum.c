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

// declare threads to manipulate
pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t array_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t array_cond = PTHREAD_COND_INITIALIZER;


//Function to check if num is prime. must be greater than 3.
int is_prime(int n) {
	if (n < 2) return 0;
	if (n == 2) return 1;
	if (n% 2 == 0) return 0;

	//sqrt does not work? Used i*i <= n instead to check 
	for (int i = 3; i * i <= n; i += 2)
		if (n % i == 0) return 0;
	return 1;
}
// Producer Thread pseudocode;
// Each producer grabs a new num to check from the count, note that this count is shared.
// If the num is prime, it places it in an index in a "shared array"
// If that spot is full, it will wait until the consumer clears it.
void* producer_thread(void* arg) {
	int id = *(int*)arg; // This holds the Thread IDs for 0-4
	int current;

	while (1) {	
		// Grab a new num to test
		pthread_mutex_lock(&count_mutex);

		if (count > upper_limit) {
			pthread_mutex_unlock(&count_mutex);
			break;
		}
		
		current = count;
		count += 2; // note that we only check odd numbers since all even nums are not prime.
		pthread_mutex_unlock(&count_mutex);

		// if the num is prime, write it to the shared array that we mentioned earlier
		if (is_prime(current)) {
			pthread_mutex_lock(&array_mutex);

			while (shared_array[id] != 0) {
				//Must wait for the consumer thread to consume 
				pthread_cond_wait(&array_cond, &array_mutex);
			}
			shared_array[id] = current; 
			//stored in the shared array that we keep mentioning :)
			//We could output the producer thread and which prime it found ex: printf("Producer thread %d found prime: %d\n", id, local);
			pthread_cond_signal(&array_cond); // We must notify/signal the consumer thread
			pthread_mutex_unlock(&array_mutex); // Can now unlock the 
		}
	}
	pthread_exit(NULL);
}

//Consumer Thread - Using 1 in this program
//Always monitoring the shared array for prime nums.
//If it finds one, it should add it to the total runnign sum of primes.
//When all producers are done and the arr is empty, it should return the final count of primes.
void* consumer_thread(void* arg) {
		while (1) {

			pthread_mutex_lock(&array_mutex);
			int found = 0;

			for (int i = 0; i < THREAD_COUNT; i++) {
				if (shared_array[i] != 0) {
					sum += shared_array[i];
					shared_array[i] = 0; //This clears the slot, 
							     //Forgot to do this originally.
					found = 1;
				}
			}
			if (found) {
				//I had this if statement flipped and I will not admit how long this
				//took me to figure out. Would not unlock and therefore getting stuck
				//in an endless wait situation.
				pthread_cond_broadcast(&array_cond);
				pthread_mutex_unlock(&array_mutex);
			} else {
				
				if (done) {
					int empty = 0;	
					for (int i = 0; i < THREAD_COUNT; i++) {
						if (shared_array[i] != 0) {
							empty = 1;
							break;
						}
					}
	
					if (!empty) {
			
						pthread_mutex_unlock(&array_mutex);
						break;
					}
				}
				//THIS CAUSED ME SO MANY PROBLEMS, check comments for solution
				pthread_cond_wait(&array_cond, &array_mutex);
				pthread_mutex_unlock(&array_mutex);
			}
		}
		//This made sense to me, but you could also exit the thread and just return null
		//or the value and change code accordingly in the main func.
	pthread_exit((void*)(long)sum);
}

int main(int argc, char* argv[]) {
	//Handle input exceptions for correct format/input
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
		//Error handling for producer. These were not thrown in testing, 
		//not sure how or what would cause these to be thrown. Question for nix.
		perror("Failed to create consumer");
		exit(EXIT_FAILURE);
	}

	//producers being launched
	for (int i = 0; i < THREAD_COUNT; i++) {
		ids[i] = i;
		if (pthread_create(&producers[i], NULL, producer_thread, &ids[i])) {
			//error handling for producer
			perror("Failed to create producer");
			exit(EXIT_FAILURE);
		}
	}
	//This waits for all 5 producers to finish
	for (int i = 0; i < THREAD_COUNT; i++) {
		pthread_join(producers[i], NULL);
	}

	//finish - We tell teh consumer that all producers are complete
	pthread_mutex_lock(&array_mutex);
	done = 1;
	pthread_cond_signal(&array_cond);
	pthread_mutex_unlock(&array_mutex);

	
	pthread_join(consumer, NULL);
	
	printf("Sum of primes up to %d: %d\n", upper_limit, sum);
	return 0;
	}
