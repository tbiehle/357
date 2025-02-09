#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>

using namespace std;

bool strquit(char *p)
{
    if (sizeof(p) < 5) return false;
    char quit[] = "quit\n";

    for (int i = 0; i < 6; i++)
    {
        if (p[i] != quit[i]) return false;
    }

    return true;
}

int main() // program 1 gets user input and creates shared memory
{
    int fd = shm_open("lab5mem", O_RDWR | O_CREAT, 0777);
    int inword[100];
    ftruncate(fd, 100*sizeof(char));
    char *p = (char *)mmap(NULL, 100*sizeof(char), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    while (true)
    {
        int n = read(STDIN_FILENO, p, 100 * sizeof(char));
        for (int i = n; i < 100; i++)
        {
            p[i] = 0;
        }
        if (strquit(p)) break;
    }  

    close(fd);
    shm_unlink("lab5mem");
    munmap(p, 100*sizeof(char));
    return 0;
}