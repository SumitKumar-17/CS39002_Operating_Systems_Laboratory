#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define SCALE 100

typedef struct{
    int value;
    pthread_mutex_t mtx;
    pthread_cond_t cv;
} semaphore;

void P(semaphore *s){
    pthread_mutex_lock(&s->mtx);
    s->value--;
    if (s->value < 0){
        pthread_cond_wait(&s->cv, &s->mtx);
    }
    pthread_mutex_unlock(&s->mtx);
}

void V(semaphore *s){
    pthread_mutex_lock(&s->mtx);
    s->value++;
    if (s->value <= 0){
        pthread_cond_signal(&s->cv);
    }
    pthread_mutex_unlock(&s->mtx);
}

int m, n;
semaphore boat = {0, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER};
semaphore rider = {0, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER};
pthread_mutex_t bmtx = PTHREAD_MUTEX_INITIALIZER;
pthread_barrier_t EOS;

int *BA;
int *BC;
int *BT;
pthread_barrier_t *BB;

int *vtime;
int *rtime;
int pending_visitors = 0;

void *boat_thread(void *arg){
    int boat_id = *((int *)arg);
    free(arg);

    pthread_barrier_init(&BB[boat_id], NULL, 2);
    printf("Boat %d Ready\n", boat_id);
    fflush(stdout);

    while (1){

        pthread_mutex_lock(&bmtx);
        BA[boat_id] = 1;
        BC[boat_id] = -1;
        pthread_mutex_unlock(&bmtx);

        V(&rider);
        P(&boat);

        pthread_barrier_wait(&BB[boat_id]);

        pthread_mutex_lock(&bmtx);
        int visitor_id = BC[boat_id];
        int ride_duration = BT[boat_id];
        BA[boat_id] = 0;
        pthread_mutex_unlock(&bmtx);

        printf("Boat %d Start of ride for visitor %d\n", boat_id, visitor_id);
        fflush(stdout);
        usleep(ride_duration * 1000 * SCALE);
        printf("Boat %d End of ride for visitor %d (ride time = %d)\n", boat_id, visitor_id, ride_duration);
        fflush(stdout);

        pthread_mutex_lock(&bmtx);
        // printf("--pending visitors %d\n", pending_visitors);
        // fflush(stdout);
        pending_visitors--;
        if (pending_visitors <= 0){
            pthread_mutex_unlock(&bmtx);
            pthread_barrier_wait(&EOS);
            return NULL;
        }
        pthread_mutex_unlock(&bmtx);
    }

    return NULL;
}

void *visitor_thread(void *arg){
    int visitor_id = *((int *)arg);
    free(arg);

    int visit_duration = vtime[visitor_id];
    int ride_duration = rtime[visitor_id];

    printf("Visitor %d Starts sightseeing for %d minutes\n", visitor_id, visit_duration);
    fflush(stdout);

    usleep(visit_duration * 1000 * SCALE);
    printf("Visitor %d Ready to ride a boat (ride time = %d)\n", visitor_id, ride_duration);
    fflush(stdout);

    V(&boat);
    P(&rider);

    int found_boat = -1;
    while (found_boat == -1){
        pthread_mutex_lock(&bmtx);
        for (int i = 1; i <= m; i++){
            if (BA[i] && BC[i] == -1){
                found_boat = i;
                BC[i] = visitor_id;
                BT[i] = ride_duration;
                break;
            }
        }
        pthread_mutex_unlock(&bmtx);

        if (found_boat == -1){
            usleep(1000); // 5ms
        }
    }

    printf("Visitor %d Finds boat %d\n", visitor_id, found_boat);
    fflush(stdout);

    pthread_barrier_wait(&BB[found_boat]);
    usleep(ride_duration * 1000 * SCALE);
    printf("Visitor %d Leaving\n", visitor_id);
    fflush(stdout);

    return NULL;
}

int main(int argc, char *argv[]){
    if (argc != 3){
        fprintf(stderr, "Usage: %s <num_boats> <num_visitors>\n", argv[0]);
        return 1;
    }

    m = atoi(argv[1]);
    n = atoi(argv[2]);

    if (m < 5 || m > 10 || n < 20 || n > 100){
        fprintf(stderr, "Constraints: 5 <= m <= 10, 20 <= n <= 100\n");

        return 1;
    }

    srand(time(NULL));
    pending_visitors = n;

    BA = calloc(m + 1, sizeof(int));
    BC = calloc(m + 1, sizeof(int));
    BT = calloc(m + 1, sizeof(int));
    BB = calloc(m + 1, sizeof(pthread_barrier_t));

    for (int i = 1; i <= m; i++){
        BC[i] = -1;
    }

    vtime = calloc(n + 1, sizeof(int));
    rtime = calloc(n + 1, sizeof(int));

    for (int i = 1; i <= n; i++){
        vtime[i] = 30 + rand() % 91;
        rtime[i] = 15 + rand() % 46;
    }
    pthread_barrier_init(&EOS, NULL, 2);

    pthread_t *boat_threads = calloc(m + 1, sizeof(pthread_t));
    for (int i = 1; i <= m; i++){
        int *id = malloc(sizeof(int));
        *id = i;
        pthread_create(&boat_threads[i], NULL, boat_thread, id);
    }

    pthread_t *visitor_threads = calloc(n + 1, sizeof(pthread_t));
    for (int i = 1; i <= n; i++){
        int *id = malloc(sizeof(int));
        *id = i;
        pthread_create(&visitor_threads[i], NULL, visitor_thread, id);
    }

    pthread_barrier_wait(&EOS);
    // printf("Main thread passed EOS barrier\n");
    fflush(stdout);

    for (int i = 1; i <= m; i++){
        pthread_barrier_destroy(&BB[i]);
    }

    pthread_barrier_destroy(&EOS);
    pthread_mutex_destroy(&bmtx);

    free(boat_threads);
    free(visitor_threads);
    free(BA);
    free(BC);
    free(BT);
    free(BB);
    free(vtime);
    free(rtime);

    return 0;
}