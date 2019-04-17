//#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>

#define MSG_SIZE 5
#define MSG_SIZE_CONF 2
#define L 10
#define Ni 2
#define N 40

#define RELEASE 0
#define REQUEST 1
#define CONFIRM 2

#define RUNWAY 0
#define HANGAR 1

// love C <3 nie ma funkcji max w standardzie
#define max(a, b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; })

struct Request
{
    int id;
    int clk;
};

struct Queue
{
    unsigned long size;
    struct Request queue[N];
};

// my id
int my_id = 1;

// my request
struct Request my_request;

//number of processors
int nproc;

//confirmation counter
int confirmation_counter = 0;

//declare table of queues
struct Queue queues[L][2];

pthread_mutex_t clock_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t confirmation_counter_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t queue_mutex[L][2] = {{PTHREAD_MUTEX_INITIALIZER}};

//lamport clock
int l_clock = 0;

void incrementClk1()
{
    pthread_mutex_lock(&clock_mutex);
    l_clock++;
    pthread_mutex_unlock(&clock_mutex);
}
void incrementClk2(int i_clock)
{
    pthread_mutex_lock(&clock_mutex);
    l_clock = max(l_clock, i_clock) + 1;
    pthread_mutex_unlock(&clock_mutex);
}

void initQueue(int id1, int id2)
{
    pthread_mutex_lock(&queue_mutex[id1][id2]);
    queues[id1][id2].size = 0;
    pthread_mutex_unlock(&queue_mutex[id1][id2]);
}

//debug functions
void printRequest(struct Request *req)
{
    printf("id: %d\tclk: %d\n", req->id, req->clk);
}

void printQueue(int id1, int id2)
{
    pthread_mutex_lock(&queue_mutex[id1][id2]);
    struct Queue *qu = &queues[id1][id2];
    int i = 0;
    printf("queue size: %ld\n", qu->size);
    for (i = 0; i < qu->size; i++)
    {
        printRequest(&qu->queue[i]);
    }
    pthread_mutex_unlock(&queue_mutex[id1][id2]);
}

//insert / remove from queue
void insertQ(int id1, int id2, struct Request req)
{
    pthread_mutex_lock(&queue_mutex[id1][id2]);
    struct Queue *qu = &queues[id1][id2];
    if (qu->size == 0)
    {
        qu->queue[0] = req;
    }
    else
    {
        for (int i = qu->size; i >= 0; i--)
        {
            if (i == 0 || (req.clk == qu->queue[i - 1].clk && req.id > qu->queue[i].id) || req.clk > qu->queue[i - 1].clk)
            {
                for (int j = qu->size - 1; j >= i; j--)
                {
                    qu->queue[j + 1] = qu->queue[j];
                }
                qu->queue[i] = req;
                break;
            }
        }
    }
    qu->size++;
    pthread_mutex_unlock(&queue_mutex[id1][id2]);
}

void removeQ(int id1, int id2, struct Request req)
{
    pthread_mutex_lock(&queue_mutex[id1][id2]);
    struct Queue *qu = &queues[id1][id2];
    for (int i = 0; i < qu->size; i++)
    {
        if (qu->queue[i].id == req.id)
        {
            for (int j = i; j < qu->size; j++)
            {
                qu->queue[j] = qu->queue[j + 1];
            }
            break;
        }
    }
    qu->size--;
    pthread_mutex_unlock(&queue_mutex[id1][id2]);
}

int whereIsMyRequest(int id1, int id2)
{
    pthread_mutex_lock(&queue_mutex[id1][id2]);
    struct Queue *qu = &queues[id1][id2];
    int i = 0;
    int ret = -1;
    for (i = 0; i < qu->size; i++)
    {
        if (qu->queue[i].id == my_id)
        {
            ret = i;
            break;
        }
    }
    pthread_mutex_unlock(&queue_mutex[id1][id2]);
    return ret;
}

//airplane operations
void flight()
{
    int t = rand() % 5 + 5;
    printf("%d: Lecę, %d s\n", my_id, t);
    sleep(t);
    printf("%d: Koniec lotu\n", my_id);
}

void land()
{
    printf("%d: Ląduję\n", my_id);
    sleep(1);
}

void takeOff()
{
    printf("%d: Startuję\n", my_id);
    sleep(1);
}

void park()
{
    int t = rand() % 4 + 4;
    printf("%d: Stoję %d s\n", my_id, t);
    sleep(t);
    printf("%d: Koniec stania\n", my_id);
}

//critical section request / release
void requestCriticalSection(int id1, int id2)
{
    printf("%d: Chcę %d %d\n", my_id, id1, id2);
    confirmation_counter = 0;
    my_request.id = my_id;
    my_request.clk = l_clock;

    //place my event in the queue
    insertQ(id1, id2, my_request);

    //send broadcast
    int msg[MSG_SIZE];
    msg[0] = REQUEST;
    msg[1] = my_id;
    msg[2] = l_clock;
    msg[3] = id1;
    msg[4] = id2;
    MPI_Bcast(msg, MSG_SIZE, MSG_INT, my_id, MPI_COMM_WORLD);
    printf("%d: Wysłałem broadcast\n", my_id);

    //initialize receive_thread
    //TODO
    
    //await for confirm
    while (confirmation_counter < nproc - 1){} // active wait

    //await for place in queue
    int position = -1;
    if (id2 == HANGAR)
    {
        position = Ni; //dostęp do hangaru
    }
    else if (id2 == RUNWAY)
    {
        position = 1; //dostęp do pasa
    }
    else
    {
        printf("%d: To nie powinno się stać %d %d\n", my_id, id1, id2);
    }
    while (whereIsMyRequest(id1, id2) >= position){} //active wait

    printf("%d: Wchodzę do %d %d, jestem: %d\n", my_id, id1, id2, whereIsMyRequest(id1, id2));
}

void releaseCriticalSection(int id1, int id2)
{
    printf("%d: Zwalniam %d %d\n", my_id, id1, id2);

    //remove my event
    removeQ(id1, id2, my_request);

    //send broadcast
    int msg[MSG_SIZE];
    msg[0] = RELEASE;
    msg[1] = my_id;
    msg[2] = l_clock;
    msg[3] = id1;
    msg[4] = id2;
    MPI_Bcast(msg, MSG_SIZE, MSG_INT, my_id, MPI_COMM_WORLD);
    printf("%d: Wysłałem broadcast\n", my_id);
}

int chooseCarrier()
{
    return rand() % L;
}

void *broadcast_thread()
{
    printf("%d: Zaczynam wątek odbierający broadcast\n", my_id);
    while (1)
    {
        //receive
        int msg[MSG_SIZE];
        MPI_Bcast(&msg, MSG_SIZE, MPI_INT, my_id, MPI_COMM_WORLD);
        //decide
        struct Request rec_request;
        rec_request.id = msg[1];
        rec_request.clk = msg[2];

        if (msg[0] == RELEASE)
        {
            //remove request from queue
            removeQ(msg[3],msg[4],rec_request);
        }
        else if (msg[0] == REQUEST) {
            //insert request in queue
            insertQ(msg[3],msg[4],rec_request);
        }
        //confirmation
        int msg2[MSG_SIZE_CONF];
        msg2[0] = CONFIRM;
        msg2[1] = my_id;
        MPI_Send(msg2, MSG_SIZE_CONF, MPI_INT, msg[1], MSG_HELLO, MPI_COMM_WORLD);
        
    }

    return 0;
}

void *receive_thread()
{
    printf("%d: Zaczynam wątek odbierający\n", my_id);
    //TODO -> interesują nas id od jakich dostajemy potwierdzenie ?
    while (1)
    {
        //receive
        int msg[MSG_SIZE_CONF];
        MPI_Status status;
        MPI_Recv(msg,MSG_SIZE_CONF, MPI_INT,MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD,&status);
        //increment confirm counter
        if(msg[0] == CONFIRM){
            confirmation_counter++;
        }
    }

    return 0;
}

int main(int argc, char **argv)
{

    //init queues
    {
        for (int i = 0; i < L; i++)
        {
            initQueue(i, RUNWAY);
            initQueue(i, HANGAR);
        }
    }

    // init random generator
    srand(time(0));

    // init mpi
    // MPI_Init(&argc, &argv);
    // MPI_Comm_size(MPI_COMM_WORLD, &nproc );
    // MPI_Comm_rank(MPI_COMM_WORLD, &my_id );

    //TODO init threads
    pthread_t thread1, thread2;
    pthread_create(&thread1, NULL, broadcast_thread, NULL);
    pthread_create(&thread2, NULL, receive_thread, NULL);

    while (1)
    {
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
        break; //comment to run properly
    }

    // join threads
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    // finish mpi
    // MPI_Finalize();

    return 0;
}