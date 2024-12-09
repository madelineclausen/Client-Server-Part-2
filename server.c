#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include "Md5.c"

int new_socket;

typedef struct file_info 
{
    FILE *p;
    char* name;
    char mode;
    long unsigned int client;
} file_info;

struct file_info files[4];
char* empty = "empty";

int check_array(char* file, char mode)
{
    for (int x=0; x<4; x++)
    {
        if (strcmp(files[x].name, file) == 0 && files[x].client == pthread_self())
        {
            return 0;
        }
    }
    return 1;
}

void initialize_struct_array()
{
    file_info new_file = {0, "none", 'n', 0}; 
    for (int x=0; x<4; x++)
    {
        files[x] = new_file;
    }
}

void open_read_helper(int socket)
{
    char* file = strtok(NULL, " \t\n");
    if (check_array(file, 'r') == 0)
    {
        char* message = "A file is already open for reading";
        int check = send(socket, message, strlen(message), 0);
    }
    else
    {
        file_info new_file; 
        new_file.p = fopen(file,"r");
        new_file.name = file;
        new_file.mode = 'r';
        new_file.client = pthread_self();
        for (int a=0; a<4; a++)
        {
            if (strcmp(files[a].name, "none") == 0)
            {
                files[a] = new_file;
                break;
            }
        }
        int check = send(socket, empty, strlen(empty), 0);
    }
}

void open_append_helper(int socket)
{
    char* file = strtok(NULL, " \t\n");
    for (int x=0; x<4; x++)
    {
        if (strcmp(files[x].name, file) == 0 && files[x].client != pthread_self())
        {
            char* message = "The file is open by another client.";
            int check = send(socket, message, strlen(message), 0);
        }
    }
    if (check_array(file, 'a') == 0)
    {
        char* message = "A file is already open for appending";
        int check = send(socket, message, strlen(message), 0);
    }
    else
    {
        file_info new_file; 
        new_file.p = fopen(file,"a");
        new_file.name = file;
        new_file.mode = 'a';
        new_file.client = pthread_self();
        for (int a=0; a<4; a++)
        {
            if (strcmp(files[a].name, "none") == 0)
            {
                files[a] = new_file;
                break;
            }
        }
        int check = send(socket, empty, strlen(empty), 0);
    }
}

void get_hash_helper(int socket) 
{
    char* file = strtok(NULL, " \t\n");
    if (check_array(file, 'a') == 0)
    {
        char* message = "A file is already open for appending";
        int check = send(socket, message, strlen(message), 0);
    }
    else
    {
        unsigned char digest[16];
        MDFile(file, digest); // PROBLEM HERE: DIGEST IS NOT LETTERS
        int check = send(socket, digest, strlen(digest), 0);
    }
}

void close_helper(int socket)
{
    char* file_name = strtok(NULL, " \t\n");
    for (int q=0; q<4; q++)
    {
        if (files[q].client == pthread_self())
        {
            files[q].name = "none";
            files[q].mode = 'n';
            files[q].client = 0;
            fclose(files[q].p);
            files[q].p = 0;
        }
    }
    int check = send(socket, empty, strlen(empty), 0);
}

void append_helper(int socket)
{
    bool append_allowed = false;
    for (int x=0; x<4; x++)
    {
        if (files[x].mode == 'a')
        {
            append_allowed = true;
        }
    }
    if (!append_allowed)
    {
        char* message = "File not open";
        int check = send(socket, message, strlen(message), 0);
    }
    else
    {
        char* to_append = strtok(NULL, " \t\n");
        for (int i=0; i<4; i++)
        {
            if (strcmp(files[i].name, "none") != 0 && files[i].mode == 'a' && files[i].client == pthread_self())
            {
                fputs(to_append, files[i].p);
            }
        }
        int check = send(socket, empty, strlen(empty), 0);
    }
}

void read_helper(int socket)
{
    bool read_allowed = false;
    for (int x=0; x<4; x++)
    {
        if (files[x].mode == 'r')
        {
            read_allowed = true;
        }
    }
    if (!read_allowed)
    {
        char* message = "File not open";
        int check = send(socket, message, strlen(message), 0);
    }
    else
    {
        char* bytes = strtok(NULL, " \t\n");
        int num_bytes = atoi(bytes);
        char read_buffer[201];
        for (int i=0; i<4; i++)
        {
            if (files[i].name != NULL && files[i].mode == 'r')
            {
                for (int y=0; y<num_bytes; y++)
                {
                    char c = fgetc(files[i].p);
                    read_buffer[y] = c;
                }
                read_buffer[num_bytes] = 0;
            }
        }
        int check = send(socket, read_buffer, strlen(read_buffer), 0); 
    }
}

void *thread(void *vargp)
{
    int connfd = *((int *)vargp);
    while(true)
    {
        char buffer[1024] = {0};
        char *buffer_p = buffer;
        int valread = read(connfd, buffer_p, 1024);
        printf("%s\n", buffer);
        fflush(stdout);
        char* command = strtok(buffer, " \t\n");
        if(strcmp("openRead", command) == 0)
        {
            open_read_helper(connfd);
        }
        else if(strcmp("openAppend", command) == 0)
        {
            open_append_helper(connfd);
        }
        else if(strcmp("read", command) == 0)
        {
            read_helper(connfd);           
        }
        else if(strcmp("append", command) == 0)
        {
            append_helper(connfd);
        }
        else if(strcmp("close", command) == 0)
        {
            close_helper(connfd);
        }
        else if(strcmp("getHash", command) == 0)
        {
            get_hash_helper(connfd);
        }
    }
    pthread_detach(pthread_self()); 
    free(vargp);                    
    close(connfd);
}

int main(int argc, char const* argv[])
{
    int server_fd, new_command, listenfd, *connfdp;
    int port = atoi(argv[1]);
    int opt = 1;
    int c = 0;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(server_fd, SOL_SOCKET,SO_REUSEADDR | SO_REUSEPORT, &opt,sizeof(opt))) 
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    if (bind(server_fd, (struct sockaddr*)&address,sizeof(address))< 0) 
    {
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) 
    {
        exit(EXIT_FAILURE);
    }
    /*if ((new_socket= accept(server_fd, (struct sockaddr*)&address,(socklen_t*)&addrlen))< 0) 
    {
        exit(EXIT_FAILURE);
    }*/
    printf("server started\n");
    fflush(stdout);
    initialize_struct_array();
    while(1)
    {
        clientlen=sizeof(struct sockaddr_storage);
        connfdp = malloc(sizeof(int)); 
        *connfdp = accept(server_fd, (struct sockaddr*)&clientaddr, &clientlen); 
        pthread_create(&tid, NULL, thread, connfdp);
        //pthread_join(tid, NULL);
        c++;
    }
    return 0;
}