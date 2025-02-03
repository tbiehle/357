#include <stdlib.h>
#include <stdio.h>
#include <cmath>
#include <string>
#include <libc.h>
#include <sys/mman.h>

typedef unsigned char BYTE;

class bfh {
    public:
    BYTE bfType[2];
	int bfSize;
	BYTE bfReserved1[2];
	BYTE bfReserved2[2];
	int bfOffBits;
};

class bih {
    public:
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

bfh* read_bfh(FILE *file) {
    // reads all data from bitmap file header into new bfh class
    bfh *ibfh = new bfh;

    fread(&ibfh->bfType, 2, 1, file);
    fread(&ibfh->bfSize, 4, 1, file);
    fread(&ibfh->bfReserved1, 2, 1, file);
    fread(&ibfh->bfReserved2, 2, 1, file);
    fread(&ibfh->bfOffBits, 4, 1, file);

    return ibfh;
}

bih* read_bih(FILE *file) {
    // reads all data from bitmap info header into bih class
    bih *ibih = new bih;

    fread(&ibih->biSize, 4, 1, file);
    fread(&ibih->biWidth, 4, 1, file);
    fread(&ibih->biHeight, 4, 1, file);
    fread(&ibih->biPlanes, 2, 1, file);
    fread(&ibih->biBitCount, 2, 1, file);
    fread(&ibih->biCompression, 4, 1, file);
    fread(&ibih->biSizeImage, 4, 1, file);
    fread(&ibih->biXPelsPerMeter, 4, 1, file);
    fread(&ibih->biYPelsPerMeter, 4, 1, file);
    fread(&ibih->biClrUsed, 4, 1, file);
    fread(&ibih->biClrImportant, 4, 1, file);

    return ibih;
}

void write_bfh(bfh *ibfh, FILE *file) {
    fwrite(&ibfh->bfType, 2, 1, file);
    fwrite(&ibfh->bfSize, 4, 1, file);
    fwrite(&ibfh->bfReserved1, 2, 1, file);
    fwrite(&ibfh->bfReserved2, 2, 1, file);
    fwrite(&ibfh->bfOffBits, 4, 1, file);
}

void write_bih(bih *ibih, FILE *file) {
    fwrite(&ibih->biSize, 4, 1, file);
    fwrite(&ibih->biWidth, 4, 1, file);
    fwrite(&ibih->biHeight, 4, 1, file);
    fwrite(&ibih->biPlanes, 2, 1, file);
    fwrite(&ibih->biBitCount, 2, 1, file);
    fwrite(&ibih->biCompression, 4, 1, file);
    fwrite(&ibih->biSizeImage, 4, 1, file);
    fwrite(&ibih->biXPelsPerMeter, 4, 1, file);
    fwrite(&ibih->biYPelsPerMeter, 4, 1, file);
    fwrite(&ibih->biClrUsed, 4, 1, file);
    fwrite(&ibih->biClrImportant, 4, 1, file);
}

BYTE *color_grade_no_fork(bih *ibih, FILE *file, float grades[]) {
    BYTE *data = (BYTE*)malloc(ibih->biSizeImage);
    BYTE *cdata = (BYTE*)malloc(ibih->biSizeImage);
    int wib = 3 * ibih->biWidth; // width in bytes
    
    fread(data, ibih->biSizeImage, 1, file);
    // calculate REAL width in bytes: needs to be multiple of four
    int rwib = (wib % 4 == 0) ? wib : (wib + 4 - (wib % 4));
    for (int y = 0; y < ibih->biHeight; y++) {
        for (int x = 0; x < ibih->biWidth; x++) {
            BYTE *pix = &data[3 * x + y * rwib];

            float mod_values[] = {pix[0] / 255., pix[1] / 255., pix[2] / 255.};

            for (int i = 0; i < 3; i++) {
                mod_values[i] *= grades[2 - i];
            }

            for (int i = 0; i < 3; i++) { // normalize values
                float cur = mod_values[i];
                cur = (cur > 1) ? 1 : cur;
                cur = (cur < -1) ? -1 : cur;
                mod_values[i] = cur;
            }

            BYTE *mpix = &cdata[3 * x + y * rwib];
            for (int i = 0; i < 3; i++) mpix[i] = mod_values[i] * 255.;
        }
    }

    return cdata;
}

BYTE *color_grade_with_fork(bih *ibih, FILE *file, float grades[]) {
    BYTE *data = (BYTE*)mmap(NULL, ibih->biSizeImage, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    BYTE *cdata = (BYTE*)mmap(NULL, ibih->biSizeImage, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    int wib = 3 * ibih->biWidth; // width in bytes
    int rwib = (wib % 4 == 0) ? wib : (wib + 4 - (wib % 4));

    fread(data, ibih->biSizeImage, 1, file);
    fclose(file);
    if (fork() == 0) // in child process
    {
        for (int y = 0; y < ibih->biHeight / 2; y++)
        {
            for (int x = 0; x < ibih->biWidth; x++)
            {
                BYTE *pix = &data[3 * x + y * rwib];

                float mod_values[] = {pix[0] / 255., pix[1] / 255., pix[2] / 255.};

                for (int i = 0; i < 3; i++) {
                    mod_values[i] *= grades[2 - i];
                }

                for (int i = 0; i < 3; i++) { // normalize values
                    float cur = mod_values[i];
                    cur = (cur > 1) ? 1 : cur;
                    cur = (cur < -1) ? -1 : cur;
                    mod_values[i] = cur;
                }

                BYTE *mpix = &cdata[3 * x + y * rwib];
                for (int i = 0; i < 3; i++) mpix[i] = mod_values[i] * 255.;
            }
        }
        exit(0);
    }
    else
    {
        wait(0);
        for (int y = ibih->biHeight / 2; y < ibih->biHeight; y++)
        {
            for (int x = 0; x < ibih->biWidth; x++)
            {
                BYTE *pix = &data[3 * x + y * rwib];

                float mod_values[] = {pix[0] / 255., pix[1] / 255., pix[2] / 255.};

                for (int i = 0; i < 3; i++) {
                    mod_values[i] *= grades[2 - i];
                }

                for (int i = 0; i < 3; i++) { // normalize values
                    float cur = mod_values[i];
                    cur = (cur > 1) ? 1 : cur;
                    cur = (cur < -1) ? -1 : cur;
                    mod_values[i] = cur;
                }

                BYTE *mpix = &cdata[3 * x + y * rwib];
                for (int i = 0; i < 3; i++) mpix[i] = mod_values[i] * 255.;
            }
        }
        
    }
    munmap(cdata, sizeof(cdata));
    return cdata;
}


void write_file(bfh *ibfh, bih *ibih, BYTE *data, FILE *out_file) {
    write_bfh(ibfh, out_file);
    write_bih(ibih, out_file);
    fwrite(data, ibih->biSizeImage, 1, out_file);
}

int main(int argc, char *argv[]) {
    // command line paramaters: 
    // 0: program name
    // 1-3: color grading float numbers
    // 4: output file name
    
    char infile[255];
    strcpy(infile, argv[1]);
    FILE *file = fopen(infile, "rb");
    //FILE *file = fopen("lion.bmp", "rb");
    if (file == NULL) {
        printf("Error1");
        return 0;
    }

    bfh *ibfh = read_bfh(file);
    bih *ibih = read_bih(file);

    char outfile[255];
    strcpy(outfile, argv[5]);
    FILE *out_file = fopen(outfile, "wb");
    //FILE *out_file = fopen("result.bmp", "wb");
    if (out_file == NULL) {
        printf("Error2");
        return 0;
    };

    float grades[] = {atof(argv[2]), atof(argv[3]), atof(argv[4])};
    //float grades[] = {0.9, 1.0, 0.93};

    clock_t start = clock();
    BYTE *cdata = color_grade_with_fork(ibih, file, grades);
    clock_t end = clock();
    long elapsed_time = (end - start);
    printf("Time with fork (ms): %ld\n", elapsed_time);

    file = fopen(infile, "rb");
    // file = fopen("lion.bmp", "rb");
    fseek(file, 0, SEEK_SET);
    read_bfh(file);
    read_bih(file);
    if (file == NULL) {
        printf("Error1");
        return 0;
    }

    start = clock();
    cdata = color_grade_no_fork(ibih, file, grades);
    end = clock();
    elapsed_time = (end - start);
    printf("Time without fork (ms): %ld\n", elapsed_time);

    write_file(ibfh, ibih, cdata, out_file);
    delete ibfh;
    delete ibih;

    fclose(out_file);
    fclose(file);
    return 0;
}