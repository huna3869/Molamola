/*
   MolaMola Web server - Simple web server in C
   Written by huna3869
   Based on Abhijeet Rastogi's Simple web server demonstration code
   2017. 10. 8
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h> // For getopt
#include <netdb.h> // For addrinfo
#include <signal.h>
#include <fcntl.h>

#define CONNMAX 1000

char* ROOT;
int stat_client[CONNMAX];
int sockfd;

void start_server(char *);
void respond(int);

int main(int argc, char* argv[])
{
    struct sockaddr_in addr_client;
    socklen_t addrlen;
    char c;


    // Set defalut values as PATH = ~/ (current path) and PORT = 12345
    // I learned from google examples though :/
    char PORT[6];
    ROOT = getenv("PWD");
    strcpy(&ROOT[strlen(ROOT)],"/web");
    strcpy(PORT,"12345");

    if (argc > 2)
    {
        fprintf(stderr, "#ERROR_INVALID_ARGUMENT");
        exit(1);
    }
    else if (argc == 2)
    {
        strcpy(PORT, argv[1]);
    }
    else { } // do nothing


    /*
    // I liked my way but requirements want worse way :(
    // parsing command line arguments :: -p <port> -d <path>
    while((c = getopt(argc, argv, "p:d:")) != -1)
    {
        switch(c)
        {
            case 'd':
                // parsing path
                ROOT = malloc(strlen(optarg));
                strcpy(ROOT,optarg);
                break;
            case 'p':
                //parsing port
                strcpy(PORT, optarg);
                break;
            case '?':
                fprintf(stderr, "#ERROR_INVALID_ARGUMENT");
                exit(1);
            default:
                exit(1);
        }
    }

    */

    printf("Molamola : Server started - PORT [%s] / ROOT [%s]\n", PORT, ROOT);

    for(int i=0; i<CONNMAX; i++)
    {
        stat_client[i] = -1;
        // To check respond pipeline status
        // initalize all status flag to -1 (failed flag)
    }
    // Start server - open socket
    start_server(PORT);

    // stat_client enumerator
    int slot = 0;

    // ACCEPT connections
    while(1)
    {
        addrlen = sizeof(addr_client);
        stat_client[slot] = accept(sockfd, (struct sockaddr *) &addr_client, &addrlen);

        if(stat_client[slot] < 0)
        {
            perror("#ERROR_ACCEPT_FAILED");
        }
        // If accept success, fork process and respond
        else
        {
            if(fork() == 0)
            {
                respond(slot);
                exit(0);
            }
        }

        // Find empty pipe
        while(stat_client[slot] != -1)
        {
            slot = (slot+1)%CONNMAX;
        }
    }

    return 0;
}

// Start server - open socket
void start_server(char* port)
{
    struct addrinfo hints, *res, *p;
    
    // Get host address information
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // IPV4
    hints.ai_socktype = SOCK_STREAM; // Stream socket
    hints.ai_flags = AI_PASSIVE;

    if(getaddrinfo(NULL, port, &hints, &res) != 0)
    {
        perror("#ERROR_GETADDRINFO_FAILED");
        exit(1);
    }

    // Socket and Bind
    // ai_next(linked list) provides flexivity to multiple addresses 
    for(p=res; p!=NULL; p=p->ai_next)
    {
        sockfd = socket(p->ai_family, p->ai_socktype, 0);
        if(sockfd == -1)
        {
            continue;
        }
        if(bind(sockfd, p->ai_addr, p->ai_addrlen) == 0)
        {
            break;
        }
    }
    if(p==NULL)
    {
        perror("#ERROR_SOCKET_OR_BIND_ERROR");
        exit(1);
    }
    
    freeaddrinfo(res);
    
    // Listen for incoming connection
    if(listen(sockfd, 1000000) != 0)
    {
        perror("#ERROR_LISTEN_FAILED");
        exit(1);
    }
}

// Client connection
void respond(int n)
{
    char msg[99999], *reqline[3], d2s[1024], path[99999];
    int rcv, fd, bytes_read;

    // Initialize msg string with null string
    memset((void*)msg, (int)'\0', 99999);

    rcv = recv(stat_client[n], msg, 99999, 0);

    // Receive error
    if(rcv<0)
    {
        fprintf(stderr, "#ERROR_RECV_FAILED");
    }
    // Receive unexpected socket close
    else if(rcv==0)
    {
        fprintf(stderr, "#CLIENT_DISCONNECTED_UNEXPECTEDLY");
    }
    // Message received
    else
    {
        // Parse request
        reqline[0] = strtok(msg, " \t\n");
        if(strncmp(reqline[0], "GET\0", 4) == 0) // If request is get method
        {
            reqline[1] = strtok(NULL, " \t");
            reqline[2] = strtok(NULL, " \t\n");
            printf("%s %s %s\n",reqline[0],reqline[1],reqline[2]);

            char* reqiter = strtok(NULL, "\t\n");
            while(reqiter != NULL)
            {
                printf("%s\n",reqiter);
                reqiter = strtok(NULL, "\t\n");
            }
            if( strncmp(reqline[2], "HTTP/1.0", 8)!=0 && strncmp(reqline[2], "HTTP/1.1", 8)!=0) // If request is not following HTTP protocol correctly
            {
                write(stat_client[n], "HTTP/1.0 400 Bad Request\n", 25);
            }
            else
            {
                if(strncmp(reqline[1], "/\0", 2) == 0) // No file specified (index.html)
                {
                    reqline[1] = "/index.html";
                }
                strcpy(path, ROOT);
                strcpy(&path[strlen(ROOT)], reqline[1]);
                printf("Accessd into : %s\n", path);

                if((fd=open(path, O_RDONLY))!=-1) // File Found
                {
                    send(stat_client[n], "HTTP/1.0 200 OK\n\n", 17, 0);
                    while((bytes_read=read(fd, d2s, 1024))>0)
                    {
                        write(stat_client[n], d2s, bytes_read);
                    }
                }
                else
                {
                    write(stat_client[n], "HTTP/1.0 404 Not Found\n", 23); // File not found
                }

            }
        }
    }

    // Closing socket
    shutdown(stat_client[n], SHUT_RDWR);
    close(stat_client[n]);
    stat_client[n] = -1;
}
