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
void print_directory(int img_file_descriptor, char* source_path, char* file_name );
void find_file(char* directory_path, superblock_t* superblock, void* start_of_file, int* cursor, int* root_end, int block_size, dir_entry_t** source_file);
void traverse_fat(int cursor, void* mem_mapping, dir_entry_t* entry, int fd,int fat_start, int fat_count, FILE* ffd);
void write_block(int cursor, int fd, void* mem_mapping, FILE* ffd);
int block_size;





/**
 * Function that copies file from img to operating system
 *  @args: 
 * 	img_file_descriptor: file descriptor returned from open system command
 * 	directory_path: path to desired directory
 * 
 * @returns:
 * 	nothing is returned
 * 
*/
void print_directory(int img_file_descriptor, char* source_path, char* target_file_name ){
	
	struct stat buf;
	superblock_t superblock;
	int file_cursor = 0;
	
	//Use fstat as we need the size of the img
	int fstat_return = fstat(img_file_descriptor, &buf);
	if (fstat_return < 0){
		perror("fstat()");
		exit(-1);
	}

	//Memory map the img
	void *start_of_file =  mmap(NULL, buf.st_size, PROT_READ, MAP_PRIVATE, img_file_descriptor, 0);

	//Create superblock packed struct
	superblock = *(superblock_t*)start_of_file;

	//Setup for root directory in big-endian using superblock
	int root_directory_start = htonl(superblock.root_dir_start_block);
	int root_directory_blocks = htonl(superblock.root_dir_block_count);

	block_size = htons(superblock.block_size);

	//Setup the cursor at the bit that signifies start of root
	int cursor = block_size*root_directory_start;
	int root_end = block_size * (root_directory_start + root_directory_blocks);
	
	//Find desired file which sets the cursor and root_end
	dir_entry_t* source_file_entry;
	find_file(source_path, &superblock, start_of_file, &cursor, &root_end, block_size, &source_file_entry);

	//Move cursor to the start of the block 
	cursor = htonl(source_file_entry->starting_block)*block_size;

	//copy desired_bytes into a file outside of the system, give full access to all 3 types of users, access octal number generated with chdmod-calculator.org
	int os_system_file = 2; //= open(target_file_name, O_WRONLY| O_CREAT, 0777);
	FILE *os_file;
	os_file = fopen(target_file_name, "w");
	if (os_file == NULL){
		perror("fopen()");
		exit(-1);
	}

	if (os_system_file < 0){
		printf("There was an error creating the file in the current working directory");
		exit(-1);

	}
	traverse_fat(cursor, start_of_file, source_file_entry, os_system_file,htonl(superblock.fat_start_block), htonl(superblock.fat_block_count),os_file);


	close(os_system_file);
}

/**
 * Starts with cursor at the start of fat table and moves cursor and with knowledge of where the first block is and the block size
 * 
*/
void traverse_fat(int cursor, void* mem_mapping, dir_entry_t* entry, int fd,int fat_start, int fat_count, FILE* ffd){
	int temp_cursor = cursor;
	int buffer;
	int output_buffer;
	uint32_t current_block = htonl(entry->starting_block);
	uint32_t num_blocks = htonl(entry->block_count);
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
		write_block(cursor, fd, mem_mapping, ffd);
	

		//Look at fat table and move current block to the next block
		memcpy(&current_block,mem_mapping+ start_of_fat_in_bytes+(current_block*4), 4);
		current_block = htonl(current_block);
		
		cursor = current_block*block_size;
		blocks_traversed++;

		//Stops looking for next block in file when we've got end of file hex code or all blocks have been traversed
		if (blocks_traversed >= fat_count || blocks_traversed >= num_blocks || current_block == 0xffffffff){
			end_of_file_reached = true;
		}



	}



}

/**
 * Writes out block in 4 byte intervals
 * @args:
 * 	int cursor: the cursor that starts at the beginning of the block
 * 
*/
void write_block(int cursor, int fd, void* mem_mapping, FILE* ffd){
	uint32_t buffer;
	//write(fd, &buffer, 4);
	int block_end = cursor+block_size;
	int counter = 0;
	//traverse through block reading
	while (cursor < block_end){
		memcpy(&buffer,mem_mapping+cursor, 4);
	 	//buffer = htonl(buffer);
		//printf("%u\n", buffer);
		//write(fd, &buffer, 4);
		fwrite(&buffer, 4, 1, ffd);
		//fseek(ffd, 0, SEEK_END);
		//write(fd, &counter, 1);
		//write (fd, "\n", 1);
		cursor = cursor + 4;
		counter++;
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
			temp_root_end = (htonl(entry->block_count) +htonl(entry->starting_block) * block_size);

			//If no next section of the path then we've found the desired file, return the file
			if (dir_to_look_for == NULL){
				directory_found = true;
				
				 *source_file = entry ;
			}
			continue;

		}
		//Look at the next directory entry
		temp_cursor = temp_cursor + 64;

	}
	if (!directory_found){
		printf("File not found");
		exit(-1);
	}

		




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

    if (argc != 4){
        printf("Invalid argument(s) provided! Try ./diskget [name_of_image] [path_of_directory_in_file_system] [name_of_file]\n");
        exit(-1);
    }


	//Open the file with read access only
	int img_file_descriptor = open(argv[1], 0);


	//When the file opening caused an error
	if (img_file_descriptor < 0 ){
		printf("The file could not be opened. The file may not exist or you may not have access to it\n");
		exit(-1);
	}
	print_directory(img_file_descriptor, argv[2],argv[3]);
	close(img_file_descriptor);


	/*int fd = open("test.img", 0);
	if (fd < 0){
		perror("open()");
	}
	print_directory(fd, "/foo.txt", "foo.txt");
	close(fd);*/
	return 0;
}