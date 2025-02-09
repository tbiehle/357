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

int main()
{
    cout << "linking..." << endl;
    int fd = shm_open("lab5mem", O_RDWR, 0777);
    if (!fd)
    {
        cout << "link failed!" << endl;
        return -1;
    }
    cout << "link successful!" << endl;

    char *p = (char *)mmap(NULL, 100*sizeof(char), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    while (p[0] == 0) {}
    cout << p;

    close(fd);
    shm_unlink("lab5mem");
    munmap(p, 100*sizeof(char));
    return 0;
}