#include <unistd.h>
#include <csignal>
#include <stdio.h>
#include <iostream>
#include <sys/mman.h>
#include <fcntl.h>

int fd[2];
int std_cpy = dup(STDIN_FILENO);

void intrfct(int i)
{
    dup2(fd[0], STDIN_FILENO);
}

int main()
{
    // create signal for activity, 0 if inactive, 1 if active
    int *act_sig = (int *)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *act_sig = 0;
    pipe(fd);

    int par_pid = getpid();
    signal(SIGUSR1, intrfct);

    char text[100];
    if (fork() == 0)
    { // in child
        while (true)
        {
            sleep(10);
            if (*act_sig == 0)
            {
                kill(par_pid, SIGUSR1); // send interrupt to parent process
                write(fd[1], "Inactivity detected!", 21);
                *act_sig = 2;
            }
            else
            {
                *act_sig = 0;
            }
        }
        return 0;
    }
    else
    { // in parent, loop of getting input
        close(fd[1]); // parent is in read mode
        while (true)
        {
            int b = read(STDIN_FILENO, text, 100);
            text[b - 1] = 0;

            if (*act_sig == 2) printf("%s\n", text);
            else printf("!%s!\n", text);
            *act_sig = 1;
            
            dup2(std_cpy, STDIN_FILENO);
        }
    }
    return 0;
}