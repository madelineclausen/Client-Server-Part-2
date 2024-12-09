// Madeline Clausen (clausenm)
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ctype.h>

int main(int argc, char *argv[])
{
    int sock = 0, valread, client_fd;
    int port = atoi(argv[2]);
    struct sockaddr_in serv_addr;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    {
        return -1;
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) < 0) 
    {
        return -1;
    }
    if ((client_fd= connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0)
    {
        return -1;
    }
    printf("> ");
    fflush(stdout);
    char input[80];
    fgets(input, 80, stdin);
    char* command = strtok(input, " \n\t");
    while(strcmp("quit", command) != 0)
    {
        char* file = strtok(NULL, " \n\t");
        char message[80] = {0};
        strcat(message, command);
        strcat(message, " ");
        strcat(message, file);
        send(sock, message, strlen(message), 0);
        char buffer[1024] = {0};
        char *buffer_p = buffer;
        int valread = read(sock, buffer_p, 1024);
        if ((isalpha(buffer[0]) || isdigit(buffer[0])) && strcmp(buffer_p, "empty") != 0)
        {
            printf("%s\n", buffer_p);
            fflush(stdout);
        }
        printf("> ");
        fflush(stdout);
        fgets(input, 80, stdin);
        char* command = strtok(input, " \n\t");
    }
    return 0;
}