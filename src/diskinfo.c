#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <netinet/in.h>
#include "diskutil.h"

void print_superblock(int file_descriptor);
void print_fat_info(int block_size, int start_of_fat, int blocks_in_fat, void *mmaped_file);


/**
 * Function that takes in a file descriptor and prints out the superblock and FAT information
 * 
 * 
*/
void print_superblock(int file_descriptor){
	struct stat* buf;
	superblock_t* the_super_block;
	int file_cursor;

	//Load file stats so we can get the size of file for memory mapping purposes.
	int fstat_return = fstat(file_descriptor, buf);
	if (fstat_return < 0){
		perror("fstat()");
		exit(-1);
	}
	
	//Memory map the image file and create a 
	void* start_of_file = mmap(NULL, buf->st_size, PROT_READ, MAP_PRIVATE, file_descriptor, 0);

	//Load info from mmap into the struct

	the_super_block = (superblock_t*)start_of_file;
	
	int block_size = 0;

	//Ensure big endianness and load attributes from packed struct
	block_size = htons(the_super_block->block_size);
	int fs_block_count = htonl(the_super_block->file_system_block_count);
	int start_of_fat = htonl(the_super_block->fat_start_block);
	int blocks_in_fat = htonl(the_super_block->fat_block_count);
	int root_directory_start = htonl(the_super_block->root_dir_start_block);
	int root_directory_blocks = htonl(the_super_block->root_dir_block_count);


	
	printf(
		"Super block information\n"
		"Block size: %u\n"
		"Block count: %u\n"
		"FAT starts: %u\n"
		"FAT blocks: %u\n"
		"Root directory start: %u\n"
		"Root directory blocks:%u \n\n",

		
		block_size,
		fs_block_count,
		start_of_fat,
		blocks_in_fat,
		root_directory_start,
		root_directory_blocks
	);
	
	print_fat_info(block_size,start_of_fat, blocks_in_fat, start_of_file);



}
/** 
 * Helper function that prints out the FAT table info (free, allocated, and reserved blocks)
 * @args:
 * 	block_size: the size of blocks in the file system, typically 512
 * 	start_of_fat: the start of the fat table in blocks
 * 	blocks_in_fat: the number of blocks that the FAT takes up
 * @return:
 * 	the function returns nothing
*/
void print_fat_info(int block_size, int start_of_fat, int blocks_in_fat, void *mmaped_file){
	int num_free_blocks = 0;
	int num_reserved_blocks = 0;
	int num_allocated_blocks = 0;

	int cursor = start_of_fat*block_size;
	int end_of_fat = block_size*(start_of_fat+blocks_in_fat);



	//Traverse through FAT 4 bytes at a time 
	while ( cursor < end_of_fat){
		int current_bytes;
		memcpy(&current_bytes, mmaped_file+cursor, 4);
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

	printf(
		"FAT information\n"
		"Free Blocks: %u\n"
		"Reserved Blocks: %u\n"
		"Allocated Blocks: %u\n", 

		num_free_blocks,
		num_reserved_blocks,
		num_allocated_blocks);
}

int main(int argc, char* argv[]){

	//Handle arguments
	if (argc == 1){
        printf("Include the name of the img file after the command as an argument\n");
        exit(-1);
    }
    if (argc != 2){
        printf(
			"The argument provided is invalid! Try ./disklit [name_of_image.img]\n"
		);
        exit(-1);
    }

	//Open the file with read access only
	int file_descriptor = open(argv[1], 0);


	//When the file opening caused an error
	if (file_descriptor < 0 ){
		printf("The file could not be opened. The file may not exist or you may not have access to it\n");
		exit(-1);
	}

	print_superblock(file_descriptor);
	close(file_descriptor);

	return 0;
}
