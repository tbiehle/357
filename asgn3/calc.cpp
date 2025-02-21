#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <dirent.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

using namespace std;

int main(int argc, char* argv[]) {
    int par_id = atoi(argv[1]);
    int par_count = atoi(argv[2]);
    int *matA, *matB, *matM;
    int matA_fd, matB_fd, matM_fd;
    int mat_w = 2;
    int mat_size = mat_w * mat_w;

    if (par_id == 0) {
        // first process, create shared memory
        // matrix A
        matA_fd = shm_open("matA_mem", O_RDWR | O_CREAT, 0777);
        matB_fd = shm_open("matB_mem", O_RDWR | O_CREAT, 0777);
        matM_fd = shm_open("matM_mem", O_RDWR | O_CREAT, 0777);
        ftruncate(matA_fd, mat_size * sizeof(int));
        ftruncate(matB_fd, mat_size * sizeof(int));
        ftruncate(matM_fd, mat_size * sizeof(int));
        matA = (int *)mmap(NULL, mat_size * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, matA_fd, 0);
        matB = (int *)mmap(NULL, mat_size * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, matB_fd, 0);
        matM = (int *)mmap(NULL, mat_size * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, matM_fd, 0);
    } else {
        // wait for processes to successfully link
        while (!(matA_fd = shm_open("matA_mem", O_RDWR, 0777)));
        while (!(matB_fd = shm_open("matB_mem", O_RDWR, 0777)));
        while (!(matM_fd = shm_open("matM_mem", O_RDWR, 0777)));
        // link matrix pointers to shared memomry
        matA = (int *)mmap(NULL, mat_size * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, matA_fd, 0);
        matB = (int *)mmap(NULL, mat_size * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, matB_fd, 0);
        matM = (int *)mmap(NULL, mat_size * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, matM_fd, 0);
    }

    // par_id: what process this is
    // par_count: how many processes there are 

    // UNCOMMENT THIS TO MAKE TRUE RANDOM VALUES
    // srand(time(0)); // initialize random seed based on cpu time

    // fill matrices with random values
    for (int i = 0; i < mat_size; i++) {
        matA[i] = rand() % 3; // generate random number between 0 and 2
    }
    for (int i = 0; i < mat_size; i++) {
        matB[i] = rand() % 3; // generate random number between 0 and 2
    }
    for (int i = 0; i < mat_size; i++) {
        matM[i] = rand() % 3; // generate random number between 0 and 2
    }

    int par_rows = mat_w / par_count; // ex) if matrix is 2x2 and there are 2 processes, one row per process
    // TODO
    // M = A * B

    // TODO
    // B = M * A

    // TODO
    // M = B * A

    // TODO
    // det(M)

    // print(det(M))

    close(matA_fd);
    close(matB_fd);
    close(matM_fd);
    if (par_id == 0) {
        shm_unlink("matA_mem");
        shm_unlink("matB_mem");
        shm_unlink("matM_mem");
    }
    return 0;
}