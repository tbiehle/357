#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <iostream>

using namespace std;

int main(int argc, char* argv[]) {
    char *ex_args[4]; // p contains args passed to execv

    // allocate memory for each argument
    for (int i = 0; i < 4; i++) { 
        ex_args[i] = (char *)malloc(100 * sizeof(char));
    }
    ex_args[3] = NULL;

    // copy executable name to first execv arg
    char text[100];
    strcpy(text, "./");
    strcat(text, argv[1]);
    strcpy(ex_args[0], text); 

    int par_count = atoi(argv[2]);
    sprintf(ex_args[2], "%d", par_count); // copy par_count to third execv arg

    pid_t child_pid, wpid; // for forking purposes
    int status = 0;

    for (int i = 0; i < par_count; i++) {
        // copy process no. to second execv arg
        sprintf(ex_args[1], "%d", i); 
        int x = 1;
        child_pid = fork();
        if (child_pid == 0) {
            execv(ex_args[0], ex_args);
            return 0;
        }
    }

    while ((wpid = wait(&status)) > 0); // collect child process

    for (int i = 0; i < 4; i++) {
        free(ex_args[i]);
    }

    return 0;
}