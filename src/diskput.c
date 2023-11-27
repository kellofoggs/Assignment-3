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
void get_all_fields(void *start_of_file, int cursor);
void traverse_fat(int cursor, void* mem_mapping, dir_entry_t* entry, int fd,int fat_start, int fat_count);
void write_block(int cursor, int fd, void* mem_mapping);
int block_size;



void get_all_fields(void *start_of_file, int cursor){
	int status_byte = 0;
	char file_or_dir_char;
	char* name;
	int size;

	dir_entry_t* directory_entry = (dir_entry_t*)(start_of_file+cursor);


	/**
	 * Get status of whether entry is file/directory
	 * Because we only have 1 byte the endianess does not matter, we don't convert with hton
	*/
	
	status_byte = directory_entry->status;

	//Set status char
	if (status_byte == 3){
		file_or_dir_char = 'F';
	}
	else if (status_byte== 5){
		file_or_dir_char = 'D';
	}
	
	//Get size
	//memcpy(&size, start_of_file+cursor+9, 4);
	size = htonl(directory_entry->size);

	//get name

	name = directory_entry->filename;
	//get creation date
	
	dir_entry_timedate_t creation_time = directory_entry->create_time;
	int year = htons(creation_time.year);

	//Because we only have 1 byte the endianess does not matter, we don't convert with hton

	int month = creation_time.month;
	int day = creation_time.day;



	//Get creation time
	//Because we only have 1 byte the endianess does not matter, we don't convert with hton

	int hours = creation_time.hour;
	int minutes = creation_time.minute;
	int seconds = creation_time.second;


	//If we have a non-empty entry print its info
	if (status_byte != 0){
		printf("%c %10d %30s %4d/%02d/%02d %02d:%02d:%02d\n", file_or_dir_char, size, name, year,month,day, hours, minutes, seconds);
	}
}

/**
 * Function that prints out the desired directory
 * @args: 
 * 	file_descriptor: file descriptor returned from open system command
 * 	directory_path: path to desired directory
 * 
 * @returns:
 * 	nothing is returned
 * 
*/
void print_directory(int file_descriptor, char* source_path, char* target_file_name ){
	
	struct stat* buf;
	superblock_t* superblock;
	int file_cursor = 0;
	
	//Use fstat as we need the size of the img
	fstat(file_descriptor, buf);


	//Memory map the img
	void *start_of_file =  mmap(NULL, buf->st_size, PROT_READ, MAP_PRIVATE, file_descriptor, 0);

	//Create superblock packed struct
	superblock = (superblock_t*)start_of_file;

	//Setup for root directory in big-endian using superblock
	int root_directory_start = htonl(superblock->root_dir_start_block);
	int root_directory_blocks = htonl(superblock->root_dir_block_count);

	block_size = htons(superblock->block_size);

	//Setup the cursor at the bit that signifies start of root
	int cursor = block_size*root_directory_start;
	int root_end = block_size * (root_directory_start + root_directory_blocks);
	
	//Find desired file which sets the cursor and root_end
	dir_entry_t* source_file_entry;
	find_file(source_path, superblock, start_of_file, &cursor, &root_end, block_size, &source_file_entry);
	//printf("source file entry: %s\n", source_file_entry->filename);

	//Move cursor to the start of the block 
	cursor = htonl(source_file_entry->starting_block)*block_size;

	

	//copy desired_bytes into a file outside of the system, give full access to all 3 types of users, access octal number generated with chdmod-calculator.org
	int unix_system_file = open(target_file_name, O_WRONLY| O_CREAT, 0777);
	if (unix_system_file < 0){
		printf("There was an error creating the file in the current working directory");
		exit(-1);

	}
	traverse_fat(cursor, start_of_file, source_file_entry, unix_system_file,htonl(superblock->fat_start_block), htonl(superblock->fat_block_count));


	close(unix_system_file);

	//while (cursor < root_end){
		//get_all_fields(start_of_file, cursor);
	
		//Move to next entry
		//cursor = cursor+64;
	//}
}

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
		write_block(cursor, fd, mem_mapping);
	

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
 * Writes out block in 4 byte intervals
 * @args:
 * 	cursor: the cursor that starts at the beginning of the block
 * 
*/
void write_block(int cursor, int fd, void* mem_mapping){
	uint32_t buffer;
	//write(fd, &buffer, 4);
	int block_end = cursor+block_size;
	//traverse through block reading
	while (cursor < block_end){
		memcpy(&buffer,mem_mapping+cursor, 4);
	 	buffer = htonl(buffer);
		//printf("%u\n", buffer);
		write(fd, &buffer, 4);
		cursor = cursor + 4;
	}



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
void write_to_img(int file_system_fd, char* os_file_name, char* img_file_path ){
   	struct stat* buf;

    //Get the os file's stats for replication
    int os_file_fd = open(os_file_name, O_RDONLY);
    if (os_file_fd < 0){
        perror("fstat()");
        exit(-1);
    }
    

    int fstat_return, fstat(os_file_fd ,buf);

    if (fstat_return < 0){
		perror("fstat()");
		exit(-1);
	}

	int file_size = buf->st_size;

	
	//Look for free blocks in the FAT



	//Write the end of the file first





    

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
	int img_file_descriptor = open(img_file_name, 0);


	//When the file opening caused an error
	if (img_file_descriptor < 0 ){
		printf("The file could not be opened. The file may not exist or you may not have access to it\n");
		exit(-1);
	}
    char* name_of_file_in_os = argv[2];
    char* path_to_dest_in_img = argv[3];
	print_directory(img_file_descriptor, name_of_file_in_os, path_to_dest_in_img);
	close(img_file_descriptor);

	return 0;
}