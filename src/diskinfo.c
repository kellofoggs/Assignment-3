#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


/**
 * Includes struct stat and fstat()

 * struct stat {
	dev_t st_dev;
	ino_t st_ino;
	mode_t st_mode;
	nlink_t st_nlink;
	uid_t st_uid;
	gid_t st_gid;
	dev_t st_rdev;
	off_t st_size;
	time_t st_atime;
	time_t st_mtime;
	time_t st_ctime;
	blksize_t st_blksize;
	blkcnt_t st_blocks;
	mode_t st_attr;
}; 
 * 
 * 
 * 
*/


#include <sys/types.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <netinet/in.h>
//Struct for superblock
typedef struct __attribute__((__packed__))superblock{
	uint8_t fs_id [8];
	uint16_t block_size;
	uint32_t file_system_block_count;
	uint32_t fat_start_block;
	uint32_t fat_block_count;
	uint32_t root_dir_start_block;
	uint32_t root_dir_block_count;


} superblock_t;

//Date and time of an entry into a directory
typedef struct __attribute__((__packed__)) dir_entry_timedate {
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
} dir_entry_timedate_t;

//Directory entry struct
typedef struct __attribute__((__packed__)) dir_entry{
	uint8_t status;
	uint32_t starting_block;
	uint32_t block_count;
	uint32_t size;
	dir_entry_timedate_t create_time;
	dir_entry_timedate_t modify_time;
	uint8_t filename[31];
	uint8_t unused[6];

}dir_entry_t;

void print_superblock(int file_descriptor);
void print_fat_info(int block_size, int start_of_fat, int blocks_in_fat, void *mmaped_file);


/**
 * Function that takes in a file descriptor and prints out the superblock and FAT information
 * 
 * 
*/
void print_superblock(int file_descriptor){
	struct stat buf;
	superblock_t* the_super_block;
	int file_cursor;


	int fstat_return = fstat(file_descriptor, &buf);
	
	//Memory map the image file and create a 
	void* start_of_file = mmap(NULL, buf.st_size, PROT_READ, MAP_PRIVATE, file_descriptor, 0);

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
		"Super block information:\n"
		"Block size: %d\n"
		"Block count: %d\n"
		"FAT starts: %d\n"
		"FAT blocks: %d\n"
		"Root directory start: %d\n"
		"Root directory blocks:%d \n\n",

		
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
 * 
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
		}else if (current_bytes > 1){
			num_allocated_blocks++;
		}
		cursor = cursor + 4;
	}

	printf(
		"FAT information: \n"
		"Free Blocks: %d\n"
		"Reserved Blocks: %d\n"
		"Allocated Blocks: %d\n", 

		num_free_blocks,
		num_reserved_blocks,
		num_allocated_blocks);
}

int main(int argc, char* argv[]){

	//Handle arguments
	if (argc == 1){
        printf("Include the name of the file after the command as an argument\n");
        exit(-1);
    }
    if (argc != 2){
        printf("The argument provided is invalid!\n");
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
