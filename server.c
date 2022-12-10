#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <netdb.h>
#include <ctype.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/stat.h>
#include <netinet/tcp.h>
#define buffer_size 1024
pthread_t threads[5]; //no of threads to service client
int clients[5];       //how many clients connected rn

typedef struct //struct to pass on client info to thread
{
    int new_sock;
    int id;
    int port;
    struct in_addr addrs;
} client;
char const *sperm(__mode_t mode) //reference: https://stackoverflow.com/questions/9638714/xcode-c-missing-sperm
{
    static char local_buff[16] = {0};
    int i = 0;
    // user permissions
    if ((mode & S_IRUSR) == S_IRUSR)
        local_buff[i] = 'r';
    else
        local_buff[i] = '-';
    i++;
    if ((mode & S_IWUSR) == S_IWUSR)
        local_buff[i] = 'w';
    else
        local_buff[i] = '-';
    i++;
    if ((mode & S_IXUSR) == S_IXUSR)
        local_buff[i] = 'x';
    else
        local_buff[i] = '-';
    i++;
    // group permissions
    if ((mode & S_IRGRP) == S_IRGRP)
        local_buff[i] = 'r';
    else
        local_buff[i] = '-';
    i++;
    if ((mode & S_IWGRP) == S_IWGRP)
        local_buff[i] = 'w';
    else
        local_buff[i] = '-';
    i++;
    if ((mode & S_IXGRP) == S_IXGRP)
        local_buff[i] = 'x';
    else
        local_buff[i] = '-';
    i++;
    // other permissions
    if ((mode & S_IROTH) == S_IROTH)
        local_buff[i] = 'r';
    else
        local_buff[i] = '-';
    i++;
    if ((mode & S_IWOTH) == S_IWOTH)
        local_buff[i] = 'w';
    else
        local_buff[i] = '-';
    i++;
    if ((mode & S_IXOTH) == S_IXOTH)
        local_buff[i] = 'x';
    else
        local_buff[i] = '-';
    return local_buff;
}
void send_file_from_server(int new_sock, FILE *fp)
{
    char buffer1[buffer_size];              //for sending the file
    memset(buffer1, '\0', sizeof(buffer1)); //reset buffer
    int bytes;
    int total_bytes=0;                              //bytes sent: we send the buffer_size characters each time
    bytes = fread(buffer1, sizeof(char), sizeof(buffer1), fp);
    total_bytes+=bytes;
    //printf("S:");

    while (bytes > 0)
    {

        write(new_sock, buffer1, bytes); //send over the buffer contents
        total_bytes+=bytes;
        //printf("BYTES WRITTEN %d \n", bytes);
        //printf("BUFFER WRITTEN %s \n", buffer1);
        memset(buffer1, '\0', sizeof(buffer1)); //reset buffer
        bytes = fread(buffer1, sizeof(char), sizeof(buffer1), fp);
    }
    printf("\nS:File sent! \n");
    fflush(0);
    memset(buffer1, '\0', sizeof(buffer1));
    //printf("Size of File sent %d \n", total_bytes);
    if(write(new_sock, "file sent!\0", sizeof("file sent!\0"))==-1) //sizeof("file sent!\0") is 12 bytes
    {
        printf("Something wrong here!");
    }
}
void *clientfunction(void *args)
{
    client *tempstruct = malloc(sizeof(tempstruct)); //copying over client info
    tempstruct = args;
    int new_sock = tempstruct->new_sock;
    int id = tempstruct->id;
    int port = tempstruct->port;
    struct in_addr addrs = tempstruct->addrs;
    char client_msg[1024];
    while (1)
    {

        memset(client_msg, '\0', sizeof(client_msg));
        int bytes_read = read(new_sock, client_msg, 1024);
        if (bytes_read == -1) //getting message from client
        {
            perror("No message sent from client!\n");
            exit(0);
        }

        if (strcmp(client_msg, "quit") == 0 | bytes_read == 0) //if client wants to exit
        {
            printf("S:Closing connection for client %d \n", id + 1);
            fflush(0);
            clients[id] = -1;
            close(new_sock);
            pthread_exit(NULL);
            break;
        }
        else if (bytes_read > 0)
        {
            pid_t tid = syscall(SYS_gettid);
            printf("S:Client %d with IP %s and port %d serviced by thread id %d asks for filename :%s \n", id + 1, inet_ntoa(addrs), port, tid, client_msg);
            FILE *fp;
            fp = fopen(client_msg, "r");
            if (fp == NULL)
            {
                printf("S:File does not exist here! Closing client connection..");
                //temporarily close file
                fflush(0);
                clients[id] = -1;
                close(new_sock);
                pthread_exit(NULL);
                break;
            }
            else if (fp != NULL)
            {
                struct stat fs;
                int r;
                printf("S:Obtaining permission mode for '%s':\n", client_msg);
                r = stat(client_msg, &fs);

                printf("S:Permissions are %s \n", sperm(fs.st_mode));
                printf("S:Permissions in octal format (%3o) \n", fs.st_mode & 0777);
                send_file_from_server(new_sock, fp);
                /*printf("\nS:File sent! \n");
                fflush(0);
                write(new_sock, "File sent!\0", buffer_size);*/

                memset(client_msg, '\0', sizeof(client_msg));
                int bytes_read = read(new_sock, client_msg, 1024);

                if (strcmp(client_msg, "Permissions?") == 0) //if client wants permissions

                {
                    write(new_sock, sperm(fs.st_mode), 1024);
                }
                printf("S:Permissions %s sent!\n",sperm(fs.st_mode) );

                //write(new_sock, fs.st_mode ,buffer_size);
            }
            fclose(fp);
            fflush(0);
        }
    }
}

int main(int argc, char const *argv[])
{
    int port;
    //inputting port
    printf("Enter port number to connect to. \n");
    scanf("%d", &port);

    int server_fd, new_sock;
    struct sockaddr_in address;
    //this is address to bind socket to

    //creating socket
    //server_fd contains the file descriptor of the socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        perror("Webserver not created successfully");
        return 1;
    }
    //socket created successfully
    int flag = 1;
    int result = setsockopt(server_fd,          /* socket affected */
                            IPPROTO_TCP,   /* set option at TCP level */
                            TCP_NODELAY,   /* name of option */
                            (char *)&flag, /* the cast is historical cruft */
                            sizeof(int));  /* length of option value */

    char hostbuffer[256];
    char *IPbuffer;

    struct ifreq ifr;
    ifr.ifr_addr.sa_family = AF_INET;

    char interfacename[256];
    printf("Please enter interface to connect to! \n");
    scanf("%s", interfacename);
    strncpy(ifr.ifr_name, interfacename, IFNAMSIZ - 1);

    ioctl(server_fd, SIOCGIFADDR, &ifr);
    int lo_flag = 0;

    if (strcmp(inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr), "0.0.0.0") == 0) //if the entered interface name is not connected to internet we use loopback
    {
        strncpy(ifr.ifr_name, "lo", IFNAMSIZ - 1);
        ioctl(server_fd, SIOCGIFADDR, &ifr);
        lo_flag = 1;
    }

    //this variable stores the size of address
    int addrlen = sizeof(address);

    //Address struct gets info added to it
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    //address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_addr.s_addr = inet_addr(inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr)); //got this from ifconfig
    //sin_family is always set to AF_INET, sin_port contains the port in network byte order.

    //Binding the socket to the address
    if (bind(server_fd, (struct sockaddr *)&address, addrlen) != 0)
    {
        perror("No successful binding!"); //if it didnt bind successfully
        return 1;
    }

    if (lo_flag == 0)
    {
        printf("IP address for %s %s\n", interfacename, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
    }
    else
    {
        printf("IP address for lo %s\n", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
    }

    printf("Port no: %d \n", port);

    //The listener for the socker listens for connections
    if (listen(server_fd, 1) != 0)
    {
        perror("Listener failure"); //in case of failure
        return 1;
    };

    for (int i = 0; i < 5; i++)
    {
        clients[i] = -1; //keeps track of clients
    }

    while (1)
    {
        int addrlen = sizeof(address);
        new_sock = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen); //Incoming connections are accepted via this statement

        //In case of error
        if (new_sock < 0)
        {
            perror("Failed connections!");
            return 1;
        }
        //connection has been established at this stage
        printf("S:Connection has been requested \n");
        fflush(0);
        int num_clients = 0; //counting no of clients connected to server
        for (int i = 0; i < 5; i++)
        {
            if (clients[i] != -1)
                num_clients++;
        }
        if (num_clients >= 5) //more than 4 clients cannot be accepted
        {
            printf("S: rejected! \n");
            fflush(0);
            write(new_sock, "rejected!\0", 1024);
            close(new_sock);
            continue;
        }
        else
        {
            printf("S:We are accepting the client! \n"); //accept client
            fflush(0);
            write(new_sock, "Hi! You are accepted\0", 1024); //send acceptance to client
            for (int i = 0; i < 5; i++)
            {
                if (clients[i] == -1)
                {
                    client *tempstruct = malloc(sizeof(tempstruct)); //create a temp struct to send to thread with client info
                    tempstruct->id = i;
                    tempstruct->port = address.sin_port;
                    tempstruct->new_sock = new_sock;
                    tempstruct->addrs = address.sin_addr;
                    clients[i] = new_sock;                                                 //place to send messages
                    pthread_create(&threads[i], NULL, clientfunction, (void *)tempstruct); //thread created to service client
                    break;
                }
            }
        }
    }

    return 0;
}