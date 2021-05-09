/*
 *	linux/zBoot/piggyback.c
 *
 *	(C) 1993 Hannu Savolainen
 */

/*
 *	This program reads the compressed system image from stdin and
 *	encapsulates it into an object file written to the stdout.
 */

#include <stdio.h>
#include <unistd.h>
#include <a.out.h>
#include <sys/fcntl.h>

int main(int argc, char *argv[])
{
	int n=0, len=0, fd = 0;
	char tmp_buf[640*1024];
	
	struct exec obj = {0x00670107};	/* object header */
	char string_names[] = {"_input_data\0_input_end\0"};

	struct nlist var_names[2] = /* Symbol table */
		{
			{	/* _input_data	*/
				{(char *)4}, 7, 0, 0, 0
			},
			{	/* _input_len */
				{(char *)16}, 7, 0, 0, 0
			}
		};


	if (argc > 1) {
		fd = open (argv[1], O_RDONLY);
		if (fd < 0) {
			fprintf (stderr, "%s: ", argv[0]);
			perror (argv[1]);
			exit (1);
		}
	}

	len = 0;
	while ((n = read(fd, &tmp_buf[len], sizeof(tmp_buf)-len+1)) > 0)
	      len += n;

	len = (len + 3) & ~3;

	if (n==-1) {
		perror("stdin");
		exit(-1);
	}

	if (len >= sizeof(tmp_buf)) {
		fprintf(stderr, "%s: Input too large\n", argv[0]);
		exit(-1);
	}

	if (argc < 2)
		fprintf(stderr, "Compressed size %d.\n", len);

/*
 *	Output object header
 */
	obj.a_data = len /* + sizeof(long) */;
	obj.a_syms = sizeof(var_names);
	write(1, (char *)&obj, sizeof(obj));

/*
 *	Output data segment (compressed system & len)
 */
	write(1, tmp_buf, len);

/*
 *	Output symbol table
 */
	var_names[1].n_value = len;
	write(1, (char *)&var_names, sizeof(var_names));

/*
 *	Output string table
 */
	len = sizeof(string_names) + sizeof(len);
	write(1, (char *)&len, sizeof(len));
	write(1, string_names, sizeof(string_names));

	exit(0);

}
