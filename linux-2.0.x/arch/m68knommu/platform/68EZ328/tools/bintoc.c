#include <stdio.h>

int
main(int argc, char *argv[])
{
	unsigned char buf[8];
	int cnt;
	int i;

	printf ("/* romfs.h: ROMfs image converted to a C char array
   Machine generated DO NOT EDIT */
char
_flashstart[] = {");

	while (cnt = read(0,buf,sizeof(buf))) {

		printf("\n");
		for (i=0; i<cnt; i++) {
			printf(" 0x%.2x,",buf[i]); 
		}
	}
	printf("\n0};\n");
}

