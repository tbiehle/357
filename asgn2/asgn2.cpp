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


int main(int argc, char *argv[])
{
    bfh bfh1, bfh2;
    bih bih1, bih2;

    FILE *img1 = fopen("mario.bmp", "rb");
    FILE *img2 = fopen("tunnel.bmp", "rb");
    float ratio = 0.5;
    int num_processes = 4;

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
    
    //BYTE *bigdata = (BYTE *)malloc(fileh.bfSize); // allocate space to store data of big image
    BYTE *bigdata = (BYTE *)mmap(NULL, fileh.bfSize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    fread(bigdata, fileh.bfSize, 1, bfile);
    BYTE *smalldata = (BYTE *)mmap(NULL, filehsmall.bfSize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    fread(smalldata, filehsmall.bfSize, 1, sfile);
    BYTE *outdata = (BYTE *)mmap(NULL, fileh.bfSize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    int wib = 3 * infohsmall.biWidth; // width in bytes: biWidth is width in pixels
    int padding = (4 - wib % 4) % 4;
    int rwib_sm = wib + padding; // account for padding

    wib = 3 * infoh.biWidth;
    padding = (4 - wib % 4) % 4;
    int rwib = wib + padding;

    float w_size_ratio = (float)infohsmall.biWidth / (float)infoh.biWidth;
    float h_size_ratio = (float)infohsmall.biHeight / (float)infoh.biHeight;

    clock_t s_time = clock();

    pid_t child_pid, wpid;
    int status = 0;

    int rows_per_process = infoh.biHeight / num_processes;
    for (int j = 0; j < num_processes; j++)
        {
        child_pid = fork();
        if (child_pid == 0)
            {
            int start_row = j * rows_per_process;
            int end_row = (j == num_processes - 1) ? infoh.biHeight : (j + 1) * rows_per_process;
            
            for (int y = start_row; y < end_row; y++)
                {
                for (int x = 0; x < infoh.biWidth; x++)
                    {
                    float fxs = (float) x * w_size_ratio; // x cords of big img in terms of small
                    int ixs = (int) fxs;
                    float fys = (float) y * h_size_ratio; // y of small i.t.o. big
                    int iys = (int) fys;

                    float dx = fxs - ixs; // delta value x
                    float dy = fys - iys; // delta value y
                    
                    int pix_idx = 3 * x + y * rwib;
                    BYTE *pix = &bigdata[pix_idx];
                    
                    BYTE i_big[3];
                    BYTE big[3] = {pix[0], pix[1], pix[2]};

                    BYTE *surr_pix[4];
                    get_neighbor_pix(surr_pix, smalldata, ixs, iys, infohsmall.biWidth, infohsmall.biHeight, rwib_sm); // four surrounding pixels

                    BYTE i_small[3];
                    for (int i = 0; i < 3; i++)
                        {
                        i_small[i] = interp_color(surr_pix, i, dx, dy);
                        pix[i] = big[i] * (1 - ratio) + ratio * i_small[i];
                        outdata[pix_idx + i] = pix[i];
                        }
                    }
                }

            exit(0);
            }
        }

    while ((wpid = wait(&status)) > 0);

    FILE *outfile = fopen("outimg.bmp", "wb"); // create output file 
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
    fwrite(outdata, fileh.bfSize, 1, outfile);

    clock_t f_time = clock();
    cout << "total time: " << (double) (f_time - s_time) << endl;

    munmap(bigdata, sizeof(bigdata));
    munmap(smalldata, sizeof(smalldata));
    fclose(img1);
    fclose(img2);
    fclose(outfile);

    return 0;
}