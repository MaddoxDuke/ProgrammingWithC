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
"primesum.c" [dos] 173L, 5123B                                                                                                                                             106,1-8       63%
