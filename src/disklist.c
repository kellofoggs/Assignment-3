#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
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

void print_directories(int file_descriptor ){
	struct stat* buf;
	superblock_t* the_super_block;
	int file_cursor = 0;
	
	//Use fstat as we need the size of the img
	fstat(file_descriptor, buf);


	//Memory map the img
	void *start_of_file =  mmap(NULL, buf->st_size, PROT_READ, MAP_PRIVATE, file_descriptor, 0);

	//Create superblock packed struct
	superblock_t* superblock = (superblock_t*)start_of_file;

	//Setup for root directory print in big-endian
	int root_directory_start = htonl(superblock->root_dir_start_block);
	int root_directory_blocks = htonl(superblock->root_dir_block_count);

	int block_size = superblock->block_size;

	//Setup the cursor at the bit that signifies start of root
	int cursor = block_size*root_directory_start;
	int root_end = block_size * (root_directory_start + root_directory_blocks);
	
	//Go to desired directory


	//Print the directory out


}

int main(int argc, char* argv[]){

	//Handle arguments
	if (argc == 1){
        printf("Include the name of the img file after the command as an argument, followed by the path to the "
		"directory\n");
        exit(-1);
    }
    if (argc != 3){
        printf("Invalid argument(s) provided! Try ./diskist [name_of_image.img] /[path_of_directory]\n"
		);
        exit(-1);
    }

	//Open the file with read access only
	int file_descriptor = open(argv[2], 0);


	//When the file opening caused an error
	if (file_descriptor < 0 ){
		printf("The file could not be opened. The file may not exist or you may not have access to it\n");
		exit(-1);
	}

	//print_superblock(file_descriptor);
	close(file_descriptor);

	return 0;
}