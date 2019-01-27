/* NAME: Ofir Cohen
 * ID: 312255847
 * DATE:
 * 
 * This program implements HTTP Server using Threadpool
 * The program is multithreaded
 */

/* INCLUDES */
#include "threadpool.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>


/* DEFINES */
#define TRUE 1
#define ACCEPT 0
#define DONT_ACCEPT 1
#define SHUTDOWN 1
#define NO_SHUTDOWN 0


/* FUNCTIONS */

/**
 * create_threadpool creates a fixed-sized thread
 * pool.  If the function succeeds, it returns a (non-NULL)
 * "threadpool", else it returns NULL.
 * this function should:
 * 1. input sanity check 
 * 2. initialize the threadpool structure
 * 3. initialized mutex and conditional variables
 * 4. create the threads, the thread init function is do_work and its argument is the initialized threadpool. 
 */
threadpool* create_threadpool(int num_threads_in_pool)
{
    // check input
    if(num_threads_in_pool <= 0 || num_threads_in_pool > MAXT_IN_POOL)
        return NULL;

    /* init threadpool and its' values */
    threadpool* tp = (threadpool*)malloc(sizeof(threadpool));
    if(tp == NULL)
        return NULL;

    // set the number of threads in threadpool to be the number that was entered
    tp->num_threads = num_threads_in_pool;
    
    // allocate memory for threads
    tp->threads = (pthread_t*)malloc(sizeof(pthread_t)*num_threads_in_pool);
    if(tp->threads == NULL)
    {
        free(tp);
        return NULL;
    }

    tp->qhead = NULL;
    tp->qtail = NULL;
    tp->qsize = 0;

    // lock for critical sections
    int check = pthread_mutex_init(&tp->qlock, NULL);
    if(check != 0)
    {
        free(tp->threads);
        free(tp);
        return NULL;
    }

    // condition value on size of job list
    check = pthread_cond_init(&tp->q_not_empty, NULL);
    if(check != 0)
    {
        free(tp->threads);
        free(tp);
        return NULL;
    }
    
    // condition value on destroy pool function
    check = pthread_cond_init(&tp->q_empty, NULL);
    if(check != 0)
    {
        free(tp->threads);
        free(tp);
        return NULL;
    }

    // values for destroy pool function
    tp->shutdown = NO_SHUTDOWN;
    tp->dont_accept = ACCEPT;


    // create threads and send them to do_work function with the threadpool as an argument
    int i;
    for(i = 0; i < num_threads_in_pool; i++)
    {
        if(pthread_create(&tp->threads[i], NULL, do_work, (void*)tp) != 0)
        {
            free(tp->threads);
            free(tp);
            return NULL;
        }
    }

    return tp;
}



/**
 * dispatch enter a "job" of type work_t into the queue.
 * when an available thread takes a job from the queue, it will
 * call the function "dispatch_to_here" with argument "arg".
 * this function should:
 * 1. create and init work_t element
 * 2. lock the mutex
 * 3. add the work_t element to the queue
 * 4. unlock mutex
 *
 */
void dispatch(threadpool* from_me, dispatch_fn dispatch_to_here, void *arg)
{
    // lock mutex
    // check if we are starting destroy threadpool process
        // if we are then return from function
    // create new job
    // init new job parameters
    // check if there are jobs in the list
        // if there aren't then head = tail = new job
        // else tail->next = new job, and set the tail to be the new job
    // increase number of jobs in list
    // signal that there is a job in the job list
    // unlock mutex

    if(from_me == NULL || dispatch_to_here == NULL)
        return;

    /* critical section - adding job to job list */
    pthread_mutex_lock(&(from_me->qlock));

    if(from_me->dont_accept == DONT_ACCEPT)
    {
        pthread_mutex_unlock(&from_me->qlock);
        return;
    }

    /* create new job and init its' parameters */
    work_t* work = (work_t*)malloc(sizeof(work_t));
    if(work == NULL)
    {
        pthread_mutex_unlock(&from_me->qlock);
        //TODO: free memory
        return;
    }

    // the job to do
    work->routine = dispatch_to_here;
    
    // argument for the routine
    work->arg = arg;
    
    // next job in queue
    work->next = NULL;

    /* if queue size is 0 then we are inserting the first job, else add to the tail */
    if(from_me->qsize == 0)
    {
        from_me->qhead = work;
        from_me->qtail = work;
    }
    else
    {
        // insert new job to the end of the list
        from_me->qtail->next = work;
        from_me->qtail = from_me->qtail->next;
        from_me->qtail->next = NULL;
    }

    // increase by 1 the size of the queue
    from_me->qsize++;
    
    /* signal that there is a job in the job list */
    pthread_mutex_unlock(&from_me->qlock);
    pthread_cond_signal(&from_me->q_not_empty);
}

/**
 * The work function of the thread
 * this function should:
 * 1. lock mutex
 * 2. if the queue is empty, wait
 * 3. take the first element from the queue (work_t)
 * 4. unlock mutex
 * 5. call the thread routine
 *
 */
void* do_work(void* p)
{
    if(p == NULL)
        return NULL;

    threadpool* tp = (threadpool*)p;

    while(TRUE)
    {        
        pthread_mutex_lock(&(tp->qlock));

        /* check if shutdown process has started */
        if(tp->shutdown == SHUTDOWN)
        {
            pthread_mutex_unlock(&tp->qlock);
            return NULL;
        }

        /* check if there is a job waiting */
        if(tp->qsize == 0)
        {    
            pthread_cond_wait(&(tp->q_not_empty), &(tp->qlock));
        }

        /* check if shutdown process has started */
        if(tp->shutdown == SHUTDOWN)
        {
            pthread_mutex_unlock(&tp->qlock);
            return NULL;
        }
        
        /* taking out a job from job list */
        work_t* work = tp->qhead;
        if(!work)
        {
            pthread_mutex_unlock(&tp->qlock);
            continue;            
        }

        tp->qsize--;

        /* run the function that we took from the job list */
        if(work)
        {
            work->routine(work->arg);
            free(work);
        }

        /* check if the job we took is the only job in the list */
        if(tp->qsize == 0)
        {
            tp->qhead = NULL;
            tp->qtail = NULL;

            /* check if we are in the destroy threadpool process */
            if(tp->dont_accept == DONT_ACCEPT)
            {
                pthread_cond_broadcast(&tp->q_empty);
            }
        }
        /* there are more than 1 job in the list */
        else
        {
            tp->qhead = tp->qhead->next;
        }

        pthread_mutex_unlock(&tp->qlock);
    }
}


/**
 * destroy_threadpool kills the threadpool, causing
 * all threads in it to commit suicide, and then
 * frees all the memory associated with the threadpool.
 */
void destroy_threadpool(threadpool* destroyme)
{    
    if(destroyme == NULL)
        return;
    
    // lock mutex
    // dont accept new jobs to be added to the job list
    pthread_mutex_lock(&destroyme->qlock);
    destroyme->dont_accept = DONT_ACCEPT;


    // if there are jobs in the job list wait untill we don't have any
    if(destroyme->qsize > 0)
        pthread_cond_wait(&destroyme->q_empty, &destroyme->qlock);


    // init destroy threadpool process (shutdown)
    // unlock mutex    
    // wake up all threads
    

    destroyme->shutdown = SHUTDOWN;
    pthread_cond_broadcast(&destroyme->q_not_empty);
    pthread_mutex_unlock(&destroyme->qlock);


    // join all threads
    // destroy lock
    // destroy condition values
    // free threads array
    // free threadpool
    int i;
    for(i = 0; i < destroyme->num_threads; i++)
        pthread_join(destroyme->threads[i], NULL);

    pthread_mutex_destroy(&destroyme->qlock);
    pthread_cond_destroy(&destroyme->q_not_empty);
    pthread_cond_destroy(&destroyme->q_empty);
    free(destroyme->threads);
    free(destroyme);

}
