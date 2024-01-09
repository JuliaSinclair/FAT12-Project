#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h> 
#include "diskfunctions.h"

int main(int argc, char *argv[]){
	// Check for input files
    if (argc != 3) {
        fprintf(stderr, "Please input a file system image AND file name\n");
        exit(1);
    }

	// Make file name input uppercase
	for (unsigned int i = 0, n = strlen(argv[2]); i < n; i++) {
    	argv[2][i] = (char) toupper(argv[2][i]);
  	}

	// Opening File System Image 
    // Modified code from CSC 360 Fall 2022 Tutorial 8
	int fd;
	struct stat sb;

	fd = open(argv[1], O_RDWR);
	fstat(fd, &sb);

	char * p = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (p == MAP_FAILED) {
		printf("Error: failed to map memory\n");
		exit(1);
	}

    // Check if input file is in root directory
	int file_found = 0;
	int file_size;
	int first_cluster;
	int directory = SECTOR_SIZE*19;
	for(int i = 0; i < SECTOR_SIZE * 14; i += 32){
		int first_logical_cluster = ((p[i + directory + 27]&0xFF)<<8) + (p[i + directory + 26]&0xFF);
		uint8_t attributes = p[i + 11 + directory];
		int volume_label_bit = (attributes & ( 1 << 3 )) >> 3;
		int subdirectory_bit = (attributes & (1 << 4)) >> 4;
		if (p[i + directory] == 0x00){
			break;
		}
		if(subdirectory_bit == 0 && p[i + directory] != 0x2E && attributes != 0x0F && first_logical_cluster != 0 && first_logical_cluster != 1 && volume_label_bit != 1 && p[i + directory] != 0xE5){
			char* name = malloc(sizeof(char));
				int k;
				for (k = 0; k < 8; k++){
					name[k] = p[k + i + directory];
				}
				name[k] = '\0';
			char* extension = malloc(sizeof(char));
				int l;
				for (l = 8; l < 11; l++){
					extension[l-8] = p[l + i + directory];
				}
				extension[l] = '\0';

			trim_white_space(name);
			trim_white_space(extension);
			strcat (name, ".");
			strcat(name, extension);

			if(strcmp(argv[2], name) == 0){
				file_found = 1;
				file_size = get_file_size(p, i + directory);
				first_cluster = first_logical_cluster;
				free(name);
				free(extension);
				break;
			}	
		}
	}

	if (file_found == 1){
		printf("%s File Found: ", argv[2]);
		
		// Create file in linux directory
		int writing_fd;
		writing_fd = open(argv[2], O_RDWR| O_CREAT, 0666);
		if (writing_fd == -1){
			fprintf(stderr, "Failed to create new file in Linux directory\n");
			munmap(p, sb.st_size);
			close(fd);
			exit(1);
		}
		int file_end = lseek(writing_fd, file_size - 1, SEEK_SET);
		if (file_end == -1){
			fprintf(stderr, "Failed to find end of new file in Linux directory\n");
			munmap(p, sb.st_size);
			close(fd);
			close(writing_fd);
			exit(1);
		}
		int write_at_end = write(writing_fd, "", 1);
		if (write_at_end == -1){
			fprintf(stderr, "Failed to write at the end of new file in Linux directory\n");
			munmap(p, sb.st_size);
			close(fd);
			close(writing_fd);
			exit(1);
		}

		// Opening Memory for Writing the File 
		// Modified code from CSC 360 Fall 2022 Tutorial 8
		char * writing_p = mmap(NULL, file_size, PROT_WRITE, MAP_SHARED, writing_fd, 0);
		if (writing_p == MAP_FAILED) {
			printf("Failed to map memory for writing the file\n");
			munmap(p, sb.st_size);
			close(fd);
			exit(1);
		}

		// Copy file system image file to linux directory file
		int start = 1;
		int size_countdown = file_size; 
		while (start == 1 || (first_cluster != 0x00 && 0xFF0 > first_cluster)){
			int physical_address = (first_cluster + 31) * SECTOR_SIZE;
			for (int i = 0; i < SECTOR_SIZE; i++){
				if (size_countdown == 0){
					break;
				}
				writing_p[file_size - size_countdown] = p[physical_address + i];
				size_countdown--;

			}
			start = 0;
			first_cluster = read_FAT_entry(p, first_cluster);
		}

		printf("Succesfully copied into local directory\n");

		//Free memory to writing memory map
		munmap(writing_p, file_size);
		close(writing_fd);
		
	}
	else{
		printf("%s File Not Found\n", argv[2]);
	}

	//Free memory for reading memory map
	munmap(p, sb.st_size);
	close(fd);
	return 0;
}