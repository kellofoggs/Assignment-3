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


//char* strupr part of string.h and stdio.h
void print_directory(int file_descriptor, char* source_path, char* file_name );
void find_file(char* directory_path, superblock_t* superblock, void* start_of_file, int* cursor, int* root_end, int block_size, dir_entry_t** source_file);
void traverse_fat(int cursor, void* mem_mapping, dir_entry_t* entry, int fd,int fat_start, int fat_count);
void write_to_img(int file_system_fd, char* os_file_name, char* img_file_path );
int find_empty_fat_slot(superblock_info_t *superblock, void* start_of_file);
void write_to_block(int cursor, int img_fd, int os_fd, int *remaining_bytes, void* mem_mapping);
int block_size;





/**
 * Starts with cursor at the start of fat table and moves cursor and with knowledge of where the first block is and the block size
 * 
*/
void traverse_fat(int cursor, void* mem_mapping, dir_entry_t* entry, int fd,int fat_start, int fat_count){
	int temp_cursor = cursor;
	int buffer;
	int output_buffer;
	int current_block = htonl(entry->starting_block);
	//printf("Current block: %u\n", current_block);
	int num_blocks = htonl(entry->block_count);
	//int 

	//read the entire block that we're currently looking at
	//memcpy(&output_buffer, mem_mapping+temp_cursor, block_size);

	//printf("output buffer %u\n", output_buffer);
	bool end_of_file_reached = false;
	//read the four bytes in the current block in the fat to get the current block
	memcpy(&buffer,mem_mapping+(cursor), 4);
	buffer = htonl(buffer);
	int start_of_fat_in_bytes = fat_start*block_size;
	int blocks_traversed = 0;
	//While we haven't gotten to 0xffff ffff in the FAT
	while (!end_of_file_reached){
		//Write out the current block
		//write_block(cursor, fd, mem_mapping);
	

		//Look at fat table and move current block to the next block
		memcpy(&current_block,mem_mapping+ start_of_fat_in_bytes+(current_block*4), 4);
		current_block = htonl(current_block);
		
		//printf("cursor @ %u\n", cursor);
		cursor = current_block*block_size;
		blocks_traversed++;

		//Stops looking for next block in file when we've got end of file hex code or all blocks have been traversed
		if (blocks_traversed >= fat_count || blocks_traversed >= num_blocks || current_block == 0xffffffff){
			end_of_file_reached = true;
		}


		//output_buffer = htonl(output_buffer);
		//printf("next_block:%u\n", output_buffer);

	}
	//printf("The buffer's value is: %u\n", buffer);

	//read the 



}


/**
 * Finds the file to be moved starting from the root block traversing deeper, stops if any part of the path is not findable
 * 
 * @args:
 * 	directory_path: the path that leads to a directory that is to be printed 
 * 	superblock: a packed struct that holds info on the superblock
 *  start_of_file: a pointer to the memory mapping (used to create packed struct for directory entries)
 * 	cursor:	pointer to value that tracks what part of the file we're looking at
 * 	root_end: pointer to value that tracks the end of the current directory entry (originally the end of the root directory)
 * 	block_size: the size of a block in the file system, typically will be 512
 * 
 * 
 * @returns: nothing
 * 
*/
void find_file(char* directory_path, superblock_t* superblock, void* start_of_file, int* cursor, int* root_end, int block_size, dir_entry_t** source_file){


	bool directory_found = false;
	char full_path[2^32];
	strcpy(full_path, directory_path);
	dir_entry_t* entry;
	/**
	 *	If the directory path is the root, then do nothing as cursor starts at the root and root_end is 
	 *  already setup to be the end of the root 
	 */


	if (strcmp("/", directory_path) == 0){
		return;
	}

	char* dir_to_look_for = strtok(directory_path, "/");
	int temp_cursor = *cursor;
	int temp_root_end = *root_end;
	//look through all entries in current root till we find the name we're looking for
	while (temp_cursor < temp_root_end && dir_to_look_for != NULL){

		entry = (dir_entry_t*)(start_of_file + temp_cursor);

		//When we find the matching name in the relevant section of path
		if (strcmp(entry->filename, dir_to_look_for) == 0){

			//Get the next section of the path if any and move cursor to it
			dir_to_look_for = strtok(NULL, "/");
			temp_cursor = htonl(entry->starting_block) *block_size;
			temp_root_end = (htonl(entry->block_count) +htonl(entry->starting_block))*512;

			//If no next section of the path then we've found the desired file, return the file
			if (dir_to_look_for == NULL){
				directory_found = true;
				//printf("Found the file  @ %u with %u bytes", htonl(entry->starting_block)*512, htonl(entry->block_count)*512);
				
				 *source_file = entry ;
			}
			continue;

		}
		//Look at the next directory entry
		temp_cursor = temp_cursor + 64;

	}
	if (!directory_found){
		printf("Directory with path %s not found\n", full_path);
		exit(-1);
	}

		




}
/**
 * Function that opens file given and writes it into the img provided
 * 
*/
void write_to_img(int file_system_fd,  char* img_file_path, char* os_file_name){
   	struct stat source_file_stats;
	struct stat img_file_stats;
	superblock_t superblock;
	superblock_info_t superblock_info;

    //Get the os file's stats for replication
    int os_file_fd = open(os_file_name, O_RDONLY);
    if (os_file_fd < 0){
		printf("Could not open the source file");
        exit(-1);
    }


	/*int img_file_fd = open(img_file_path, 0);
	if (img_file_fd < 0){
		printf("Could not open the img file");
		exit(-1);
	}*/
    lseek(file_system_fd, 0, SEEK_SET);

    int fstat_return = fstat(os_file_fd, &source_file_stats);
	int fstat_return_two = fstat(file_system_fd, &img_file_stats);

    if (fstat_return < 0 || fstat_return_two < 0){
		perror("fstat()");
		exit(-1);
	}
	
	int source_file_size = source_file_stats.st_size;
	int img_file_size = img_file_stats.st_size;
	

	//memory map the image
	void *start_of_file =  mmap(NULL, img_file_size, PROT_READ, MAP_PRIVATE, file_system_fd, 0);

	//Create superblock packed struct
	superblock = *(superblock_t*)start_of_file;
	prep_superblock_struct(file_system_fd, &superblock_info);

	//Setup for root directory in big-endian using superblock
	int root_directory_start = superblock_info.root_directory_start;
	int root_directory_blocks = superblock_info.root_directory_blocks;

	int fat_start = superblock_info.start_of_fat;

	block_size = superblock_info.block_size;

	//Setup the cursor at the bit that signifies start of root
	int cursor = block_size*root_directory_start;
	int root_end = block_size * (root_directory_start + root_directory_blocks);
	int remaining_bytes = source_file_size;
	//Look for free blocks in the FAT
	uint32_t free_spot = find_empty_fat_slot(&superblock_info, start_of_file);
	int previous_spot = -1;
	printf("Free spot @ block %d\n", free_spot);
	uint32_t blocks_taken = 0;
	
	while(remaining_bytes > 0){
		//If not the first block set value of previous spot to current spot
		if (previous_spot != -1){

		}
		//Reserve free spot by writing "1"
		//cursor;
		cursor = (fat_start*block_size)+ (4 * free_spot);
		printf("cursor: %u\n", cursor);
		lseek(file_system_fd, cursor,SEEK_SET);
		int reserved = 1234;
		write(file_system_fd, &reserved, 4);
		printf("Reserved: %d\n",reserved);
		write(1, &reserved, 4);
		previous_spot = free_spot;
		
		//Move cursor to the right block and write to it
		cursor = free_spot*block_size;
		write_to_block(cursor, file_system_fd, os_file_fd, &remaining_bytes, start_of_file);
		blocks_taken++;

		//Upon succesful write of a block find the next free spot and point the resreved spot in fat to it
		free_spot = find_empty_fat_slot(&superblock_info, start_of_file);
		cursor =  (fat_start*block_size) + (4*previous_spot);		
		lseek(file_system_fd, cursor, SEEK_SET);
		write(file_system_fd, &free_spot, 4);



		//Move to the next free_fat_block
		


		

		




	}
	//Move the file cursor to the right position in the FAT
		//cursor = +free_spot*4;

		//Move the file cursor empty position in FDT


	//Write the end of the file first





    

}

bool is_empty_entry(dir_entry_t* dir_entry){
	uint8_t last_bit = (dir_entry->status) & 1;
	return (last_bit == 0);

}

void write_to_block(int cursor, int img_fd, int os_fd, int *remaining_bytes, void* mem_mapping){
	//Move the file cursor to the start of the block to be written
	lseek(img_fd, cursor, SEEK_SET);
	uint8_t buffer;
	int counter = 0;
	while(*remaining_bytes > 0 && counter < block_size){
		
		read(os_fd, &buffer, 1);
		//Undo when writing to file.
		//buffer = htonl(buffer);
		
		printf("%u\n", buffer);
		write(img_fd, &buffer, 1);
		*remaining_bytes = *remaining_bytes-1;
		counter++;

		printf("Remaining bytes: %d, counter: %d\n", *remaining_bytes, counter);
	}

}

/**Traverses down the fat until an empty spot (x0000 0000 is found)*/
int find_empty_fat_slot(superblock_info_t *superblock, void* start_of_file){
	//Initialize the current_fat reading as something besides 0 
	uint32_t current_fat_reading = 0;
	int counter = 0;

	//Get bounds of fat 
	bool found_spot;
	int cursor = (superblock->start_of_fat * superblock->block_size);
	printf("Cursor in method: %d\n", cursor);
	int end_of_fat = (superblock->start_of_fat + superblock->blocks_in_fat) * block_size; 
	
	while (cursor < end_of_fat){
		memcpy(&current_fat_reading, start_of_file+cursor, 4);

		current_fat_reading = htonl(current_fat_reading);
		//printf("current reading: %u\n", current_fat_reading);

		//When we find an empty spot return the fat index
		if (current_fat_reading == 0){
			//printf("counter: %d\n", counter);
			return counter;
		}		
		//Move the cursor along to the next FAT entry

		cursor = cursor+4;

		counter++;
	}
	
	//Exit if the FAT is full
	printf("There were no available spots in the FAT\n");
	exit(-1);
	

}

/**
 * Main method 
 * @args:
 * 	argc: the number of arguments passed in when the process is invoked
 * argv[]
 * 
*/
int main(int argc, char* argv[]){

	//Handle arguments
	if (argc == 1){
        printf("Include the name of the img file after the command as an argument, followed by the path to the "
		"directory\n");
        exit(-1);
    }

    /*if (argc != 4){
        printf("Invalid argument(s) provided! Try ./diskput [name_of_image]  [name_of_file_in_operating_system] [path_of_directory_in_img\n");
        exit(-1);
    }*/


	//Open the img file with read access only
    char* img_file_name = argv[1];
	int img_file_descriptor; //= open(img_file_name, 0);


	//When the file opening caused an error
	/*if (img_file_descriptor < 0 ){
		printf("The file could not be opened. The file may not exist or you may not have access to it\n");
		exit(-1);
	}
*/
    char* path_of_file_in_img = argv[2];
    char* path_of_source_in_os = argv[3];

	img_file_descriptor = open("test.img", O_RDWR |O_APPEND );
	path_of_file_in_img = "";
	path_of_source_in_os = "mkfile.cc";//"foo.txt";

	write_to_img(img_file_descriptor, path_of_file_in_img, path_of_source_in_os);
	//print_directory(img_file_descriptor, name_of_file_in_os, path_to_dest_in_img);
	close(img_file_descriptor);

	return 0;
}