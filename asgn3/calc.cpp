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

#define VALUE_RANGE 10;
#define MATRIX_WIDTH 100;

using namespace std;

void given_sync(int par_id, int par_count, int*ready) {
    int synchid = ready[par_count] + 1;
    ready[par_id] = synchid;
    int breakout = 0;
    while (1) {
        breakout = 1;
        for(int i=0; i < par_count; i++) {
            if (ready[i] < synchid) {
                breakout = 0;
                break;
            }
        }
        if (breakout == 1) {
            ready[par_count] = synchid;
            break;
        }
    }
}

void matrix_mult(int *matA, int *matB, int *mat_out, int start_row, int end_row, int mat_w) {
    for (int y = start_row; y < end_row; y++) { // y coordinate of result
        for (int x = 0; x < mat_w; x++) { // x coordinate of result
            int result = 0;
            int curr_idx = y * mat_w + x;

            for (int c = 0; c < mat_w; c++) { // iterator through all the cells 
                result += matA[y * mat_w + c] * matB[c * mat_w + x];
            }
            
            mat_out[curr_idx] = result;
        }
    }
}

long long diagonal_sum(int *mat, int mat_w) {
    long long sum = 0;
    for (int i = 0; i < mat_w; i++) {
        sum += mat[i + i * mat_w];
    }

    return sum;
}

void generate_matrix(int *mat, int mat_size) {
    

    for (int i = 0; i < mat_size; i++) {
        mat[i] = rand() % VALUE_RANGE; // generate random number between 0 and 2
    }
}

int main(int argc, char* argv[]) {
    int par_id = atoi(argv[1]);
    int par_count = atoi(argv[2]);
    // par_id: what process this is
    // par_count: how many processes there are 

    int *matA, *matB, *matM; // matrix pointers
    int matA_fd, matB_fd, matM_fd; // matrix file descriptors
    int *ready; // synch pointer

    int *row_tracker; 
    // remainder pointer for keeping track of extra work
    // index 0 is extra rows needed to be processed
    // index 1 is running total of how many rows have been processed, i.e. assigned to each process
    // so if the remainder is 4, the first four processes get an extra row

    int remainder_fd;
    int ready_fd;
    int mat_w = MATRIX_WIDTH;
    int mat_size = mat_w * mat_w;

    int par_rows = mat_w / par_count; // ex) if matrix is 2x2 and there are 2 processes, one row per process
    int processed_rows = par_rows * par_count;
    int remaining_rows = mat_w - processed_rows;
    
    if (par_id == 0) {
        // first process, create shared memory
        // matrix A
        matA_fd = shm_open("matA_mem", O_RDWR | O_CREAT, 0777);
        matB_fd = shm_open("matB_mem", O_RDWR | O_CREAT, 0777);
        matM_fd = shm_open("matM_mem", O_RDWR | O_CREAT, 0777);
        ready_fd = shm_open("ready_mem", O_RDWR | O_CREAT, 0777);
        remainder_fd = shm_open("remainder_mem", O_RDWR | O_CREAT, 0777);
        if (matA_fd == -1 || matB_fd == -1 || matM_fd == -1 || ready_fd == -1 || remainder_fd == -1) {
            perror("shm_open");
            exit(1);
        }
        ftruncate(matA_fd, mat_size * sizeof(int));
        ftruncate(matB_fd, mat_size * sizeof(int));
        ftruncate(matM_fd, mat_size * sizeof(int));
        ftruncate(ready_fd, (par_count + 1) * sizeof(int));
        ftruncate(remainder_fd, 2 * sizeof(int));
        matA = (int *)mmap(NULL, mat_size * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, matA_fd, 0);
        matB = (int *)mmap(NULL, mat_size * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, matB_fd, 0);
        matM = (int *)mmap(NULL, mat_size * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, matM_fd, 0);
        ready = (int *)mmap(NULL, (par_count + 1) * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, ready_fd, 0);
        row_tracker = (int *)mmap(NULL, 2 * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, remainder_fd, 0);

        for (int i = 0; i < par_count; i++) {ready[i] = 0;}
        ready[par_count] = 0;

        row_tracker[0] = remaining_rows;
        row_tracker[1] = 0;

        // COMMENT THIS OUT TO MAKE NON-RANDOM VALUES
        srand(time(0)); // initialize random seed based on cpu time

        generate_matrix(matA, mat_size);
        generate_matrix(matB, mat_size);
    } else {
        sleep(1);
        // wait for processes to successfully link
        while (!(matA_fd = shm_open("matA_mem", O_RDWR, 0777)));
        while (!(matB_fd = shm_open("matB_mem", O_RDWR, 0777)));
        while (!(matM_fd = shm_open("matM_mem", O_RDWR, 0777)));
        while (!(ready_fd = shm_open("ready_mem", O_RDWR, 0777)));
        while (!(remainder_fd = shm_open("remainder_mem", O_RDWR, 0777)));
        // link matrix pointers to shared memomry
        matA = (int *)mmap(NULL, mat_size * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, matA_fd, 0);
        matB = (int *)mmap(NULL, mat_size * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, matB_fd, 0);
        matM = (int *)mmap(NULL, mat_size * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, matM_fd, 0);
        ready = (int *)mmap(NULL, (par_count + 1) * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, ready_fd, 0);
        row_tracker = (int *)mmap(NULL, 2 * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, remainder_fd, 0);
        if (matA == MAP_FAILED || matB == MAP_FAILED || matM == MAP_FAILED || ready == MAP_FAILED || row_tracker == MAP_FAILED) {
            perror("mmap");
            exit(1);
        }
        //sleep(1);
    }

    int start_row = row_tracker[1];

    if (row_tracker[0] > 0) {
        par_rows += 1;
        row_tracker[0] -= 1;
    }
    row_tracker[1] += par_rows;

    int end_row = start_row + par_rows;

    timespec s_time, f_time;
    
    given_sync(par_id, par_count, ready);
    if (par_id == 0) {
        clock_gettime(CLOCK_MONOTONIC, &s_time);
    }

    // M = A * B
    matrix_mult(matA, matB, matM, start_row, end_row, mat_w);

    given_sync(par_id, par_count, ready);
    // B = M * A
    matrix_mult(matM, matA, matB, start_row, end_row, mat_w);

    given_sync(par_id, par_count, ready);
    // M = B * A
    matrix_mult(matB, matA, matM, start_row, end_row, mat_w);

    given_sync(par_id, par_count, ready);
    // diagonal_sum(M)
    long long diag_sum = diagonal_sum(matM, mat_w);

    given_sync(par_id, par_count, ready);

    if (par_id == 0) {
        clock_gettime(CLOCK_MONOTONIC, &f_time);
        long elapsed_time_ns = (f_time.tv_sec - s_time.tv_sec) * 1000000000L + (f_time.tv_nsec - s_time.tv_nsec);
        std::cout << "Diagonal sum: " << diag_sum << std::endl;
        std::cout << "Total time: " << elapsed_time_ns << " microseconds" << std::endl;
    }

    close(matA_fd);
    close(matB_fd);
    close(matM_fd);
    close(ready_fd);
    close(remainder_fd);
    if (par_id == 0) {
        shm_unlink("matA_mem");
        shm_unlink("matB_mem");
        shm_unlink("matM_mem");
        shm_unlink("ready_mem");
        shm_unlink("remainder_mem");
    }
    return 0;
}