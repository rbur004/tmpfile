#include <stdio.h>

#ifndef TMPFILE
#define TMPFILE

	typedef struct tmpfile
	{
		FILE *fp;
		char filename[256];
		char flags[16];
		int no_unlink;
	} Tmpfile;

#endif //TMPFILE

