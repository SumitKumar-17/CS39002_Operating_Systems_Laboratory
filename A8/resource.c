#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

#define MAX_RESOURCES 20
#define MAX_THREADS 100
#define MAX_QUEUE_SIZE 100

#define REQ_ADDITIONAL 0     // _request for additional resources 
#define REQ_RELEASE 1        // Release resources 
#define REQ_QUIT 2           // Thread is quitting 

int m,n;
int available[MAX_RESOURCES];
int maxNeed[MAX_THREADS][MAX_RESOURCES];
int allocation[MAX_THREADS][MAX_RESOURCES];
int need[MAX_THREADS][MAX_RESOURCES];

pthread_barrier_t bos; //for beginnning of session
pthread_mutex_t pmtx; //for printing;
pthread_cond_t reqCv; //cv for master thread;
pthread_mutex_t reqMutex; //for req queue
pthread_cond_t threadCv[MAX_THREADS]; //cv fro threads
pthread_mutex_t threadMutex[MAX_THREADS]; //mutex for threads

typedef struct {
    int type;
    int thread;
    int resources[MAX_RESOURCES];
} _request;

typedef struct {
    _request queue[MAX_QUEUE_SIZE];
    int size;
    int front;
    int rear;
} _requestQueue;

_requestQueue requestQueue;

typedef struct{
    int id;
    char filename[100];
} _threadData;

void initializeQueue(_requestQueue *q){
    q->size=0;
    q->front=0;
    q->rear=-1;
}

bool isQueueEmpty(_requestQueue *q){
    return q->size==0;
}

bool isQueueFull(_requestQueue *q){
    return q->size==MAX_QUEUE_SIZE;
}

void enqueueRequest(_requestQueue *q,_request *req){
    if(isQueueFull(q)){
        fprintf(stderr,"Request queue overflow\n");
        exit(1);
    }

    q->rear=(q->rear+1)%MAX_QUEUE_SIZE;
    q->queue[q->rear]=*req;
    q->size++;

    pthread_cond_signal(&reqCv); //signal master
}

bool dequeueRequest(_requestQueue* q, _request* req) {
    if (isQueueEmpty(q)) {
        return false;
    }
    
    *req = q->queue[q->front];
    q->front = (q->front + 1) % MAX_QUEUE_SIZE;
    q->size--;
    
    return true;
}

void printWaitingThreads(){
    _requestQueue *q=&requestQueue;

    if(q->size==0){
        printf("\n");
        return;
    }

    for(int i=0;i< q->size;i++){
        int index=(q->front+i)%MAX_QUEUE_SIZE;
        if(i>0) printf(" ");
        printf("%d",q->queue[index].thread);
    }
    printf("\n");
}

bool isRequestStatisfied(int thread,int request[]){
    for(int i=0;i<m;i++){
        if(request[i]>available[i]) return false;
    }
    return true;
}

bool isSafeState(int thread,int request[]){
    int work[MAX_RESOURCES];
    int finish[MAX_THREADS]={0};
    int count=0;

    for(int i=0;i<m;i++){
        work[i]=available[i]-request[i];
    }

    int tempAllocate[MAX_RESOURCES];
    int tempNeed[MAX_RESOURCES];
    //make temporary allocation of resources 
    for(int i=0;i<m;i++){
        tempAllocate[i]=allocation[thread][i]+request[i];
        tempNeed[i]=need[thread][i]-request[i];
    }

    bool allZero=true;
    for(int i=0;i<m;i++){
        if(tempNeed[i]>0){
            allZero=false;
            break;
        }
    }

    if(allZero){
        finish[thread]=1;
        count++;

        // allocate the new resources to work vector
        for(int i=0;i<m;i++){
            work[i]+=tempAllocate[i];
        }
    }

    //check if other can finish SAFE SEQ check
    bool found;
    do{
        found=false;

        for(int i=0;i<n;i++){
            if(finish[i]==0){
                bool canFinish=true;

                for(int j=0;j<m;j++){
                    if(i==thread){
                        if(tempNeed[j]>work[j]){
                            canFinish=false;
                            break;
                        }
                    }else {
                        if(need[i][j]>work[j]){
                            canFinish=false;
                            break;
                        }
                    }
                }

                if(canFinish){
                    finish[i]=1;
                    count++;
                    found=true;

                    for(int j=0;j<m;j++){
                        if(i==thread){
                            work[j]+=tempAllocate[j];
                        }else{
                            work[j]+=allocation[i][j];
                        }
                    }
                }
            }
        }
    }while(found && count<n);

    //all threads finish safe state
    return count==n;
}

void* masterThread(void* arg) {
    int i, j;
    _request req;
    int activeThreads = n;
    _requestQueue pendingQueue;
    
    initializeQueue(&pendingQueue);
    pthread_barrier_wait(&bos);

    while (activeThreads > 0) {
        pthread_mutex_lock(&reqMutex);
        while (isQueueEmpty(&requestQueue)) {
            pthread_cond_wait(&reqCv, &reqMutex);
        }
        dequeueRequest(&requestQueue, &req);
        pthread_mutex_unlock(&reqMutex);

        if (req.type == REQ_QUIT) {
            activeThreads--;
            
            pthread_mutex_lock(&pmtx);
            printf("Master thread releases resources of thread %d\n", req.thread);
            printf("\t\tWaiting threads: ");
            printWaitingThreads();
            printf("%d threads left: ", activeThreads);
            
            for (i = 0, j = 0; i < n; i++) {
                if(i==req.thread) continue;
                bool thread_alive = false;
                for (int k = 0; k < m; k++) {
                    if (allocation[i][k] > 0) {
                        thread_alive = true;
                        break;
                    }
                }
                if (thread_alive || i == req.thread) {
                    if (j > 0) printf(" ");
                    printf("%d", i);
                    j++;
                }
            }
            printf("\nAvailable resources: ");
            for (i = 0; i < m; i++) {
                available[i] += allocation[req.thread][i];
                allocation[req.thread][i] = 0;
                need[req.thread][i] = 0;
                printf("%d ", available[i]);
            }
            printf("\n");
            printf("\tThread %d going to quit\n", req.thread);
            pthread_mutex_unlock(&pmtx);
        } else if (req.type == REQ_RELEASE) {
            pthread_mutex_lock(&pmtx);
            printf("\tThread %d sends resource request: type = RELEASE\n", req.thread);
            pthread_mutex_unlock(&pmtx);
            
            for (i = 0; i < m; i++) {
                if (req.resources[i] < 0) {
                    allocation[req.thread][i] += req.resources[i];
                    need[req.thread][i] -= req.resources[i];
                    available[i] -= req.resources[i];
                }
            }
            
            pthread_mutex_lock(&pmtx);
            printf("Master thread tries to grant pending requests\n");
            printWaitingThreads();
            pthread_mutex_unlock(&pmtx);
            
            pthread_mutex_lock(&pmtx);
            printf("\tThread %d is done with its resource release request\n", req.thread);
            pthread_mutex_unlock(&pmtx);
        } else if (req.type == REQ_ADDITIONAL) {
            pthread_mutex_lock(&pmtx);
            printf("\tThread %d sends resource request: type = ADDITIONAL\n", req.thread);
            pthread_mutex_unlock(&pmtx);
            
            pthread_mutex_lock(&pmtx);
            printf("Master thread stores resource request of thread %d\n", req.thread);
            printf("\t\tWaiting threads: ");
            
            if (pendingQueue.size == 0) {
                printf("%d\n", req.thread);
            } else {
                printWaitingThreads();
                printf(" %d\n", req.thread);
            }
            pthread_mutex_unlock(&pmtx);
            
            for (i = 0; i < m; i++) {
                if (req.resources[i] < 0) {
                    available[i] -= req.resources[i];
                    allocation[req.thread][i] += req.resources[i];
                    need[req.thread][i] -= req.resources[i];
                    req.resources[i] = 0;
                }
            }
            
            _request pending_req = req;
            enqueueRequest(&pendingQueue, &pending_req);
            
            pthread_mutex_lock(&pmtx);
            printf("Master thread tries to grant pending requests\n");
            pthread_mutex_unlock(&pmtx);
            
            _requestQueue tempQueue;
            initializeQueue(&tempQueue);
            
            while (!isQueueEmpty(&pendingQueue)) {
                _request pending;
                dequeueRequest(&pendingQueue, &pending);
                
                bool canSatisfy = isRequestStatisfied(pending.thread, pending.resources);
                bool isSafe = true;
                
                #ifdef _DLAVOID
                if (canSatisfy) {
                    isSafe = isSafeState(pending.thread, pending.resources);
                }
                #endif
                
                pthread_mutex_lock(&pmtx);
                if (!canSatisfy) {
                    printf("    +++ Insufficient resources to grant request of thread %d\n", pending.thread);
                    enqueueRequest(&tempQueue, &pending);
                } 
                #ifdef _DLAVOID
                else if (!isSafe) {
                    printf("    +++ Unsafe to grant request of thread %d\n", pending.thread);
                    enqueueRequest(&tempQueue, &pending);
                } 
                #endif
                else {
                    printf("Master thread grants resource request for thread %d\n", pending.thread);
                    
                    for (j = 0; j < m; j++) {
                        allocation[pending.thread][j] += pending.resources[j];
                        need[pending.thread][j] -= pending.resources[j];
                        available[j] -= pending.resources[j];
                    }
                    
                    pthread_mutex_lock(&threadMutex[pending.thread]);
                    pthread_cond_signal(&threadCv[pending.thread]);
                    pthread_mutex_unlock(&threadMutex[pending.thread]);
                }
                pthread_mutex_unlock(&pmtx);
            }
            
            while (!isQueueEmpty(&tempQueue)) {
                _request pending;
                dequeueRequest(&tempQueue, &pending);
                enqueueRequest(&pendingQueue, &pending);
            }
            
            pthread_mutex_lock(&pmtx);
            printf("\t\tWaiting threads: ");
            if (pendingQueue.size == 0) {
                printf("\n");
            } else {
                for (i = 0; i < pendingQueue.size; i++) {
                    if (i > 0) printf(" ");
                    printf("%d", pendingQueue.queue[(pendingQueue.front + i) % MAX_QUEUE_SIZE].thread);
                }
                printf("\n");
            }
            pthread_mutex_unlock(&pmtx);
        }
        
        if (pendingQueue.size > 0) {
            _requestQueue tempQueue;
            initializeQueue(&tempQueue);
            bool granted = false;
            
            while (!isQueueEmpty(&pendingQueue)) {
                _request pending;
                dequeueRequest(&pendingQueue, &pending);
                
                bool canSatisfy = isRequestStatisfied(pending.thread, pending.resources);
                bool isSafe = true;
                
                #ifdef _DLAVOID
                if (canSatisfy) {
                    isSafe = isSafeState(pending.thread, pending.resources);
                }
                #endif
                
                if (canSatisfy && isSafe) {
                    granted = true;
                    pthread_mutex_lock(&pmtx);
                    printf("Master thread grants resource request for thread %d\n", pending.thread);
                    pthread_mutex_unlock(&pmtx);
                    
                    for (j = 0; j < m; j++) {
                        allocation[pending.thread][j] += pending.resources[j];
                        need[pending.thread][j] -= pending.resources[j];
                        available[j] -= pending.resources[j];
                    }
                    
                    pthread_mutex_lock(&threadMutex[pending.thread]);
                    pthread_cond_signal(&threadCv[pending.thread]);
                    pthread_mutex_unlock(&threadMutex[pending.thread]);
                } else {
                    enqueueRequest(&tempQueue, &pending);
                }
            }
            
            while (!isQueueEmpty(&tempQueue)) {
                _request pending;
                dequeueRequest(&tempQueue, &pending);
                enqueueRequest(&pendingQueue, &pending);
            }
            
            if (granted) {
                pthread_mutex_lock(&pmtx);
                printf("\t\tWaiting threads: ");
                if (pendingQueue.size == 0) {
                    printf("\n");
                } else {
                    for (i = 0; i < pendingQueue.size; i++) {
                        if (i > 0) printf(" ");
                        printf("%d", pendingQueue.queue[(pendingQueue.front + i) % MAX_QUEUE_SIZE].thread);
                    }
                    printf("\n");
                }
                pthread_mutex_unlock(&pmtx);
            }
        }
    }
    
    return NULL;
}


void* userThread(void* arg) {
    _threadData* data = (_threadData*)arg;
    int threadId = data->id;
    FILE* threadFile = fopen(data->filename, "r");
    int i, delay;
    bool isAdditional;
    
    if (!threadFile) {
        perror("Error opening thread file");
        exit(1);
    }
    
    for (i = 0; i < m; i++) {
        fscanf(threadFile, "%d", &maxNeed[threadId][i]);
        need[threadId][i] = maxNeed[threadId][i];
    }
    
    pthread_mutex_lock(&pmtx);
    printf("\tThread %d born\n", threadId);
    pthread_mutex_unlock(&pmtx);
    
    pthread_barrier_wait(&bos);
    
    char requestType;
    while (fscanf(threadFile, "%d %c", &delay, &requestType) == 2) {
        usleep(delay * 1000);
        
        if (requestType == 'Q') {
            _request req;
            req.type = REQ_QUIT;
            req.thread = threadId;
            
            pthread_mutex_lock(&reqMutex);
            enqueueRequest(&requestQueue, &req);
            pthread_mutex_unlock(&reqMutex);
            
            break;
        } else if (requestType == 'R') {
            _request req;
            isAdditional = false;
            
            req.thread = threadId;
            
            for (i = 0; i < m; i++) {
                fscanf(threadFile, "%d", &req.resources[i]);
                if (req.resources[i] > 0) {
                    isAdditional = true;
                }
            }
            
            if (isAdditional) {
                req.type = REQ_ADDITIONAL;
                
                pthread_mutex_lock(&reqMutex);
                enqueueRequest(&requestQueue, &req);
                pthread_mutex_unlock(&reqMutex);
                
                pthread_mutex_lock(&threadMutex[threadId]);
                pthread_cond_wait(&threadCv[threadId], &threadMutex[threadId]);
                pthread_mutex_unlock(&threadMutex[threadId]);
                
                pthread_mutex_lock(&pmtx);
                printf("\tThread %d is granted its last resource request\n", threadId);
                pthread_mutex_unlock(&pmtx);
            } else {
                req.type = REQ_RELEASE;
                
                pthread_mutex_lock(&reqMutex);
                enqueueRequest(&requestQueue, &req);
                pthread_mutex_unlock(&reqMutex);
            }
        }
    }
    
    fclose(threadFile);
    return NULL;
}

int main(){
    FILE *systemFile;
    pthread_t * threads;
    _threadData * threadData;

    pthread_mutex_init(&pmtx,NULL);
    pthread_mutex_init(&reqMutex,NULL);
    pthread_cond_init(&reqCv,NULL);
    initializeQueue(&requestQueue);


    for(int i=0;i<MAX_THREADS;i++){
        pthread_cond_init(&threadCv[i],NULL);
        pthread_mutex_init(&threadMutex[i],NULL);
    }

    systemFile =fopen("input/system.txt","r");
    if(!systemFile){
        perror("Error opening the file.Not present.");
        exit(1);
    }

    fscanf(systemFile,"%d %d",&m,&n);
    for(int i=0;i<m;i++){
        fscanf(systemFile,"%d",&available[i]);
    }
    fclose(systemFile);

    //0 index is for the masterThread
    //reast are n user threads
    threads=malloc((n+1)*sizeof(pthread_t));
    threadData=malloc(n*sizeof(_threadData));


    for(int i=0;i<n;i++){
        for(int j=0;j<m;j++){
            allocation[i][j]=0;
            need[i][j]=0;
            maxNeed[i][j]=0;
        }
        threadData[i].id=i;
        sprintf(threadData[i].filename,"input/thread%02d.txt",i);
    }

    pthread_barrier_init(&bos,NULL,n+1);
    pthread_create(&threads[0],NULL,masterThread,NULL);

    for(int i=0;i<n;i++){
        pthread_create(&threads[i+1],NULL,userThread,&threadData[i]);
    }

    for(int i=0;i<=n;i++){
        pthread_join(threads[i],NULL);
    }

    for(int i=0;i<n;i++){
        pthread_cond_destroy(&threadCv[i]);
        pthread_mutex_destroy(&threadMutex[i]);
    }

    pthread_barrier_destroy(&bos);
    pthread_cond_destroy(&reqCv);
    pthread_mutex_destroy(&reqMutex);
    pthread_mutex_destroy(&pmtx);
    free(threads);
    free(threadData);

    return 0;
}



