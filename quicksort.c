#include<stdio.h>
#include<stdlib.h>
#include<sys/time.h>
#include<pthread.h>
#include<unistd.h> //to use sleep
#include <sys/time.h>

#define THREADS 4
#define THRESHOLD 100
#define N 950
#define ARRAY_SIZE 100000

// Definitions: types of messages
#define WORK 0
#define FINISH 1
#define SHUTDOWN 2

typedef struct message {
    int type;
    int start;
    int end;
} Message;

Message mqueue[N];
int qin = 0, qout = 0, mcount = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t msg_in = PTHREAD_COND_INITIALIZER;
pthread_cond_t msg_out = PTHREAD_COND_INITIALIZER;

/* 
 * massages
*/
void send(int type, int start, int end){
    pthread_mutex_lock(&mutex);
    while(mcount >= N){
        pthread_cond_wait(&msg_out, &mutex);
    }
    mqueue[qin].type = type;
    mqueue[qin].start = start;
    mqueue[qin].end = end;
    qin = (qin + 1) % N;
    mcount++;
    printf("%d\n", mcount);
    pthread_cond_signal(&msg_in);
    pthread_mutex_unlock(&mutex);
}

void receive(int * type, int * start, int * end) {
    pthread_mutex_lock(&mutex);
    while(mcount < 1){ // perimeno oso den yparxoun mhnymata
        pthread_cond_wait(&msg_in, &mutex);
    }
    *type = mqueue[qout].type;
    *start = mqueue[qout].start;
    *end = mqueue[qout].end;
    // printf("in receive [%d][%d][%d]\n", *type, *start, *end);
    fflush(stdout);
    qout = (qout + 1) % N;
    mcount--;
    pthread_cond_signal(&msg_out);
    pthread_mutex_unlock(&mutex);
}

// About sorting the array
void swap(double * a, double * b){
    double tmp = * a;
    *a = *b;
    *b = tmp;
}

void insertion_sort(double *a, int n){
    int i ,j;
    double t;
    for(i = 1; i < n; i++){
        j = i;
        while((j > 0) && (a[j - 1] > a[j])){
            swap(a+j-1, a+j);
            j--;
        }
    }
}

int partition(double * a, int n){
    int first = 0, middle = n / 2, last = n - 1;
    if (a[first] > a[middle]) {
        swap(a+first, a+middle);
    }
    if (a[middle] > a[last]) {
        swap(a+middle, a+last);
    }
    if (a[first] > a[middle]) {
        swap(a+first, a+middle);
    }
    double p = a[middle];
    int i, j;
    for (i = 1, j = n - 2; ; i++, j++){
        while(a[i] < p) i++;
        while(a[j] > p) j--;
        if (i >= j) break;
        swap(a + i, a + j);
    }
    return i;
}


void * thread_func(void * params){
    double * a = (double *) params;
    int type, start, end;

    while (1){
        receive(&type, &start, &end);
        // printf("I am [%d] and I received type: [%d] message\n", id, type);
        if (type == WORK){
            if ((end - start) < THRESHOLD){
                insertion_sort(a+start, end - start);
                send(FINISH, start, end);
                continue;
            }
            int p = partition(a + start, end - start);
            send(WORK, start, start+p);
            send(WORK, start+p, end);
        } else if(type == SHUTDOWN){
            send(SHUTDOWN, 0, 0);
            break;
        } else {
            send(type, start, end);
        }
    }
    pthread_exit(NULL);
}

int main(){
    pthread_t thread[THREADS];
    int i, complited = 0, type, start, end;
    double * array;
    array = (double *) malloc(ARRAY_SIZE * sizeof(double));
    for (i = 0; i < ARRAY_SIZE; i++){
        *(array+i) = (double) rand()/RAND_MAX;
    }
    for (i = 0; i < THREADS; i++){
        if (pthread_create(&thread[i], NULL, thread_func, array) != 0) {
            printf("Thread creation error\n");
            exit(1);
        }
    }
    send(WORK, 0, ARRAY_SIZE);
    while (1){
        receive(&type, &start, &end);
        if (type == FINISH) {
            complited += end - start;
            // for (i = 0; i < ARRAY_SIZE; i++){
            //     printf("a[%d] = %f,   ", i, *(array+i));
            // }
            
            // printf("qsize = %d\ncomplited = %d\n", mcount, complited);
            if (complited == ARRAY_SIZE){
                printf("FINISH!!!\n");
                break;
            }
        } else {
            send(type, start, end);
        }
    }
    send(SHUTDOWN, 10, 0);
    for (i = 1; i < ARRAY_SIZE; i++){
        if (array[i-1] > array[i]){
            printf("error at %d\n", i);
            break;
        }
    }
    for (i = 0; i < THREADS; i++){
        pthread_join(thread[i], NULL);
    }
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&msg_in);
    pthread_cond_destroy(&msg_out);
    free(array);
    return 0;
}
