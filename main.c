//#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>

#define MS_SIZE 5
#define L 10
#define Ni 2
#define N 40

#define RELEASE 0
#define REQUEST 1
#define CONFIRM 2

#define RUNWAY 0
#define HANGAR 1

// love C <3 nie ma funkcji max w standardzie
#define max(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; })

struct Request{
    int id;
    int clk;
};

struct Queue{
    unsigned long size;
    struct Request queue[N];
};

// my id
int my_id = 1;

//confirmation counter
int confirmation_counter = 0;

//declare table of queues
struct Queue queues[L][2];

pthread_mutex_t clock_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t confirmation_counter_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t queue_mutex[L][2] = {{ PTHREAD_MUTEX_INITIALIZER }};

//lamport clock
int l_clock = 0;

void incrementClk1(){
    l_clock++; 
}
void incrementClk2(int i_clock){ 
    l_clock = max(l_clock, i_clock) + 1;
}

void initQueue(struct Queue * qu){
    qu->size = 0;
}

//debug functions
void printRequest(struct Request * req){
    printf("id: %d\tclk: %d\n",req->id, req->clk);
}

void printQueue(struct Queue * qu){
    int i=0;
    printf("queue size: %d\n",qu->size);
    for(i = 0; i < qu->size; i++){
        printRequest(&qu->queue[i]);
    }
}

//insert / remove from queue
void insertQ(struct Queue * qu, struct Request req){
    if(qu->size == 0){
        qu->queue[0] = req;
    } else {
        int i, j;
        for(int i=qu->size; i>=0; i--){
            if(i==0 || (req.clk == qu->queue[i-1].clk && req.id > qu->queue[i].id) || req.clk > qu->queue[i-1].clk ){
                for(j=qu->size -1; j >= i; j--){
                    qu->queue[j+1] = qu->queue[j];
                }
                qu->queue[i] = req;
                break;
            }
        }

    }

    qu->size++;
}

void removeQ(struct Queue * qu, struct Request req){
    //TODO
    int i, j;
    for(i = 0; i<qu->size; i++){
        if(qu->queue[i].id == req.id){
            for(j=i; j<qu->size; j++){
                qu->queue[j] = qu->queue[j+1];
            }
            break;
        }
    }
    qu->size--;
}

int whereIsMyRequest(struct Queue * qu){
    int i = 0;
    for(i=0; i<qu->size; i++){
        if(qu->queue[i].id == my_id) return i;
    }
    return -1;
}

//airplane operations
void flight(){
    //TODO
}

void land(){
    //TODO
}

void takeOff(){
    //TODO
}

void park(){
    //TODO
}

//critical section request / release
void requestCriticalSection(int id1, int id2){
    //TODO
}

void releaseCriticalSection(int id1, int id2){
    //TODO
}

int chooseCarrier(){
    //TODO
    return 0;
}

int main(){

    //init queues
    {
        //int i;
        for(int i = 0; i<L; i++){
            initQueue(&queues[i][RUNWAY]);
            initQueue(&queues[i][HANGAR]);
        }
    }

    //--queue tests--//

    struct Request req1, req2, req3, req4, req5;
    req1.id = 1; req1.clk = 2;
    req2.id = 2; req2.clk = 4;
    req3.id = 3; req3.clk = 3;
    req4.id = 4; req4.clk = 3;
    req5.id = 5; req5.clk = 1;

    //initQueue(&queues[0][0]);

    insertQ(&queues[0][0],req1);
    printQueue(&queues[0][0]);
    insertQ(&queues[0][0],req2);
    printQueue(&queues[0][0]);
    insertQ(&queues[0][0],req3);
    printQueue(&queues[0][0]);
    insertQ(&queues[0][0],req4);
    printQueue(&queues[0][0]);
    insertQ(&queues[0][0],req5);
    printQueue(&queues[0][0]);

    removeQ(&queues[0][0],req5);
    printQueue(&queues[0][0]);

    

    printf("Where %d\n", whereIsMyRequest(&queues[0][0]));

    //--end tests--//

    //TODO init random generator

    //TODO init mpi

    //TODO init threads

    while(1){
        flight();
        int carrier_i_want = chooseCarrier();
        requestCriticalSection(carrier_i_want, HANGAR);
        requestCriticalSection(carrier_i_want, RUNWAY);
        land();
        releaseCriticalSection(carrier_i_want, RUNWAY);
        park();
        requestCriticalSection(carrier_i_want, RUNWAY);
        takeOff();
        releaseCriticalSection(carrier_i_want, RUNWAY);
        releaseCriticalSection(carrier_i_want, HANGAR);
        break;
    }

    //TODO join threads

    //TODO finish mpi

    return 0;
}