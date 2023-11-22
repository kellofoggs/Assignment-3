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
void print_name_and_size(void *start_of_file, int cursor);
void get_all_fields(void *start_of_file, int cursor, char** output_string);


void get_all_fields(void *start_of_file, int cursor, char** output_string){
	int status_byte = 0;
	char file_or_dir_char;
	char name[31];
	int size;

	//Get status of whether entry is file/directory
	memcpy(&status_byte, start_of_file+cursor, 1);

	//Print status
	if (status_byte == 3){
		file_or_dir_char = 'F';
	}
	else if (status_byte== 5){
		file_or_dir_char = 'D';
	}
	
	//Get size
	memcpy(&size, start_of_file+cursor+9, 4);
	size = htonl(size);

	//get name
	strcpy(name, start_of_file+cursor+27);	

	//get_date
	int year = 0;
	int month = 0;
	int day = 0;

	memcpy(&year, start_of_file+cursor+13, 2);
	year = htons(year);

	//Because we only have 1 byte the endianess does not matter

	memcpy(&month, start_of_file+cursor+15, 1);
	memcpy(&day, start_of_file+cursor+16, 1);




	//get_time
	int hours = 0;
	int minutes = 0;
	int seconds = 0;


	//Because we only have 1 byte the endianess does not matter
	memcpy(&hours, start_of_file+cursor+17, 1);
	memcpy(&minutes, start_of_file+cursor+18, 1);
	memcpy(&seconds, start_of_file+cursor+19, 1);

	


	//If we have a non-empty entry print its info
	if (status_byte != 0){
		printf("%c %10d %30s %4d/%02d/%02d %02d:%02d:%02d\n", file_or_dir_char, size, name, year,month,day, hours, minutes, seconds);
	}

	



}
void print_directory_or_file(void* start_of_file, int cursor){
	int status_byte = 0;
	char output;
	
	memcpy(&status_byte, start_of_file+cursor, 1);

	if (status_byte == 3){
		output = 'F';
	}
	else if (status_byte== 5){
		output = 'D';
	}

	printf("%c	",output);


}
void print_time(void* start_of_file, int cursor){
	int hours = 0;
	int minutes = 0;
	int seconds = 0;

	memcpy(&hours, start_of_file+cursor+17, 1);
	memcpy(&minutes, start_of_file+cursor+18, 1);
	memcpy(&seconds, start_of_file+cursor+19, 1);


	printf("%02d:%02d:%02d\n",hours, minutes, seconds);

}

void print_name_and_size(void *start_of_file, int cursor){

	char name[31];
	int size;
	strcpy(name, start_of_file+cursor+27);
	memcpy(&size, start_of_file+cursor+9, 4);
	size = htonl(size);
	printf("%d	%s	", size,name);

}
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

	int block_size = htons(superblock->block_size);

	//Setup the cursor at the bit that signifies start of root
	int cursor = block_size*root_directory_start;
	int root_end = block_size * (root_directory_start + root_directory_blocks);
	//printf("%d\n",root_directory_start);
	
	//Go to desired directory
	while (cursor < root_end){

		/*print_directory_or_file(start_of_file, cursor);
		print_name_and_size(start_of_file, cursor);
		print_time(start_of_file, cursor);
		*/
		get_all_fields(start_of_file, cursor, NULL);
		//Move to next entry
		cursor = cursor+64;
	}


	//Print the directory out


}

int main(int argc, char* argv[]){

	//Handle arguments
	if (argc == 1){
        printf("Include the name of the img file after the command as an argument, followed by the path to the "
		"directory\n");
        exit(-1);
    }
 if (argc != 2){
        printf("Invalid argument(s) provided! Try ./diskist [name_of_image.img] /[path_of_directory]\n"
		);
        exit(-1);
    }

   /* if (argc != 3){
        printf("Invalid argument(s) provided! Try ./diskist [name_of_image.img] /[path_of_directory]\n"
		);
        exit(-1);
    }*/

	//Open the file with read access only
	int file_descriptor = open(argv[1], 0);


	//When the file opening caused an error
	if (file_descriptor < 0 ){
		printf("The file could not be opened. The file may not exist or you may not have access to it\n");
		perror("open()");
		exit(-1);
	}

	print_directories(file_descriptor);
	close(file_descriptor);

	return 0;
}