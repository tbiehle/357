#include <stdlib.h>
#include <iostream>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <fstream>
#include <time.h>
#include <sys/mman.h>


typedef unsigned char BYTE;
using namespace std;

// global real width in bits for use in functions
int rwib;

struct bfh
{
    BYTE bfType[2];
	int bfSize;
	BYTE bfReserved1[2];
	BYTE bfReserved2[2];
	int bfOffBits;
};

struct bih
{
    int biSize;
    int biWidth;
    int biHeight;
    BYTE biPlanes[2];
    BYTE biBitCount[2];
    int biCompression;
    int biSizeImage;
    int biXPelsPerMeter;
    int biYPelsPerMeter;
    int biClrUsed;
    int biClrImportant;
};

class batch {
public:
    int start, end; // x coordinates
    BYTE val;
    int line; // y coordinates
};

void write_batch(BYTE *data, int batch_amt, batch* batches, int col_idx) {
    for (int i = 0; i < batch_amt; i++) {
        batch curr_batch = batches[i];
        int start = curr_batch.start;
        int end = curr_batch.end;
        int y = curr_batch.line;
        BYTE val = curr_batch.val;

        for (int x = curr_batch.start; x < curr_batch.end; x++) {
            data[y * rwib + x * 3 + col_idx] = val;
        }
    }
}

int main(int argc, char *argv[]) {
    timespec start, end;
    // start the clock
    clock_gettime(CLOCK_MONOTONIC, &start);

    // read input filename from command line and open it
    char compfilename[100];
    strcpy(compfilename, argv[1]);
    FILE *compfile = fopen(compfilename, "rb");
    if (!compfile) {
        cerr << "error opening input file" << endl;
        return -1;
    }

    // get and sterilize output filename from command line
    char outfilename[100];
    strcpy(outfilename, argv[2]);
    int len;
    for (int i = 0; i < 100; i++) {
        if (outfilename[i] == 0) {
            len = i;
            break;
        }
    }
    if (len < 5 || strcmp(outfilename + len - 4, ".bmp") != 0) {
        cerr << "invalid output filename" << endl;
        return -1;
    }

    bfh bfh;
    bih bih;

    // read header info of img
    fread(&bfh.bfType, 2, 1, compfile);
    fread(&bfh.bfSize, 4, 1, compfile);
    fread(&bfh.bfReserved1, 2, 1, compfile);
    fread(&bfh.bfReserved2, 2, 1, compfile);
    fread(&bfh.bfOffBits, 4, 1, compfile);

    fread(&bih.biSize, 4, 1, compfile);
    fread(&bih.biWidth, 4, 1, compfile);
    fread(&bih.biHeight, 4, 1, compfile);
    fread(&bih.biPlanes, 2, 1, compfile);
    fread(&bih.biBitCount, 2, 1, compfile);
    fread(&bih.biCompression, 4, 1, compfile);
    fread(&bih.biSizeImage, 4, 1, compfile);
    fread(&bih.biXPelsPerMeter, 4, 1, compfile);
    fread(&bih.biYPelsPerMeter, 4, 1, compfile);
    fread(&bih.biClrUsed, 4, 1, compfile);
    fread(&bih.biClrImportant, 4, 1, compfile);

    int *batch_amts = (int *)mmap(NULL, sizeof(int) * 3, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    // read batch amounts from compressed file
    int red_amts, green_amts, blue_amts;
    fread(&red_amts, sizeof(int), 1, compfile);
    fread(&green_amts, sizeof(int), 1, compfile);
    fread(&blue_amts, sizeof(int), 1, compfile);
    // store batch amounts in array
    batch_amts[2] = red_amts;
    batch_amts[1] = green_amts;
    batch_amts[0] = blue_amts;

    // read batches from compressed file
    batch *red = (batch *)mmap(NULL, sizeof(batch) * red_amts, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    batch *green = (batch *)mmap(NULL, sizeof(batch) * green_amts, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    batch *blue = (batch *)mmap(NULL, sizeof(batch) * blue_amts, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    fread(red, sizeof(batch), red_amts, compfile);

    fread(green, sizeof(batch), green_amts, compfile);
    fread(blue, sizeof(batch), blue_amts, compfile);
    // store batch arrays in array
    batch **batches = (batch **)mmap(NULL, sizeof(batch *) * 3, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    batches[2] = red;
    batches[1] = green;
    batches[0] = blue;

    // calculate rwib
    rwib = 3 * bih.biWidth + (4 - (3 * bih.biWidth) % 4) % 4;

    // allocate output data array
    BYTE *data = (BYTE *)mmap(NULL, bih.biSizeImage, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    // decompress batches and write to data array
    for (int i = 0; i < 3; i++) {
        if (fork() == 0) {
            write_batch(data, batch_amts[i], batches[i], i);
            return 0;
        }
    }
    wait(0);

    // open output file
    FILE *outfile = fopen(outfilename, "wb");
    if (!outfile) {
        cerr << "error opening input file" << endl;
        return -1;
    }

    // write .bmp header info
    fwrite(&bfh.bfType, 2, 1, outfile);
    fwrite(&bfh.bfSize, 4, 1, outfile);
    fwrite(&bfh.bfReserved1, 2, 1, outfile);
    fwrite(&bfh.bfReserved2, 2, 1, outfile);
    fwrite(&bfh.bfOffBits, 4, 1, outfile);

    fwrite(&bih.biSize, 4, 1, outfile);
    fwrite(&bih.biWidth, 4, 1, outfile);
    fwrite(&bih.biHeight, 4, 1, outfile);
    fwrite(&bih.biPlanes, 2, 1, outfile);
    fwrite(&bih.biBitCount, 2, 1, outfile);
    fwrite(&bih.biCompression, 4, 1, outfile);
    fwrite(&bih.biSizeImage, 4, 1, outfile);
    fwrite(&bih.biXPelsPerMeter, 4, 1, outfile);
    fwrite(&bih.biYPelsPerMeter, 4, 1, outfile);
    fwrite(&bih.biClrUsed, 4, 1, outfile);
    fwrite(&bih.biClrImportant, 4, 1, outfile);

    // write data
    fwrite(data, bih.biSizeImage, 1, outfile);

    // stop the clock and calculate elapsed time
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed_time;
    elapsed_time = end.tv_nsec - start.tv_nsec;
    elapsed_time /= 1e6; // this converts nanoseconds to milliseconds

    cout << "finished decompression" << endl;
    cout << "total time: " << elapsed_time << " milliseconds" << endl;

    munmap(batch_amts, sizeof(int) * 3);
    munmap(batches, sizeof(batch*) * 3);
    munmap(red, sizeof(batch) * red_amts);
    munmap(green, sizeof(batch) * green_amts);
    munmap(blue, sizeof(batch) * blue_amts);
    munmap(data, bih.biSizeImage);

    return 0;
}