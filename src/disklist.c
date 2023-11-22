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

/**
 * Function that uses packed directory entry struct to get info about the entry
 * Be
*/
void get_all_fields(void *start_of_file, int cursor, char** output_string){
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
	//memcpy(&status_byte, start_of_file+cursor, 1);

	//Print status
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
	//Because we only have 1 byte the endianess does not matter

	int hours = creation_time.hour;
	int minutes = creation_time.minute;
	int seconds = creation_time.second;


	//If we have a non-empty entry print its info
	if (status_byte != 0){
		//printf("%c %10d %30s %4d/%02d/%02d %02d:%02d:%02d\n", file_or_dir_char, size, name, year,month,day, hours, minutes, seconds);
		printf("%d\n",cursor);
	}
			//printf("%s\n", name);


	



}

void print_directories(int file_descriptor, char* directory_path ){
	struct stat* buf;
	superblock_t* superblock;
	int file_cursor = 0;
	
	//Use fstat as we need the size of the img
	fstat(file_descriptor, buf);


	//Memory map the img
	void *start_of_file =  mmap(NULL, buf->st_size, PROT_READ, MAP_PRIVATE, file_descriptor, 0);

	//Create superblock packed struct
	superblock = (superblock_t*)start_of_file;

	//Setup for root directory print in big-endian
	int root_directory_start = htonl(superblock->root_dir_start_block);
	int root_directory_blocks = htonl(superblock->root_dir_block_count);

	int block_size = htons(superblock->block_size);

	//Setup the cursor at the bit that signifies start of root
	int cursor = block_size*root_directory_start;
	int root_end = block_size * (root_directory_start + root_directory_blocks);
	//printf("%d\n",root_directory_start);
	
	//Find directory
	
	//Go through desired directory
	dir_entry_t* directory_entry;
	
	//find_directory(directory_path, superblock, &cursor, &start_of_file);

	while (cursor < root_end){
		//printf("%d\n", directory_entry->size);
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

void find_directory(char* directory_path, superblock_t* superblock,int* cursor, int* root_end){

	//If the directory path is the root, then do nothing as cursor starts at the root
	if (strcmp("/", directory_path) == 0){
		return;
	}

	char* dir_to_look_for = strtok(directory_path, "/");
	int temp_cursor = *cursor;
	while (temp_cursor < *root_end){
		//look through all entries till we find the name we're looking for
		

		temp_cursor = temp_cursor + 64;
	}




	//separate path into file/directory names


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

	print_directories(file_descriptor, argv[2]);
	close(file_descriptor);

	return 0;
}