/* NAME: Ofir Cohen
 * ID: 312255847
 * DATE:
 * 
 * This program implements HTTP Server using Threadpool
 * The program is multithreaded
 */

/* INCLUDES */
#include "threadpool.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <time.h>
#include <errno.h>
#include <dirent.h>
#include <signal.h>


/* DEFINES */
#define SUCCESS 0
#define FAILED 1
#define BUFFER_SIZE 4000
#define QUEUE_SIZE 5
#define TIME_NOW 2
#define TIME_MOD 3
#define MIN_PORT 0
#define MAX_PORT 65535

#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"
#define USAGE_ERR "Usage: server <port> <pool-size> <max-number-of-request>\n"

#define FOUND 302
#define BAD_REQUEST 400
#define FORBIDDEN 403
#define NOT_FOUND 404
#define INTERNAL_ERROR 500
#define NOT_SUPPORTED 501
#define OK 200
#define DIR_CONTENT 100
#define FILE_CONTENT 101


/* STRUCTS */
typedef struct request_st{
    char* path;
    char* mime;
    char* read_buff;
    char* write_buff;
    char* time_now;
    char* time_mod;
} request_t;


/* FUNCTIONS */
int create_server(int port);
int create_response(void* arg);
int check_input(char* input, request_t* request, int fd);
int error_response(request_t* request, int err_type, int fd);
int dir_content(request_t* request, int fd);
int file_content(request_t* request, int fd);
char* get_mime_type(char* name);
int server_error(int fd, request_t* request);
int check_permissions(char *path, request_t* request, int fd);
int get_timebuff(request_t* request, int flag, int fd);
int response_size(request_t* request, int flag, int fd);
int count_digits(int num);
int is_number(char* num);
void free_struct(request_t* request);


/* MAIN FUNCTION */
int main(int argc, char* argv[])
{
    /* check number of arguments */
    if(argc != 4)
    {
        printf(USAGE_ERR);
        exit(FAILED);
    }

    /* check if each input is a number */
    if(is_number(argv[1]) == FAILED || is_number(argv[2]) == FAILED || is_number(argv[3]) == FAILED)
    {
        printf(USAGE_ERR);
        exit(FAILED);
    }

    /* all the inputs are definitly numbers */
    int port = atoi(argv[1]);
    int num_of_threads = atoi(argv[2]);
    int max_requests = atoi(argv[3]);

    /* check if port is between it's bounderies and max request is a positive number */
    if(port <= MIN_PORT || port > MAX_PORT || max_requests <= 0)
    {
        printf(USAGE_ERR);
        exit(FAILED);
    }

    int sockfd = create_server(port);
    threadpool* tp = create_threadpool(num_of_threads);
    if(tp == NULL)
    {
        printf(USAGE_ERR);
        close(sockfd);
        exit(FAILED);
    }

    int* fds = (int*)malloc(sizeof(int)*max_requests);
    if(fds == NULL)
    {
        printf("error on allocating memory\r\n");
        destroy_threadpool(tp);
        close(sockfd);
        exit(FAILED);
    }
    int i;
    for(i = 0; i < max_requests; i++)
    {
        fds[i] = accept(sockfd, NULL, NULL);
        if(fds[i] < 0)
        {
            perror("accept");
            destroy_threadpool(tp);
            close(sockfd);
            exit(FAILED);
        }
        dispatch(tp, create_response, (void*)&fds[i]);
    }

    destroy_threadpool(tp);
    close(sockfd);
    free(fds);
    return SUCCESS;
}


/* create socket descriptor where the server is listening to requests */
int create_server(int port)
{
    int sockfd;
    struct sockaddr_in srv;

    if((sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket");
        exit(FAILED);
    }

    /* set internet family, server port, receiving ips */
    srv.sin_family = AF_INET;
    srv.sin_port = htons(port);        
    srv.sin_addr.s_addr = htonl(INADDR_ANY);

    /* bind socket to the server */
    if((bind(sockfd, (struct sockaddr*)&srv, sizeof(srv))) < 0)
    {
        perror("bind");
        exit(FAILED);
    }

    /* socket queue is 5 */
    if(listen(sockfd, QUEUE_SIZE) < 0)
    {
        perror("listen");
        exit(FAILED);
    }

    return sockfd;
}


/* this is the function where we are been sent from dispatch, it creates the response for the client */
int create_response(void* arg)
{
    /* getting the socket where the client is talking with us */
    int* fd_pointer = (int*)arg;
    int fd = *fd_pointer;

    /* creating request struct to keep variables that are necessery for response like path */
    request_t* request = (request_t*)malloc(sizeof(request_t));
    if(request == NULL)
    {
        server_error(fd, request);
        return FAILED;
    }
    bzero(request, sizeof(request_t));

    /* create buffer for reading request from client into buffer and read the request */
    request->read_buff = (char*)malloc(sizeof(char)*BUFFER_SIZE);
    if(request->read_buff == NULL)
    {
        server_error(fd, request);
        exit(FAILED);
    }
    bzero(request->read_buff, BUFFER_SIZE);

    int nbytes = 0;
    nbytes = read(fd, request->read_buff, BUFFER_SIZE);
    if(nbytes < 0)
    {
        perror("read");
        server_error(fd, request);
        exit(FAILED);
    }

    /* check the type of response we need to send back */
    int type = check_input(request->read_buff, request, fd);
    
    if(type == FAILED)
        server_error(fd, request);

    else
    {
        /* write response to write buffer in request struct and send back to client */
        int check;
        switch(type)
        {
            case BAD_REQUEST:
                check = error_response(request, BAD_REQUEST, fd);
                break;
            
            case NOT_SUPPORTED:
                check = error_response(request, NOT_SUPPORTED, fd);
                break;

            case NOT_FOUND:
                check = error_response(request, NOT_FOUND, fd);
                break;    

            case FOUND:
                check = error_response(request, FOUND, fd);
                break;

            case FORBIDDEN:
                check = error_response(request, FORBIDDEN, fd);
                break;
            
            case DIR_CONTENT:
                check = dir_content(request, fd);
                break;

            case FILE_CONTENT:
                check = file_content(request, fd);
                break;
        }


        /* error types and directory content are being sent here to client, file content */
        if(type != FILE_CONTENT && check != FAILED)
        {
            nbytes = write(fd, request->write_buff, strlen(request->write_buff));
            if(nbytes < 0)
            {
                perror("write");
                server_error(fd, request);
                exit(FAILED);
            }
        }
    }

    /* each struct is for one request so we need to free it and close socket */
    free_struct(request);
    close(fd);
    return SUCCESS;
}


/* the function return what type of response we need to answer to the client, it keeps essentials variables in request struct */
int check_input(char* input, request_t* request, int fd)
{
    if(input == NULL)
        return BAD_REQUEST;

    
    char* local_input = (char*)malloc(sizeof(char)*(strlen(input)+1));
    strcpy(local_input, input);

    char* method;
    char* path;
    char* version;
    
    /* CHECK FOR INPUT */
    /* get from input METHOD PATH VERSION */ 
    method = strtok(local_input, " ");
    if(!method)
    {
        free(local_input);
        return BAD_REQUEST;
    }

    /* get the path from the request and the type of file and insert into the struct */
    path = strtok(NULL, " ");
    if(!path)
    {
        free(local_input);
        return BAD_REQUEST; 
    }

    /* get the version of the http request and insert into the struct */
    version = strtok(NULL, "\r");
    if(!version)
    {
        free(local_input);
        return BAD_REQUEST;
    }

    if(strcmp(version, "HTTP/1.0") != 0 && strcmp(version, "HTTP/1.1") != 0)
    {
        free(local_input);
        return BAD_REQUEST;
    }

    /* SUPPORT ONLY GET METHOD */
    /* check if the method is get */
    if(strcmp(method, "GET") != 0)
    {
        free(local_input);
        return NOT_SUPPORTED;
    }

    /* check if the client is asking for the main directory of the server */
    if(strlen(path) == 1 && strcmp(path, "/") == 0)
    {
        path = "/./";
    }

    /* we don't need the first '/' for response */
    if(strlen(path) > 1)
        path++;


    /* CHECK IF THE PATH EXISTS */
    /* there is no such path, then NOT FOUND */
    struct stat fileStat;
    if(stat(path, &fileStat) < 0)
    {
        if(errno == ENOENT || (errno == ENOTDIR && path[strlen(path)-1] == '/'))
        {
            free(local_input);
            return NOT_FOUND;
        }
        else
        {
            server_error(fd, request);
            return FAILED;
        }
    }
    
    else
    {
        /* check if each folder in path has execute permissions */
        int check = check_permissions(path, request, fd);

        /* if it is a path of directory */
        if(S_ISDIR(fileStat.st_mode))
        {
            /* if it doesn't ends with '/' then return FOUND response */
            if(path[strlen(path)-1] != '/')
            {
                // keep path in struct for further uses
                request->path = (char*)malloc(sizeof(char)*(strlen(path)+2));
                bzero(request->path, (strlen(path)+1));
                sprintf(request->path, "/%s", path);
                
                free(local_input);
                return FOUND;
            }

            /* one of the folders doesn't have execute permissions */
            if(check == FAILED)
            {
                free(local_input);
                return FORBIDDEN;
            }

            /* path is bad */
            else if(check == NOT_FOUND)
            {
                free(local_input);
                return NOT_FOUND;
            }

            /* it is a path of directory and it ends with '/' then: */
            if(path[strlen(path)-1] == '/')
            {
                /* 1. look up for index.html file */
                char* index = (char*)malloc(sizeof(char)*(strlen(path) + strlen("index.html") + 1));
                if(index == NULL)
                {
                    server_error(fd, request);
                    return FAILED;
                }
                bzero(index, sizeof(char)*(strlen(path) + strlen("index.html") + 1));
                sprintf(index, "%sindex.html", path);

                
                /* if there is such file and other has read permissions then call file_content */
                if((stat(index, &fileStat) >= 0) && (fileStat.st_mode & S_IROTH) && (check == SUCCESS))
                {
                    /* keep path in struct for further uses */
                    request->path = (char*)malloc(sizeof(char)*(strlen(index)+1));
                    bzero(request->path, (strlen(index)+1));
                    strcpy(request->path, index);

                    free(index);
                    free(local_input);
                    return FILE_CONTENT;
                }

                /* 2. else return dir_content */        
                else
                {
                    /* keep path in struct for further uses */
                    request->path = (char*)malloc(sizeof(char)*(strlen(path)+1));
                    bzero(request->path, (strlen(path)+1));
                    strcpy(request->path, path);

                    free(index);
                    free(local_input);
                    return DIR_CONTENT;
                }
            }
        }

        /* if it isn't a regular file then return FORBIDDEN */
        if(!S_ISREG(fileStat.st_mode))
        {
            free(local_input);
            return FORBIDDEN;
        }

        /* the caller doesn't have read permissions then return FORBIDDEN */
        else if((stat(path, &fileStat) >= 0) && !(fileStat.st_mode & S_IROTH) && (check == SUCCESS))
        {
            free(local_input);
            return FORBIDDEN;
        }

        /* if the path is a file */
        if(S_ISREG(fileStat.st_mode) && (S_IROTH) && (check == SUCCESS))
        {
            /* keep path in struct for further uses */
            request->path = (char*)malloc(sizeof(char)*(strlen(path)+1));
            bzero(request->path, (strlen(path)+1));
            strcpy(request->path, path);

            free(local_input);
            return FILE_CONTENT;
        }   
    }
    
    free(local_input);
    return FAILED;
}


/* if there is a problem in the request of the client then return this error */
int error_response(request_t* request, int err_type, int fd)
{
    /* get current date and insert into request struct */
    int check;
    check = get_timebuff(request, TIME_NOW, fd);
    if(check == FAILED)
        return FAILED;

    /* get size of error response and insert response into write buff in request so we will send it after */
    int size = 0;
    switch(err_type)
    {
        case FOUND:
            size = response_size(request, FOUND, fd);
            request->write_buff = (char*)malloc(sizeof(char)*(size));
            if(request->write_buff == NULL)
            {
                server_error(fd, request);
                return FAILED;
            }
            bzero(request->write_buff, size);
            sprintf(request->write_buff, "HTTP/1.0 302 Found\r\nServer: webserver/1.0\r\nDate: %s\r\nLocation: %s/\r\nContent-Type: text/html\r\nContent-Length: 123\r\nConnection: close\r\n\r\n", request->time_now, request->path);
            sprintf(request->write_buff + strlen(request->write_buff), "<HTML><HEAD><TITLE>302 Found</TITLE></HEAD>\r\n<BODY><H4>302 Found</H4>\r\nDirectories must end with a slash.\r\n</BODY></HTML>");
            break;


        case BAD_REQUEST:
            size = response_size(request, BAD_REQUEST, fd);
            request->write_buff = (char*)malloc(sizeof(char)*(size));
            if(request->write_buff == NULL)
            {
                server_error(fd, request);
                return FAILED;
            }
            bzero(request->write_buff, size);
            sprintf(request->write_buff, "HTTP/1.0 400 Bad Request\r\nServer: webserver/1.0\r\nDate: %s\r\nContent-Type: text/html\r\nContent-Length: 113\r\nConnection: close\r\n\r\n", request->time_now);
            sprintf(request->write_buff + strlen(request->write_buff), "<HTML><HEAD><TITLE>400 Bad Request</TITLE></HEAD>\r\n<BODY><H4>400 Bad Request</H4>\r\nBad Request.\r\n</BODY></HTML>");
            break;

        case FORBIDDEN:
            size = response_size(request, FORBIDDEN, fd);
            request->write_buff = (char*)malloc(sizeof(char)*(size));
            if(request->write_buff == NULL)
            {
                server_error(fd, request);
                return FAILED;
            }
            bzero(request->write_buff, size);
            sprintf(request->write_buff, "HTTP/1.0 403 Forbidden\r\nServer: webserver/1.0\r\nDate: %s\r\nContent-Type: text/html\r\nContent-Length: 111\r\nConnection: close\r\n\r\n", request->time_now);
            sprintf(request->write_buff + strlen(request->write_buff), "<HTML><HEAD><TITLE>403 Forbidden</TITLE></HEAD>\r\n<BODY><H4>403 Forbidden</H4>\r\nAccess denied.\r\n</BODY></HTML>");
            break;

        case NOT_FOUND:
            size = response_size(request, NOT_FOUND, fd);
            request->write_buff = (char*)malloc(sizeof(char)*(size));
            if(request->write_buff == NULL)
            {
                server_error(fd, request);
                return FAILED;
            }
            bzero(request->write_buff, size);
            sprintf(request->write_buff, "HTTP/1.0 404 Not Found\r\nServer: webserver/1.0\r\nDate: %s\r\nContent-Type: text/html\r\nContent-Length: 112\r\nConnection: close\r\n\r\n", request->time_now);            
            sprintf(request->write_buff + strlen(request->write_buff), "<HTML><HEAD><TITLE>404 Not Found</TITLE></HEAD>\r\n<BODY><H4>404 Not Found</H4>\r\nFile not found.\r\n</BODY></HTML>");
            break;

        case NOT_SUPPORTED:
            size = response_size(request, NOT_SUPPORTED, fd);
            request->write_buff = (char*)malloc(sizeof(char)*(size));
            if(request->write_buff == NULL)
            {
                server_error(fd, request);
                return FAILED;
            }
            bzero(request->write_buff, size);
            sprintf(request->write_buff, "HTTP/1.0 501 Not supported\r\nServer: webserver/1.0\r\nDate: %s\r\nContent-Type: text/html\r\nContent-Length: 129\r\nConnection: close\r\n\r\n", request->time_now);
            sprintf(request->write_buff + strlen(request->write_buff), "<HTML><HEAD><TITLE>501 Not supported</TITLE></HEAD>\r\n<BODY><H4>501 Not supported</H4>\r\nMethod is not supported.\r\n</BODY></HTML>");
            break;
    }

    return SUCCESS;
}


/* return the cotent of the directory */
int dir_content(request_t* request, int fd)
{
    /* get the size of response */
    int size_b_title = 0;
    int size_b_body = 0;
    int size_b = 0;
    int size_h = 0;

    size_b_title += strlen("<HTML><HEAD><TITLE>Index of ") + strlen(request->path) + strlen("</TITLE></HEAD>") + strlen("\r\n");
    size_b_title += strlen("<BODY><H4>Index of ") + strlen(request->path) + strlen("</H4>") + strlen("\r\n");
    size_b_body += strlen("<table CELLSPACING=8>") + strlen("\r\n");
    size_b_body += strlen("<tr><th>Name</th><th>Last Modified</th><th>Size</th></tr>");
    size_b_body += strlen("</table>") + strlen("\r\n");
    size_b_body += strlen("<HR>") + strlen("\r\n");
    size_b_body += strlen("<ADDRESS>webserver/1.0</ADDRESS>") + strlen("\r\n");
    size_b_body += strlen("</BODY></HTML>");

    char* body_response;

    struct stat dirStat;
    if(stat(request->path, &dirStat) < 0)
    {
        server_error(fd, request);
        return FAILED;
    }

    struct dirent **namelist;
    int num, i = 0;

    /* get the number of items inside the directory */
    num = scandir(request->path, &namelist, NULL, alphasort);
    if (num == -1) 
    {
        server_error(fd, request);
        return FAILED;
    }

    /* add more details to size response */
    while (i < num) 
    {
        char* file = (char*)malloc(sizeof(char)*(strlen(request->path) + strlen(namelist[i]->d_name) + 1));
        if(file == NULL)
        {
            server_error(fd, request);
            return FAILED;
        }
        bzero(file, sizeof(char)*(strlen(request->path) + strlen(namelist[i]->d_name) + 1));
        sprintf(file, "%s%s", request->path, namelist[i]->d_name);

        struct stat fileStat;
        if(stat(file, &fileStat) < 0)
        {
            server_error(fd, request);
            free(file);
            return FAILED;
        }

        /* get last modified time */
        char file_last_modified[128];
        strftime(file_last_modified, sizeof(file_last_modified), RFC1123FMT, gmtime(&fileStat.st_mtime));

        size_b_body += strlen("<tr>") + strlen("\r\n");
        size_b_body += strlen("<td><A HREF=\"") + strlen(file) + strlen("\">") + strlen("</A>") + strlen(namelist[i]->d_name) + strlen("</td>");
        size_b_body += strlen("<td>") + strlen(file_last_modified) + strlen("</td>");
        size_b_body += strlen("<td>") + count_digits(fileStat.st_size) + strlen("</td>");
        size_b_body += strlen("</tr>") + strlen("\r\n") + 10;

        free(file);
        i++;
    }
    char* title = (char*)malloc(sizeof(char)*(strlen(request->path) + strlen("Index of ") + 1));
    if(title == NULL)
    {
        server_error(fd, request);
        return FAILED;
    }
    bzero(title, sizeof(char)*(strlen(request->path) + strlen("Index of ") + 1));
    sprintf(title, "Index of %s", request->path);

    char* body = (char*)malloc(sizeof(char)*(size_b_body + 1));
    if(body == NULL)
    {
        server_error(fd, request);
        free(title);
        return FAILED;
    }
    bzero(body, sizeof(char)*(size_b_body + 1));
    sprintf(body, "<table CELLSPACING=8>\r\n<tr><th>Name</th><th>Last Modified</th><th>Size</th></tr>\r\n");

    i = 0;
    while(i < num)
    {
        char* file = (char*)malloc(sizeof(char)*(strlen(request->path) + strlen(namelist[i]->d_name) + 1));
        if(file == NULL)
        {
            server_error(fd, request);
            free(body);
            free(title);
            return FAILED;
        }
        bzero(file, sizeof(char)*(strlen(request->path) + strlen(namelist[i]->d_name) + 1));
        sprintf(file, "%s%s", request->path, namelist[i]->d_name);

        struct stat fileStat;
        if(stat(file, &fileStat) < 0)
        {
            server_error(fd, request);
            free(file);
            free(body);
            free(title);
            while(namelist[i])
                free(namelist[i++]);
            free(namelist);
            return FAILED;
        }

        /* get last modified time */
        char file_last_modified[128];
        strftime(file_last_modified, sizeof(file_last_modified), RFC1123FMT, gmtime(&fileStat.st_mtime));

        if(S_ISDIR(fileStat.st_mode))
            sprintf(body + strlen(body), "<tr>\r\n<td><A HREF=\"%s/\">%s</A></td><td>%s</td>\r\n<td></td>\r\n</tr>\r\n", namelist[i]->d_name, namelist[i]->d_name, file_last_modified);
        
        else
            sprintf(body + strlen(body), "<tr>\r\n<td><A HREF=\"%s\">%s</A></td><td>%s</td>\r\n<td>%d</td>\r\n</tr>\r\n", namelist[i]->d_name, namelist[i]->d_name, file_last_modified, (int)fileStat.st_size);

        free(namelist[i]);
        free(file);
        i++;
    }
    sprintf(body + strlen(body), "</table>\r\n<HR>\r\n<ADDRESS>webserver/1.0</ADDRESS>");
    free(namelist);

    size_b += size_b_body + size_b_title;
    body_response = (char*)malloc(sizeof(char)*(size_b + 1));
    if(body_response == NULL)
    {
        server_error(fd, request);
        free(body);
        free(title);
        return FAILED;
    }
    bzero(body_response, sizeof(char)*(size_b + 1));


    sprintf(body_response, "<HTML><HEAD><TITLE>%s</TITLE></HEAD>\r\n<BODY><H4>%s</H4>\r\n%s\r\n</BODY></HTML>", title, title, body);

    int check;
    check = get_timebuff(request, TIME_NOW, fd);
    if(check == FAILED)
        return FAILED;

    check = get_timebuff(request, TIME_MOD, fd);
    if(check == FAILED)
        return FAILED;

    char* header;
    size_h += strlen("HTTP/1.0 200 OK") + strlen("\r\n");
    size_h += strlen("Server: webserver/1.0") + strlen("\r\n");
    size_h += strlen("Date: ") + strlen(request->time_now) + strlen("\r\n");
    size_h += strlen("Content-Type: ") + strlen("text/html") + strlen("\r\n");
    size_h += strlen("Content-Length: ") + strlen(body) + strlen("\r\n");
    size_h += strlen("Last-Modified: ") + strlen(request->time_mod) + strlen("\r\n");
    size_h += strlen("Connection: close") + strlen("\r\n\r\n");

    header = (char*)malloc(sizeof(char)*(size_h + 1));
    if(header == NULL)
    {
        server_error(fd, request);
        free(body);
        free(title);
        free(body_response);
        return FAILED;
    }

    sprintf(header, "HTTP/1.0 200 OK\r\nServer: webserver/1.0\r\nDate: %s\r\nContent-Type: text/html\r\nContent-Length: %d\r\nLast-Modified: %s\r\nConnection: close\r\n\r\n", request->time_now, (int)strlen(body_response), request->time_mod);

    request->write_buff = (char*)malloc(sizeof(char)*(strlen(header) + strlen(body_response) + 1));
    if(request->write_buff == NULL)
    {
        server_error(fd, request);
        free(body);
        free(title);
        free(body_response);
        return FAILED;
    }
    bzero(request->write_buff, sizeof(char)*(strlen(header) + strlen(body_response) + 1));
    sprintf(request->write_buff, "%s%s", header, body_response);

    free(title);
    free(body);
    free(body_response);
    free(header);

    return SUCCESS;
}


/* return the file content */
int file_content(request_t* request, int fd)
{
    /* get current time and modified time */    
    int check;
    check = get_timebuff(request, TIME_NOW, fd);
    if(check == FAILED)
        return FAILED;

    check = get_timebuff(request, TIME_MOD, fd);
    if(check == FAILED)
        return FAILED;

    /* get mime type */
    char* mime = get_mime_type(request->path);
    if(mime)
    {
        request->mime = (char*)malloc(sizeof(char)*(strlen(mime)+1));
        bzero(request->mime, (strlen(mime)+1));
        strcpy(request->mime, mime);
    }

    int size = response_size(request, OK, fd);
    request->write_buff = (char*)malloc(sizeof(char)*(size));
    if(request->write_buff == NULL)
    {
        server_error(fd, request);
        return FAILED;
    }
    bzero(request->write_buff, size);
    
    /* stat struct for getting the size of the file */
    struct stat fileStat;
    if(stat(request->path, &fileStat) < 0)
    {
        server_error(fd, request);
        return FAILED;
    }

    sprintf(request->write_buff, "HTTP/1.0 200 OK\r\nServer: webserver/1.0\r\nDate: %s\r\n", request->time_now);

    if(request->mime)
        sprintf(request->write_buff + strlen(request->write_buff), "Content-Type: %s\r\nContent-Length: %d\r\nLast-Modified: %s\r\nConnection: close\r\n\r\n", request->mime, (int)fileStat.st_size, request->time_mod);

    else
        sprintf(request->write_buff + strlen(request->write_buff), "Content-Length: %d\r\nLast-Modified: %s\r\nConnection: close\r\n\r\n", (int)fileStat.st_size, request->time_mod);        

    
    /* write header of response to client */
    int bytes_write = 0;
    bytes_write = write(fd, request->write_buff, strlen(request->write_buff));
    if(bytes_write < 0)
    {
        server_error(fd, request);
        return FAILED;
    }

    /* open file to get data and write it to the client */
    int file_fd;
    if((file_fd = open(request->path, O_RDONLY)) < 0)
    {
        server_error(fd, request);
        return FAILED;
    }

    unsigned char buffer[512];
    bzero(buffer, 512);
    int bytes_read = 0;
    bytes_write = 0;
    while(1)
    {
        bytes_read = read(file_fd, buffer, 512 - 1);
        /* reached EOF */
        if(bytes_read == 0)
            break;
        if(bytes_read < 0)
        {
            server_error(fd, request);
            return FAILED;
        }
        else
        {
            bytes_write = write(fd, buffer, bytes_read);
            if(bytes_write < 0)
            {
                server_error(fd, request);
                return FAILED;
            }

        }
        bzero(buffer, 512);
    }
    close(file_fd);
    return SUCCESS;
}


/* returns the type of file to be asked in the request */
char* get_mime_type(char* name)
{
    char *ext = strrchr(name, '.');
    if (!ext) return NULL;
    if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) return "text/html";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".gif") == 0) return "image/gif";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".css") == 0) return "text/css";
    if (strcmp(ext, ".au") == 0) return "audio/basic";
    if (strcmp(ext, ".wav") == 0) return "audio/wav";
    if (strcmp(ext, ".avi") == 0) return "video/x-msvideo";
    if (strcmp(ext, ".mpeg") == 0 || strcmp(ext, ".mpg") == 0) return "video/mpeg";
    if (strcmp(ext, ".mp3") == 0) return "audio/mpeg";
    
    return NULL;
}


/* if there is error on server side after we established a connection then return Internal Server Error */
int server_error(int fd, request_t* request)
{
    /* get current time */
    get_timebuff(request, TIME_NOW, fd);
    
    /* get size of interal error response, insert error content to write buff */
    int size = response_size(request, INTERNAL_ERROR, fd);
    request->write_buff = (char*)malloc(sizeof(char)*(size));
    if(request->write_buff == NULL)
    {
        printf("error on allocating memory\r\n");
        free_struct(request);
        close(fd);
        return FAILED;
    }
    bzero(request->write_buff, size);
    sprintf(request->write_buff, "HTTP/1.0 %s\r\nServer: webserver/1.0\r\nDate: %s\r\nContent-Type: %s\r\nContent-Length: %d\r\nConnection: close\r\n\r\n", "500 Internal Server Error", request->time_now, "text/html", 144);
    sprintf(request->write_buff + strlen(request->write_buff), "<HTML><HEAD><TITLE>%s</TITLE></HEAD>\r\n<BODY><H4>%s</H4>\r\n%s\r\n</BODY></HTML>", "500 Internal Server Error", "500 Internal Server Error", "Some server side error.");

    /* write error response to client */
    int nbytes;
    nbytes = write(fd, request->write_buff, strlen(request->write_buff));
    if(nbytes < 0)
    {
        perror("write");
        free_struct(request);
        close(fd);
        return FAILED;
    }

    free_struct(request);
    close(fd);
    return SUCCESS;
}


/* check if each folder in path has other execute permissions, if it is then return 0, else 1 */
int check_permissions(char *path, request_t* request, int fd) 
{
    struct stat fileStat;
    char* local_path = (char*)malloc(sizeof(char)*(strlen(path) + 1));
    if(local_path == NULL)
    {
        server_error(fd, request);
        return FAILED;
    }
    strcpy(local_path, path);
    char* ptr = strtok(local_path, "/");
    if(!ptr)
    {
        free(local_path);
        return NOT_FOUND;
    }
    char* curr;
    while (1) 
    {
        /* for checking permissions */
        if(stat(ptr, &fileStat) < 0)
        {
            server_error(fd, request);
            free(ptr);
            return FAILED;
        }
        
        /* check if it is a directory and has execute permissions */
        if (S_ISDIR(fileStat.st_mode) && !(fileStat.st_mode & S_IXOTH)) 
        {
            free(ptr);
            return FAILED;
        }
        curr = strtok(NULL, "/");
        if(!curr)
            break;
        
        sprintf(ptr + strlen(ptr), "/%s", curr);
    }
    free(ptr);
    return SUCCESS;
}


/* get current time and modified time into request struct */
int get_timebuff(request_t* request, int flag, int fd)
{
    /* get current time */
    if(flag == TIME_NOW)
    {
        time_t now;
        char timebuf[128];
        now = time(NULL);
        strftime(timebuf, sizeof(timebuf), RFC1123FMT, gmtime(&now));

        request->time_now = (char*)malloc(sizeof(char)*(strlen(timebuf) + 1));
        if(request->time_now == NULL)
        {
            server_error(fd, request);
            return FAILED;
        }
        bzero(request->time_now, sizeof(char)*(strlen(timebuf) + 1));
        strcpy(request->time_now, timebuf);
    }
    
    /* get last modified time */
    else if(flag == TIME_MOD)
    {
        struct stat fileStat;
        if(stat(request->path, &fileStat) < 0)
        {
            server_error(fd, request);
            return FAILED;
        }
        
        char last_modified[128];
        strftime(last_modified, sizeof(last_modified), RFC1123FMT, gmtime(&fileStat.st_mtime));

        request->time_mod = (char*)malloc(sizeof(char)*(strlen(last_modified) + 1));
        if(request->time_mod == NULL)
        {
            server_error(fd, request);
            return FAILED;
        }
        bzero(request->time_mod, sizeof(char)*(strlen(last_modified) + 1));
        strcpy(request->time_mod, last_modified);
    }
    return SUCCESS;
}


/* get response size of each response type for client */
int response_size(request_t* request, int flag, int fd)
{
    int check = -1;
    if(!request->time_now)
        check = get_timebuff(request, TIME_NOW, fd);
    
    if(check == FAILED)
        return FAILED;

    int size = 0;
    size += strlen("HTTP/1.0 ") /* status phrase */ + strlen("\r\n");
    size += strlen("Server: webserver/1.0") + strlen("\r\n");
    size += strlen("Date: ") + /* current date */ strlen(request->time_now) + strlen("\r\n");
    size += strlen("Content-Length: ") + /* length of response */ strlen("\r\n");
    size += strlen("Connection: close") + strlen("\r\n") + strlen("\r\n");

    if(flag == OK)
    {
        if(!request->time_mod)
            check = get_timebuff(request, TIME_MOD, fd);
        
        if(check == FAILED)
            return FAILED;

        struct stat fileStat;
        if(stat(request->path, &fileStat) < 0)
        {
            server_error(fd, request);
            return FAILED;
        }

        size += strlen("200 OK");
        if(request->mime)
            size += strlen("Content-Type: ") + strlen(request->mime) + strlen("\r\n");

        size += (int)fileStat.st_size;
        size += strlen("Last-Modified: ") + strlen(request->time_mod) + strlen("\r\n");

        return size + 150;
    }

    size += strlen("<HTML><HEAD><TITLE>") + /* title */ strlen("</TITLE></HEAD>") + strlen("\r\n");
    size += strlen("<BODY><H4>") + /* title */ strlen("</H4>") + strlen("\r\n") + strlen("\r\n");
    size += strlen("</BODY></HTML>");

    switch(flag)
    {
        case FOUND:
            size += strlen("302 Found");
            size += strlen("Content-Type: text/html") + strlen("\r\n");
            size += strlen("123");
            size += strlen("Location: ") + strlen(request->path) + strlen("/") + strlen("\r\n");
            size += strlen("302 Found");
            size += strlen("302 Found");
            size += strlen("Directories must end with a slash.");
            break;

        case BAD_REQUEST:
            size += strlen("400 Bad Request");
            size += strlen("Content-Type: text/html") + strlen("\r\n");
            size += strlen("113");
            size += strlen("400 Bad Request");
            size += strlen("400 Bad Request");
            size += strlen("Bad Request.");
            break;

        case FORBIDDEN:
            size += strlen("403 Forbidden");
            size += strlen("Content-Type: text/html") + strlen("\r\n");
            size += strlen("111");
            size += strlen("403 Forbidden");
            size += strlen("403 Forbidden");
            size += strlen("Access denied.");
            break;

        case NOT_FOUND:
            size += strlen("404 Not Found");
            size += strlen("Content-Type: text/html") + strlen("\r\n");
            size += strlen("112");
            size += strlen("404 Not Found");
            size += strlen("404 Not Found");
            size += strlen("File not found.");
            break;

        case INTERNAL_ERROR:
            size += strlen("500 Internal Server Error");
            size += strlen("Content-Type: text/html") + strlen("\r\n");
            size += strlen("144");
            size += strlen("500 Internal Server Error");
            size += strlen("500 Internal Server Error");
            size += strlen("Some server side error.");
            break;

        case NOT_SUPPORTED:
            size += strlen("501 Not supported");
            size += strlen("Content-Type: text/html") + strlen("\r\n");
            size += strlen("129");
            size += strlen("501 Not supported");
            size += strlen("501 Not supported");
            size += strlen("Method is not supported.");
            break;
    }

    return size + 150;
}


/* count the number of digits */
int count_digits(int num)
{
    int digits = 0;
    while(num != 0)
    {
        num /= 10;
        digits++;
    }
    return digits;
}


/* checks if a certain string is a number. if it is a number then return 0, else -1 */
int is_number(char* num)
{
    int i;
    for(i = 0; num[i] != '\0'; i++)
    {
        if(num[i] >= '0' && num[i] <= '9')
            continue;
        
        else
            return FAILED;
    }
    return SUCCESS;
}


/* free response struct for client after sending the response */
void free_struct(request_t* request)
{
    if(request->mime)
        free(request->mime);

    if(request->path)
        free(request->path);

    if(request->read_buff)
        free(request->read_buff);

    if(request->write_buff)
        free(request->write_buff);

    if(request->time_now)
        free(request->time_now);

    if(request->time_mod)    
        free(request->time_mod);

    free(request);
}
