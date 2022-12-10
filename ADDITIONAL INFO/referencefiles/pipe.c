#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

int main()
{   
    int parentpipe[2];
    int childpipe[2];
    if (pipe(parentpipe) == -1) {
        perror("Parent to child pipe not created");
        exit(0);
    }

    if (pipe(childpipe) == -1) {
        perror("Child to parent pipe not created");
        exit(0);
    }



    int forked_pid = fork();
    if (forked_pid == 0)
    {   
        //closing pipes
        close(childpipe[0]);          //child closes unused read end of child pipe
        close(parentpipe[1]); //child closes unused write end of parent pipe

        //printing pid
        int child_pid = getpid();
        printf("CHILD: My pid is %d \n", child_pid);

        //reading input from parent pipe
        int child_output;
        if(read(parentpipe[0], &child_output, sizeof(child_output))==-1) //read blocks till it get input, so it waits for parent to write into pipe
        {
            perror("Error in reading from parent pipe!");
            exit(0);
        }

        printf("CHILD: Child pid is %d and number received from parent is %d \n", child_pid, child_output);
        close(parentpipe[0]);//close read end of parentpipe


        //taking input from command line
        int child_input;
        printf("CHILD: Child pid is %d.Enter input number which child sends to parent \n", child_pid);
        scanf("%d",&child_input);

        //writing child_input into child pipe
        if(write(childpipe[1], &child_input, sizeof(child_input))==-1)
        {
                perror("Error in writing from child to parent!");
                exit(0);
        }
        close(childpipe[1]);//close write end of childpipe

        
        exit(0);
    }
    else if (forked_pid==-1)
    {
        perror("Error in making child!");
        return 0;
    }
    else
    {   
        close(parentpipe[0]);          //parent closes unused read end of parent pipe
        close(childpipe[1]); //parent closes unused write end of child pipe

        int parent_pid = getpid();//pid of parent
        printf("PARENT: My pid is %d \n", parent_pid);
        
        //waits for child to print pid
        unsigned int microsecond = 1000000;
        usleep(1 * microsecond);

        //taking input from command line
        int parent_input;
        printf("PARENT: Parent pid is %d. Enter input number which parent sends to child \n", parent_pid);
        scanf("%d",&parent_input);

        //writing parent_input into parent pipe
        if(write(parentpipe[1], &parent_input, sizeof(parent_input))==-1)
        {   
            perror("Error in writing from parent to child!");
            exit(0);
        }
        close(parentpipe[1]);//close write end of parentpipe

        //reading input from child pipe
        int parent_output;
        if(read(childpipe[0], &parent_output, sizeof(parent_output))==-1)//read blocks till it get input from childpipe
        {
            perror("Error in reading from childpipe!");
            exit(0);
        }

        printf("PARENT: Parent pid is %d and number received from child is %d \n", parent_pid, parent_output);
        close(childpipe[0]);//close read end of childpipe


        wait(NULL);

        return(0);
    }

    return 0;
}