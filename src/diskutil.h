#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
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

typedef struct superblock_info{
	uint16_t 	block_size;
	uint32_t	fs_block_count;
	uint32_t	start_of_fat;
	uint32_t	blocks_in_fat;
	uint32_t	root_directory_start;
	uint32_t	root_directory_blocks;
	uint32_t free_blocks;
	uint32_t reserved_blocks;
	uint32_t allocated_blocks;

}superblock_info_t;


void prep_superblock_struct(int file_descriptor, superblock_info_t* superblock_info_struct);
void prep_fat_struct(superblock_info_t* superblock_info_struct, void* start_of_file);

