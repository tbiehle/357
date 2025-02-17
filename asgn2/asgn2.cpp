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
#include <ctime>
#include <string.h>

using namespace std;

typedef unsigned char BYTE;

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

BYTE interp_color(BYTE **surr_pix, int col, float dx, float dy)
    {
    return (1 - dx)*(1 - dy)*surr_pix[0b00][col] + dx*(1-dy)*surr_pix[0b10][col] + (1-dx)*dy*surr_pix[0b01][col] + dx*dy* surr_pix[0b11][col];
    }

void get_neighbor_pix(BYTE *neighbors[4], BYTE *data, int x, int y, int w, int h, int rwib)
{
    // IMPORTANT
    // bottom left : 00 = 0
    // bottom right : 10 = 2
    // top left : 01 = 1
    // top right : 11 = 3
    
    neighbors[0b00] = &data[3 * x + y * rwib]; // bottom left pixel surrounding small pixel
    neighbors[0b10] = neighbors[0b00]; // initialize to bottom left
    neighbors[0b01] = neighbors[0b00]; // initialize to bottom left
    neighbors[0b11] = neighbors[0b00]; // initialize to bottom left

    // bottom right pixel surrounding small pixel
    if (x < w - 1)
    {
        neighbors[0b10] = &data[3 * (x + 1) + y * rwib]; 
    }
    
    // top left pixel 
    if (y < h - 1)
    {
        neighbors[0b01] = &data[3 * x + (y + 1) * rwib];
    }

    // top right pixel
    if (x < w - 1 && y < h - 1)
    {
        neighbors[0b11] = &data[3 * (x + 1) + (y + 1) * rwib];
    }
}

bool is_bmp(string name)
    {
    string bmp = ".bmp";

    int length = name.length();
    if (length <= 4) return false;

    if (name.compare(length-4, 4, bmp) == 0) return true;

    return false;
    }


int main(int argc, char *argv[])
    {
    bfh bfh1, bfh2;
    bih bih1, bih2;
    bool manual = false;

    char *file1 = argv[1];
    char *file2 = argv[2];
    char *outname = argv[5];
    
    string file1_str = file1;
    string file2_str = file2;
    string outname_str = outname;

    if (!is_bmp(file1_str) || !is_bmp(file2_str))
        {
        cout << "Invalid input file name: try [filename].bmp\n" << endl;
        manual = true;
        }
    if (!is_bmp(outname_str))
        {
        cout << "Invalid output file name: try [filename].bmp\n" << endl;
        manual = true;
        }

    float ratio = atof(argv[3]);
    if (ratio < 0 || ratio > 1)
        {
        cout << "Invalid interpolation ratio: try [0-1]\n" <<endl;
        manual = true;
        }

    int num_processes = atoi(argv[4]);
    if (num_processes < 1 || num_processes > 4)
        {
        cout << "Invalid number of processes: try an integer 1-4\n" << endl;
        manual = true;
        }

    FILE *img1 = fopen(file1, "rb");
    FILE *img2 = fopen(file2, "rb");
    FILE *outfile = fopen(outname, "wb"); // create output file 

    if (!img1 || !img2 || !outfile)
    {
        cout << "One or more files could not be opened, make sure the input files exist!\n" << endl;
        manual = true;
    }

    if (manual)
    {
        cout << "--------------------- User manual ---------------------" << endl;
        cout << "command line argument 1: input file 1 name - [name].bmp" << endl;
        cout << "command line argument 2: input file 2 name - [name].bmp" << endl;
        cout << "command line argument 3: interpolation ratio - [0-1]" << endl;
        cout << "command line argument 4: number of processes - [1-4] " << endl;
        cout << "command line argument 5: output file name - [name].bmp" << endl;
        return -1;
    }

    // read header info of img1
    fread(&bfh1.bfType, 2, 1, img1);
    fread(&bfh1.bfSize, 4, 1, img1);
    fread(&bfh1.bfReserved1, 2, 1, img1);
    fread(&bfh1.bfReserved2, 2, 1, img1);
    fread(&bfh1.bfOffBits, 4, 1, img1);
    fread(&bih1.biSize, 4, 1, img1);
    fread(&bih1.biWidth, 4, 1, img1);
    fread(&bih1.biHeight, 4, 1, img1);
    fread(&bih1.biPlanes, 2, 1, img1);
    fread(&bih1.biBitCount, 2, 1, img1);
    fread(&bih1.biCompression, 4, 1, img1);
    fread(&bih1.biSizeImage, 4, 1, img1);
    fread(&bih1.biXPelsPerMeter, 4, 1, img1);
    fread(&bih1.biYPelsPerMeter, 4, 1, img1);
    fread(&bih1.biClrUsed, 4, 1, img1);
    fread(&bih1.biClrImportant, 4, 1, img1);
    
    // read header info of img2
    fread(&bfh2.bfType, 2, 1, img2);
    fread(&bfh2.bfSize, 4, 1, img2);
    fread(&bfh2.bfReserved1, 2, 1, img2);
    fread(&bfh2.bfReserved2, 2, 1, img2);
    fread(&bfh2.bfOffBits, 4, 1, img2);
    fread(&bih2.biSize, 4, 1, img2);
    fread(&bih2.biWidth, 4, 1, img2);
    fread(&bih2.biHeight, 4, 1, img2);
    fread(&bih2.biPlanes, 2, 1, img2);
    fread(&bih2.biBitCount, 2, 1, img2);
    fread(&bih2.biCompression, 4, 1, img2);
    fread(&bih2.biSizeImage, 4, 1, img2);
    fread(&bih2.biXPelsPerMeter, 4, 1, img2);
    fread(&bih2.biYPelsPerMeter, 4, 1, img2);
    fread(&bih2.biClrUsed, 4, 1, img2);
    fread(&bih2.biClrImportant, 4, 1, img2);

    bih infoh, infohsmall; // infoh: bigger file info header 
    bfh fileh, filehsmall; // fileh: bigger file file header
    FILE *bfile, *sfile; // pointer to bigger and smaller images

    if (bih1.biWidth > bih2.biWidth) // img1 is bigger than img2, set proper info for outfile
        {
        bfile = img1;
        sfile = img2;
        infoh = bih1;
        fileh = bfh1;
        infohsmall = bih2;
        filehsmall = bfh2;
        ratio = 1 - ratio; 
        // need to flip ratio since the given ratio since ratio should compute how much of first image to display
        // e.g. if ratio is 0.3, want 30% img1 and 70% img2
        }
    else // img2 bigger than img1
        {
        bfile = img2;
        sfile = img1;
        infoh = bih2;
        fileh = bfh2;
        infohsmall = bih1;
        filehsmall = bfh1;
        }
    
    // allocate space to store data of big image
    BYTE *bigdata = (BYTE *)mmap(NULL, fileh.bfSize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    fread(bigdata, fileh.bfSize, 1, bfile);
    // data for small image
    BYTE *smalldata = (BYTE *)mmap(NULL, filehsmall.bfSize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    fread(smalldata, filehsmall.bfSize, 1, sfile);
    // data for output image
    BYTE *outdata = (BYTE *)mmap(NULL, fileh.bfSize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    int wib = 3 * infohsmall.biWidth; // width in bytes per row of small image: biWidth is width in pixels
    int padding = (4 - wib % 4) % 4; // padding on each row
    int rwib_sm = wib + padding; // full width in bytes of small image, accounting for padding

    wib = 3 * infoh.biWidth; // wib big image
    padding = (4 - wib % 4) % 4;
    int rwib = wib + padding; // full width in bytes per row of big image

    float w_ratio = (float)infohsmall.biWidth / (float)infoh.biWidth; // width ratio: small / big
    float h_ratio = (float)infohsmall.biHeight / (float)infoh.biHeight; // heigh ratio: small / big

    timespec s_time; // initial time 
    clock_gettime(CLOCK_MONOTONIC, &s_time); 

    pid_t child_pid, wpid; // for forking purposes
    int status = 0;

    int rows_per_process = infoh.biHeight / num_processes; // calculate amount of data each process needs to process
    for (int j = 0; j < num_processes; j++)
        {
        child_pid = fork();
        if (child_pid == 0)
            {
            int start_row = j * rows_per_process; // where to retrieve and modify data
            int end_row = (j == num_processes - 1) ? infoh.biHeight : (j + 1) * rows_per_process; // where to stop
            // important that the last process continues to the full height of the image to account for leftover pixels in row calculations
            
            for (int y = start_row; y < end_row; y++)
                {
                for (int x = 0; x < infoh.biWidth; x++)
                    {
                    float fxs = (float) x * w_ratio; // x cords of big img in terms of small
                    int ixs = (int) fxs;
                    float fys = (float) y * h_ratio; // y of small i.t.o. big
                    int iys = (int) fys;

                    float dx = fxs - ixs; // delta value x
                    float dy = fys - iys; // delta value y
                    
                    int pix_idx = 3 * x + y * rwib;
                    BYTE *pix = &bigdata[pix_idx];
                    
                    BYTE i_big[3];
                    BYTE big[3] = {pix[0], pix[1], pix[2]}; // grb values of pixel

                    BYTE *surr_pix[4]; // array for surrounding pixels
                    get_neighbor_pix(surr_pix, smalldata, ixs, iys, infohsmall.biWidth, infohsmall.biHeight, rwib_sm); // four surrounding pixels

                    BYTE i_small[3]; // grb values of small pixel
                    for (int i = 0; i < 3; i++)
                        {
                        i_small[i] = interp_color(surr_pix, i, dx, dy); // computer bilinear interpolation for small image
                        pix[i] = big[i] * (1 - ratio) + ratio * i_small[i]; // compute linear interpolation for image mixing
                        outdata[pix_idx + i] = pix[i]; // write the final computed values to output data array
                        }
                    }
                }

            exit(0);
            }
        }

    while ((wpid = wait(&status)) > 0); // collect child process

    // write info from bigger image into outfile header
    fwrite(&fileh.bfType, 2, 1, outfile);
    fwrite(&fileh.bfSize, 4, 1, outfile);
    fwrite(&fileh.bfReserved1, 2, 1, outfile);
    fwrite(&fileh.bfReserved2, 2, 1, outfile);
    fwrite(&fileh.bfOffBits, 4, 1, outfile);
    fwrite(&infoh.biSize, 4, 1, outfile);
    fwrite(&infoh.biWidth, 4, 1, outfile);
    fwrite(&infoh.biHeight, 4, 1, outfile);
    fwrite(&infoh.biPlanes, 2, 1, outfile);
    fwrite(&infoh.biBitCount, 2, 1, outfile);
    fwrite(&infoh.biCompression, 4, 1, outfile);
    fwrite(&infoh.biSizeImage, 4, 1, outfile);
    fwrite(&infoh.biXPelsPerMeter, 4, 1, outfile);
    fwrite(&infoh.biYPelsPerMeter, 4, 1, outfile);
    fwrite(&infoh.biClrUsed, 4, 1, outfile);
    fwrite(&infoh.biClrImportant, 4, 1, outfile);

    // write computed data to output file
    fwrite(outdata, fileh.bfSize, 1, outfile);

    timespec f_time; // get final time 
    clock_gettime(CLOCK_MONOTONIC, &f_time);
    long elapsed_time_ns = (f_time.tv_nsec - s_time.tv_nsec);
    cout << "total time: " << elapsed_time_ns << " microseconds" << endl;
    // code for getting clock resolution
    // timespec res;
    // clock_getres(CLOCK_MONOTONIC, &res);
    // cout << res.tv_nsec << endl;

    munmap(bigdata, sizeof(bigdata));
    munmap(smalldata, sizeof(smalldata));
    munmap(outdata, sizeof(outdata));
    fclose(img1);
    fclose(img2);
    fclose(outfile);

    return 0;
}