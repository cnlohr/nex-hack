#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "src/lzpt_io.h"
//eg
//
// /mnt/c/projects/nex-hack/bin/lzpt_writer -i level5/nflasha8_usr_ext2.fsimg -o level3/fdat_fw/0700_part_image/dev/nflasha8 -l 1


char plog_global[8192];

void log_it(char *pinfo) { fprintf( stderr, "%s\n", pinfo ); }

int main( int argc, char ** argv )
{
	int c;
	int compress_level = -1;
	const char * infile = 0;
	const char * outfile = 0;
	int newfmt = 0;
	int showhelp = 0;
	while ((c = getopt (argc, argv, "l:i:o:nh")) != -1)
	switch (c)
	{
		case 'l': compress_level = atoi( optarg ); break;
		case 'i': infile = optarg; break;
		case 'o': outfile = optarg; break;
		case 'n': newfmt = 1; break;
		case 'h': showhelp = 1; break;
	}
	if( !infile || !outfile || compress_level < -1 || compress_level > 9 || showhelp )
	{
		fprintf( stderr, "Usage: lzpt_writer [-i file] [-o file] [-l level(-1 to 9)] [-n] [-h]\n  Where -n means version 0x11, and lack there of is version 0x10\n  -h shows this help.\n  This tool creates lzpt (TPZL) files for Sony cameras\n" );
		return -9;
	}

	int r = lzpt_compress_file( infile, outfile, newfmt?0x11:0x10, compress_level);
	if( r )
	{
		fprintf( stderr, "File out failed.\n" );
	}
	return r;
}
