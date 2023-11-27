#include "diskutil.h"
//Compile with gcc -o diskinfo diskinfo.c diskutil.c





void print_superblock(int file_descriptor);


/**
 * Function that takes in a file descriptor and prints out the superblock and FAT information from a struct with info
 * 
 * 
*/
void print_superblock(int file_descriptor){

	superblock_info_t superblock_info;	
	prep_superblock_struct(file_descriptor, &superblock_info);


	
	printf(
		"Super block information\n"
		"Block size: %u\n"
		"Block count: %u\n"
		"FAT starts: %u\n"
		"FAT blocks: %u\n"
		"Root directory start: %u\n"
		"Root directory blocks:%u \n\n",

		
		superblock_info.block_size,
		superblock_info.fs_block_count,
		superblock_info.start_of_fat,
		superblock_info.blocks_in_fat,
		superblock_info.root_directory_start,
		superblock_info.root_directory_blocks
	);
	
	printf(
		"FAT information\n"
		"Free Blocks: %u\n"
		"Reserved Blocks: %u\n"
		"Allocated Blocks: %u\n", 

		superblock_info.free_blocks,
		superblock_info.reserved_blocks,
		superblock_info.allocated_blocks);
	


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

	//Open the file with read access onlyf
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
