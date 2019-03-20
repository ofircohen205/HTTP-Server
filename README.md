Ofir Cohen

ex3 - Computer Communication Course

This program implements HTTP Server using Threadpool
The program is multithreaded

list of files:
threadpool.c
server.c
README.md

how to install the program:
open linux terminal, navigate to the folder containing ex3
using the "cd" command (confirm it by using ls command)
incase you have makefile, type make and the program will
automaticily be compiled, if you don't, type 
gcc threadpool.h threadpool.c server.c -o server -g -Wall -lpthread

and your program will automaticily be compiled

to activate program:
open linux terminal, navigate to ex3 executeable file
location using "cd" command (confirm it using ls command) and type
valgrind ./server <port> <pool-size> <max-number-of-request>


/***************************************************************************************************/

/* THREADPOOL STRUCTS: */
work_t - struct of a job that needed to be done
the job list is a linked list of work_t

threadpool - struct that keeps information about the threadpool such as number of threads,
             current size of job list, the threads, pointer to the head and tail of the list,
             mutex lock on list, condition value on empty list, condition value for destroy_threadpool function, flags for shutdown process and don't accept new jobs


/* THREADPOOL FUNCTIONS: */
threadpool* create_threadpool(int num_threads_in_pool);
input: number of threads to be in the pool
output: pool of threads ready to do work


void dispatch(threadpool* from_me, dispatch_fn dispatch_to_here, void *arg);
input: threadpool, function to execute, arguments of the function
output: inserting new job that needed to be done on the job list


void* do_work(void* p);
input: arguments that being sent from pthread_create function
output: one of the threads in the pool is executing the job in the head of the list


void destroy_threadpool(threadpool* destroyme);
input: threadpool to be destroyed
output: waits until there are no jobs in the list and then starting the killing threadpool process


/***************************************************************************************************/

/* SERVER STRUCT: */
request_t - keeps the path that the client asked for, the type of file, read buffer where we are inserting the client request, write buffer where we are inserting the server response, current time, modified time


/* SERVER FUNCTIONS: */
int create_server(int port);
input: port number where the server will be listening
output: socket fd number


int create_response(void* arg);
input: the fd number that we are getting from accept function
output: we are writing the response for the client, if there is an error in any time in this function then 500 Internal Server error is being sent


int check_input(char* input, request_t* request, int fd);
input: input - what we read from the client, request struct to keep the essential details, the fd where we communicate with the client
output: returns the type of comment we need to send back to client (error, file content or directory content), if there is an error in any time in this function then 500 Internal Server error is being sent


int error_response(request_t* request, int err_type, int fd);
input: request struct to keep the essential details, type of error, the fd where we communicate with the client
output: constructs the error we are sending back to the client and keeps it in write buffer in request_t struct, if there is an error in any time in this function then 500 Internal Server error is being sent


int dir_content(request_t* request, int fd);
input: request struct to keep the essential details, the fd where we communicate with the client 
output: constructs the directory content we are sending back to the client and keeps it in write buffer in request_t struct, if there is an error in any time in this function then 500 Internal Server error is being sent


int file_content(request_t* request, int fd);
input: request struct to keep the essential details, the fd where we communicate with the client 
output: constructs the directory content we are sending back to the client and, if there is an error in any time in this function then 500 Internal Server error is being sent


char* get_mime_type(char* name);
input: the path that the client asked for
output: returns the type of file


int server_error(int fd, request_t* request);
input: request struct to keep the essential details, the fd where we communicate with the client 
output: constructs the Internal Server Error (500) we are sending back to the client and keeps it in write buffer in request_t struct


int check_permissions(char *path, request_t* request, int fd);
input: the path that the client asked for, request struct to keep the essential details, the fd where we communicate with the client
output: checks for each folder in the path if it has execute permissions, if there is an error in any time in this function then 500 Internal Server error is being sent


int get_timebuff(request_t* request, int flag, int fd);
input: the path that the client asked for, request struct to keep the essential details, the fd where we communicate with the client, flag to determine if we want current time or modified time
output: inserts the current time / modified time into the request struct, if there is an error in any time in this function then 500 Internal Server error is being sent


int response_size(request_t* request, int flag, int fd);
input: the path that the client asked for, request struct to keep the essential details, the fd where we communicate with the client, flag to determine the type of response
output: returns the size of bytes we need to allocate for the response


int count_digits(int num);
input: the number we want to check how many digits it has
output: returns the number of digits


int is_number(char* num);
input: string of a number
output: return 0 if it is a number, else returns 1


void free_struct(request_t* request);
input: request struct
output: free all the memory we are allocating


int main(int argc, char* argv[]);
input: size of arguments that being sent from the shell, the arguments
output: multithreaded server
