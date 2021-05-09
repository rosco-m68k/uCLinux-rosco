/*
 *  linux/zBoot/xtract.c
 *
 *  Copyright (C) 1993  Hannu Savolainen
 *
 *	Extracts the system image and writes it to the stdout.
 *	based on tools/build.c by Linus Torvalds
 */

#include <stdio.h>	/* fprintf */
#include <string.h>
#include <stdlib.h>	/* contains exit */
#include <sys/types.h>	/* unistd.h needs this */
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>	/* contains read/write */
#include <fcntl.h>
#include <a.out.h>

#define N_MAGIC_OFFSET 1024

static int GCC_HEADER = sizeof(struct exec);

#define STRINGIFY(x) #x

void die(char * str)
{
	fprintf(stderr,"%s\n",str);
	exit(1);
}

void usage(void)
{
	die("Usage: xtract system [ | gzip | piggyback > piggy.s]");
}

int main(int argc, char ** argv)
{
	int id, sz;
	char buf[1024];

	struct exec *ex = ((struct exec *)buf) + 1;

	if (argc  != 2)
		usage();
	
	if ((id=open(argv[1],O_RDONLY,0))<0)
		die("Unable to open 'system'");
	if (read(id,ex,GCC_HEADER) != GCC_HEADER)
		die("Unable to read header of 'system'");

	switch (N_MAGIC(*ex)) {
	case ZMAGIC:
		GCC_HEADER = N_MAGIC_OFFSET;
		lseek(id, GCC_HEADER, SEEK_SET);
		break;
	case QMAGIC:
		memset (buf, 0, GCC_HEADER);
		buf[0x14] = 0x20;
		write (1, buf, GCC_HEADER);
		break;
	default:
		die("Non-GCC header of 'system'");
	}

	sz = N_SYMOFF(*ex) - GCC_HEADER + 4;	/* +4 to get the same result than tools/build */

	fprintf(stderr, "System size is %d\n", sz);

	while (sz) {
		int l, n;

		l = sz;
		if (l > sizeof(buf)) l = sizeof(buf);

		if ((n=read(id, buf, l)) !=l)
		{
			if (n == -1) 
			   perror(argv[1]);
			else
			   fprintf(stderr, "Unexpected EOF\n");

			die("Can't read system");
		}

		write(1, buf, l);
		sz -= l;
	}

	close(id);
	return(0);
}
