#include <stdlib.h>
#include <iostream>
#include <stdio.h>
#include <algorithm>
#include <queue>

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

class huff_node {
public:
    int val, freq;
    huff_node *left, *right, *parent;
    int digits;
};

bool comp(huff_node *h1, huff_node *h2) {
    if (!h1) return false;
    if (!h2) return true;
    return h1->freq < h2->freq;
}

huff_node *create_huff_tree(huff_node **nodes) {
    int length = 255;

    // check where null pointers start
    for (int i = 0; i < length; i++) {
        if (nodes[i] == NULL) {
            length = i - 1;
            break;
        }
    }

    while (nodes[1] || nodes[2]) { // node array has more than one valid pointer
        sort(nodes, nodes + length, comp);
        length--;

        huff_node *combined = new huff_node;

        nodes[0]->parent = nodes[1]->parent = combined;
        combined->left = nodes[0];
        combined->right = nodes[1];

        combined->freq = nodes[0]->freq + nodes[1]->freq; // combine frequencies
        combined->val = -1; // make garbage value to indicate this is not a leaf

        nodes[0] = combined;
        nodes[1] = NULL;
    }

    return nodes[0];
}

void create_code_h(huff_node *node, char *codes, char code=0) {
    if (!node->left && !node->right) {
        codes[node->val] = code;
    }

    code <<= 1;
    if (node->left) {
        create_code_h(node->left, codes, code | 0b0);
    }
    if (node->right) {
        create_code_h(node->right, codes, code | 0b1);
    }
}

void create_code(huff_node *root, char *codes) {
    if (!root) return;

    create_code_h(root, codes);
}

int main() {
    int quality = 256;
    int scale = 256 / quality;

    huff_node *red_freq[256];
    huff_node *green_freq[256];
    huff_node *blue_freq[256];
    huff_node **col_freqs[] = {red_freq, green_freq, blue_freq};

    for (int i = 0; i < 256; i++) {
        red_freq[i] = NULL;
        green_freq[i] = NULL;
        blue_freq[i] = NULL;
    }

    // open file and read data;
    FILE *img = fopen("test.bmp", "rb");
    bih bih;
    bfh bfh;
    // read header info of img
    fread(&bfh.bfType, 2, 1, img);
    fread(&bfh.bfSize, 4, 1, img);
    fread(&bfh.bfReserved1, 2, 1, img);
    fread(&bfh.bfReserved2, 2, 1, img);
    fread(&bfh.bfOffBits, 4, 1, img);
    fread(&bih.biSize, 4, 1, img);
    fread(&bih.biWidth, 4, 1, img);
    fread(&bih.biHeight, 4, 1, img);
    fread(&bih.biPlanes, 2, 1, img);
    fread(&bih.biBitCount, 2, 1, img);
    fread(&bih.biCompression, 4, 1, img);
    fread(&bih.biSizeImage, 4, 1, img);
    fread(&bih.biXPelsPerMeter, 4, 1, img);
    fread(&bih.biYPelsPerMeter, 4, 1, img);
    fread(&bih.biClrUsed, 4, 1, img);
    fread(&bih.biClrImportant, 4, 1, img);

    BYTE *data = (BYTE *)malloc(bih.biSizeImage);
    fread(data, bih.biSizeImage, 1, img);
    BYTE *outdata = (BYTE *)malloc(bih.biSizeImage);

    int wib = 3 * bih.biWidth;
    int padding = (4 - wib % 4) % 4; // padding on each row
    int rwib = wib + padding;

    for (int y = 0; y < bih.biHeight; y++) {
        for (int x = 0; x < bih.biWidth; x++) {
            int pix_idx = 3 * x + y * rwib;
            BYTE *pix = &data[pix_idx];

            for (int i = 0; i < 3; i++) {
                // get pix value and scale it by quantity
                int pix_val = (pix[i] / scale) * scale;

                outdata[pix_idx + i] = pix_val;

                // get current node and increase the frequency
                huff_node **freq_array = col_freqs[i];
                if (!freq_array[pix_val]) {
                    freq_array[pix_val] = new huff_node;
                }
                freq_array[pix_val]->freq++;
                freq_array[pix_val]->val = pix_val;
            }
        }
    }

    sort(begin(red_freq), end(red_freq), comp);
    sort(begin(green_freq), end(green_freq), comp);
    sort(begin(blue_freq), end(blue_freq), comp);

    // create huff trees
    create_huff_tree(red_freq);
    create_huff_tree(green_freq);
    create_huff_tree(blue_freq);

    // create huff codes

    char red_codes[256];
    create_code(red_freq[0], red_codes);
    char blue_codes[256];
    create_code(blue_freq[0], blue_codes);
    char green_codes[256];
    create_code(green_freq[0], green_codes);
    
    // write huff code to outfile

    FILE *compfile = fopen("comp.huff", "wb");
    fwrite(&bfh.bfType, 2, 1, compfile);
    fwrite(&bfh.bfSize, 4, 1, compfile);
    fwrite(&bfh.bfReserved1, 2, 1, compfile);
    fwrite(&bfh.bfReserved2, 2, 1, compfile);
    fwrite(&bfh.bfOffBits, 4, 1, compfile);
    fwrite(&bih.biSize, 4, 1, compfile);
    fwrite(&bih.biWidth, 4, 1, compfile);
    fwrite(&bih.biHeight, 4, 1, compfile);
    fwrite(&bih.biPlanes, 2, 1, compfile);
    fwrite(&bih.biBitCount, 2, 1, compfile);
    fwrite(&bih.biCompression, 4, 1, compfile);
    fwrite(&bih.biSizeImage, 4, 1, compfile);
    fwrite(&bih.biXPelsPerMeter, 4, 1, compfile);
    fwrite(&bih.biYPelsPerMeter, 4, 1, compfile);
    fwrite(&bih.biClrUsed, 4, 1, compfile);
    fwrite(&bih.biClrImportant, 4, 1, compfile);
    
    

    fclose(img);
    fclose(compfile);

    free(data);
    free(outdata);

    return 0;
}