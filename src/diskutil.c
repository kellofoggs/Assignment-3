#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <netinet/in.h>
#include "diskutil.h"




/**
 * Function that takes in a file descriptor and a struct that it populates with superblock info and fat info
 * @args:
 * 	file_descriptor: fd of an open img file that we want more info on
 * 	superblock_info_struct: pointer to a struct created outside of function that we want to fill with info
 *
*/

void prep_superblock_struct(int file_descriptor, superblock_info_t* superblock_info_struct){
	struct stat buf;
	superblock_t* the_super_block;

	int file_cursor;

	//Load file stats so we can get the size of file for memory mapping purposes.
	int fstat_return = fstat(file_descriptor, &buf);
	if (fstat_return < 0){
		perror("fstat()");
		exit(-1);
	}
	
	//Memory map the image file and create a 
	void* start_of_file = mmap(NULL, buf.st_size, PROT_READ, MAP_PRIVATE, file_descriptor, 0);

	//Load info from mmap into the struct

	the_super_block = (superblock_t*)start_of_file;
	

	//Ensure big endianness and load attributes from packed struct into the superblock_info_struct
	superblock_info_struct->block_size = htons(the_super_block->block_size);
	superblock_info_struct->fs_block_count = htonl(the_super_block->file_system_block_count);
	superblock_info_struct->start_of_fat = htonl(the_super_block->fat_start_block);
	superblock_info_struct->blocks_in_fat = htonl(the_super_block->fat_block_count);
	superblock_info_struct->root_directory_start = htonl(the_super_block->root_dir_start_block);
	superblock_info_struct->root_directory_blocks = htonl(the_super_block->root_dir_block_count);

		prep_fat_struct(superblock_info_struct, start_of_file);


	
	

}

void prep_fat_struct(superblock_info_t* superblock_info_struct, void* start_of_file){
	int num_free_blocks = 0;
	int num_reserved_blocks = 0;
	int num_allocated_blocks = 0;

	int start_of_fat = superblock_info_struct->start_of_fat;
	int block_size = superblock_info_struct->block_size;
	int blocks_in_fat = superblock_info_struct->blocks_in_fat;

	int cursor = start_of_fat*block_size;
	int end_of_fat = block_size*(start_of_fat+blocks_in_fat);



	//Traverse through FAT 4 bytes at a time 
	while ( cursor < end_of_fat){
		int current_bytes;
		memcpy(&current_bytes, start_of_file+cursor, 4);
		current_bytes = htonl(current_bytes);

		if (current_bytes == 0){
			num_free_blocks++;
		}else if(current_bytes == 1){
			num_reserved_blocks++;
		}else {
			num_allocated_blocks++;
		}
		cursor = cursor + 4;
	}
	superblock_info_struct->free_blocks = num_free_blocks;
	superblock_info_struct->reserved_blocks = num_reserved_blocks;
	superblock_info_struct->allocated_blocks = num_allocated_blocks;

}
