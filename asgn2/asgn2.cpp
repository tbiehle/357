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
#include <libc.h>

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

BYTE interp_color(BYTE **surr_pix, int col, float t_x, float t_y)
    {
    return (1 - t_x)*(1 - t_y)*surr_pix[0b00][col] + t_x*(1-t_y)*surr_pix[0b10][col] + (1-t_x)*t_y*surr_pix[0b01][col] + t_x*t_y* surr_pix[0b11][col];
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

int main()
{
    bfh bfh1, bfh2;
    bih bih1, bih2;

    FILE *img1 = fopen("wolf.bmp", "rb");
    FILE *img2 = fopen("tunnel.bmp", "rb");
    float ratio = 0.3;

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
    
    BYTE *bigdata = (BYTE *)malloc(fileh.bfSize); // allocate space to store data of big image
    fread(bigdata, fileh.bfSize, 1, bfile);
    BYTE *smalldata = (BYTE *)malloc(filehsmall.bfSize);
    fread(smalldata, filehsmall.bfSize, 1, sfile);
    
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

    int wib = 3 * infohsmall.biWidth; // width in bytes: biWidth is width in pixels
    int padding = (4 - wib % 4) % 4;
    int rwib_sm = wib + padding; // account for padding

    wib = 3 * infoh.biWidth;
    padding = (4 - wib % 4) % 4;
    int rwib = wib + padding;

    float w_size_ratio = (float)infohsmall.biWidth / (float)infoh.biWidth;
    float h_size_ratio = (float)infohsmall.biHeight / (float)infoh.biHeight;

    for (int y = 0; y < infoh.biHeight; y++)
        {
        for (int x = 0; x < infoh.biWidth; x++)
            {
                float fxs = (float) x * w_size_ratio; // x cords of big img in terms of small
                int ixs = (int) fxs;
                float fys = (float) y * h_size_ratio; // y of small i.t.o. big
                int iys = (int) fys;

                float t_x = fxs - ixs; // delta value x
                float t_y = fys - iys; // delta value y
                
                BYTE *pix = &bigdata[3 * x + y * rwib];
                
                BYTE i_big[3];
                BYTE big[3] = {pix[2], pix[1], pix[0]};

                BYTE *surr_pix[4];
                get_neighbor_pix(surr_pix, smalldata, ixs, iys, infohsmall.biWidth, infohsmall.biHeight, rwib_sm); // four surrounding pixels

                BYTE i_small[3];
                for (int i = 0; i < 3; i++)
                    {
                    i_small[i] = interp_color(surr_pix, i, t_x, t_y);
                    pix[i] = big[i] * (1 - ratio) + ratio * i_small[i];
                    }

                fwrite(pix, 3, 1, outfile);
            }
            for (int i = 0; i < padding; i++)
            {
                fputc(0, outfile);
            }
        }

    free(bigdata);
    free(smalldata);
    fclose(img1);
    fclose(img2);
    fclose(outfile);

    return 0;
}