#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <cmath>
#include <string>
#include <libc.h>

int main() {
    FILE * fd = fopen("file.txt", O_RDONLY);
    
    char *c;
    
    fread(c, 1, 1, fd);
    fclose(fd);
}