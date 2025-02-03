#include <unistd.h>

int main()
{
	int a[2];
	pipe(a); // now a[0] = 3, a[1] = 4
	// a[0] is the read end
	// a[1] is the write end
	// 3 and 4 are the file descriptors
  int dupa0 = dup(a[0]);
	
	char text[100];
	write(a[1], "hello!", 7);
	int bytesread = read(a[0], text, 50); 
	// will not read more from pipe than the pipe contains
	int x = 2;
}