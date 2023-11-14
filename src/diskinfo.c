#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

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

	return 0;
}
