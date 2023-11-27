//
// mkfile.cc
//

#include <stdio.h>
#include <iostream>

#define BUFFER_SIZE	512
#define BLOCK_COUNT	5

using namespace std;

int main ( int argc, char *argv[] )
{
	char *buffer[BUFFER_SIZE];
	int	blockCount = BLOCK_COUNT;
	char ch	= 'A';
	
	if ( argc != 4 )
	{
		cout << "Usage: makefile filename size character" << endl;
		return 0;
	}
	
	FILE *file = fopen (argv[1], "wb");
	
	if ( !file )
	{
		cout << "Couldn't create: " << argv[1] << endl;
		return 0;
	}
	
	if ( argv[2] )
		blockCount = atoi(argv[2]);
	
	if ( argv[3] )
		ch = argv[3][0];
	
	for ( int i = 0; i < blockCount; i++ )
	{
		memset (buffer, (int)ch, BUFFER_SIZE);
		fwrite (buffer, sizeof (char), BUFFER_SIZE, file);
		ch++;
	}
	fclose (file);
	
	return 0;
}


	
	T;
	char ch	= 'A';
	
	if ( argc != 4 )
	{
		cout << "Usage: makefile filename size character" << endl;
		return 0;
	}
	
	FILE *file = fopen (argv[1], "wb");
	
	if ( !file )
	{
		cout << "Couldn't create: " << argv[1] << endl;
		return 0;
	}
	
	if ( argv[2] )
		blockCount = atoi(argv[2]);
