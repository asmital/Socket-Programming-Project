
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>

#define buffer_size 1024
void write_from_client(int server_fd, FILE *fp)
{
    char buffer1[buffer_size];              //for received messages
    memset(buffer1, '\0', sizeof(buffer1)); //reset buffer
    int bytes;  
    int total_bytes=0;                            //bytes received
    bytes = read(server_fd, buffer1, sizeof(buffer1));
    total_bytes+=bytes;
    int flag = 1;
    while (flag == 1)
    {
        if (bytes == -1)
        {
            perror("Error in receiving bytes!");
            exit(0);
        }
        /*
        printf("BYTES WRITTEN %d \n", bytes);
        fflush(0);
        printf("CLIENT WRITES %s \n", buffer1);
        fflush(0);
        */

        //fwrite(buffer1, sizeof(char),sizeof(buffer1), fp); doesnt work for text
        //fwrite(buffer1, sizeof(char),strlen(buffer1), fp); doesnt work for binary
        fwrite(buffer1, sizeof(char), bytes, fp); //works for both text and binary files
        memset(buffer1, '\0', sizeof(buffer1));   //reset buffer
        total_bytes+=bytes;

        bytes = read(server_fd, buffer1, sizeof(buffer1));
        /*printf("BYTES TO WRITE %d \n", bytes);
        fflush(0);
        printf("SERVER SENDS %s\n", buffer1);
        fflush(0);*/
        char subbuff[12];
        memcpy(subbuff, &buffer1[bytes-12], 12);
        //printf("SUBBUF %s \n", subbuff);
        fflush(0);
        if (strcmp(buffer1, "file sent!") == 0 || bytes <= 0 || strcmp(subbuff, "file sent!") == 0)
        {

            //printf("Breaking here: \n");
            fflush(0);
            flag = 0;
            /*printf("Buffer1[bytes-13]: %c \n", buffer1[bytes-13]);
            fflush(0);
            printf("Buffer1[bytes-12]: %c \n", buffer1[bytes-12]);
            fflush(0);
            printf("Buffer1[bytes-11]: %c \n", buffer1[bytes-11]);
            fflush(0);*/
            if(strcmp(subbuff, "file sent!") == 0 && strcmp(buffer1, "file sent!") != 0 )
            {
                fwrite(buffer1, sizeof(char), bytes - 12, fp);
                total_bytes+=bytes-12;
            }
        }
        
    }
    //printf("Total bytes written %d \n", total_bytes);
}

int parse(char *perms) //https://stackoverflow.com/questions/37233964/how-to-change-permissions-of-a-file-in-linux-with-c
{
    int bits = 0;
    for (int i = 0; i < 9; i++)
    {
        if (perms[i] != '-')
        {
            bits |= 1 << (8 - i);
        }
    }
    return bits;
}
int main(int argc, char const *argv[])
{
    int port;
    //inputting port
    printf("Enter port number to connect to. \n");
    scanf("%d", &port);

    char IPaddr[buffer_size];

    printf("Enter IP address to connect to. \n");
    scanf("%s", IPaddr);

    int server_fd, new_sock;
    struct sockaddr_in address;
    //this is address to bind socket to

    //creating socket
    //server_fd contains the file descriptor of the socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        perror("Webclient not created successfully");
        return 1;
    }
    //client created successfully

    //this variable stores the size of address
    int addrlen = sizeof(address);

    //Address struct gets info added to it
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = inet_addr(IPaddr);
    //sin_family is always set to AF_INET, sin_port contains the port in network byte order.

    //Incoming connections are accepted via this statement
    if (connect(server_fd, (struct sockaddr *)&address, (socklen_t)sizeof(address)) == -1)
    {
        perror("Error in connection, unable to reach server");
        exit(0);
    }
    //connection has been established at this stage

    char buffer1[buffer_size];      //for received messages
    char buffer2[buffer_size];      //for sending messages
    char filename[buffer_size + 2]; //for sharing filename

    memset(buffer1, '\0', sizeof(buffer1)); //reset buffer
    if (read(server_fd, buffer1, sizeof(buffer1)) == -1)
    {
        perror("Error in receiving in client!");
        exit(0);
    }
    printf("S: %s\n", buffer1); //getting accept or reject message from server
    fflush(0);
    if (strcmp(buffer1, "rejected!") == 0)
    {
        //Closing connection since we have been rejected
        close(server_fd);

        return 0;
    }

    int flag = 1;
    fgetc(stdin);
    //if we reach here it means no error message

    do
    {
        //client sends message
        fflush(0);
        printf("Please enter filename! \n");
        memset(buffer2, '\0', sizeof(buffer2));
        
        printf("C: ");
        fgets(buffer2, sizeof(buffer2), stdin); //we take in filename message

        for (int i = 0; i < buffer_size; i++)
        {
            if (buffer2[i] == '\n')
                buffer2[i] = '\0'; //new lines to terminal characters
        }
        sprintf(filename, "./%s", buffer2);

        write(server_fd, buffer2, sizeof(buffer2)); //sending filename to server
        if (strcmp(buffer2, "quit") == 0)           //so the input is quit and we get an empty msg from server and end connection
        {
            flag = 0;
        }
        else
        {
            //opening file with the name we sent
            FILE *fp;
            fp = fopen(buffer2, "w");
            write_from_client(server_fd, fp);
            fflush(0);

            printf("C: File received!\n");
            memset(buffer2, '\0', sizeof(buffer2)); //reset buffer

            write(server_fd, "Permissions?\0", buffer_size);
            printf("C:Obtaining permissions: \n");

            memset(buffer1, '\0', sizeof(buffer1));
            read(server_fd, buffer1, sizeof(buffer1));
            printf("C:Permissions are %s \n", buffer1);

            printf("C: Permissions in Octal form %o\n", parse(buffer1));

            if (chmod(filename, parse(buffer1)) < 0)
            {
                perror("Error in chmod!");
                exit(1);
            }
            printf("C:Permissions updated!\n");
            fclose(fp);
        }
        memset(buffer1, '\0', sizeof(buffer1));

    } while (flag);

    //Closing connection
    close(server_fd);

    return 0;
}