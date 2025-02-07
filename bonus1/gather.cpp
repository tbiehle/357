#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
using namespace std;

int *c;
int *s;

void gather(int id)
{
    c[id] = 1;
    while (!(c[0] && c[1] && c[2])) {}

    if (s[id] == 0) s[id] = 1;
    while (!(s[0] && s[1] && s[2])) {}

    c[id] = 0;
    while (c[0] || c[1] || c[2]) {}
    
    s[id] = 0;
}

void work(int id)
{
    gather(id);
    cout << id << endl;
    gather(id);
    cout << id << endl;
    gather(id);
    cout << id << endl;
    gather(id);
    cout << id << endl;
    return;
}

int main()
{
    c = (int *)mmap(0, sizeof(int) * 3, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    s = (int *)mmap(0, sizeof(int) * 3, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    (*s) = 0;
    
    if ( fork() == 0)
    {
        int id = 1;
        work(id);
        return 0;
    }
    if (fork() == 0)
    {
        int id = 2;
        work(id);
        return 0;
    }
    int id = 0;

    work(id);
    wait(0);
    return 0;
}
