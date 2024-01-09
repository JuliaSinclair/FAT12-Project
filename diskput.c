#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h> 
#include "diskfunctions.h"

/*
	Function: remove_file_name
	Arguments:
		path: file path including directories and file name (e.g. sub/sub1/file)
		file: file name
	Purpose: Removes file name from file path
*/
char* remove_file_name(char* path, const char* file) { 
	char *file_location = strstr(path, file);
    if (*file && file_location != NULL) {
        size_t file_length = strlen(file);
		char *end_path = file_location + file_length;
		
        while ((*file_location++ = *end_path++) != '\0')
            continue;
    }
    return path;
}

/*
	Function: search_directory
	Arguments:
		p: pointer to mapped memory
		directory: location in mapped memory of directory to be printed 
		directory_name: name of directory to be printed 
		searching_name: name of directory or file being searched for in file system
		is_subdir: if looking for a subdirectory 1, if looking for a file 0
	Purpose: Return 0 if not found, 1 if file found, subdirectory location if subdirectory found
*/
int search_directory(char * p, int directory, char* directory_name, char* searching_name, int is_subdir) {
    int found = -1;
    int FAT_entry = directory/SECTOR_SIZE - 31;
    int sector = 512;
    int start = 1;
	char* name = malloc(sizeof(char));
	char* extension = malloc(sizeof(char));
    // If root directory 
    if (directory == SECTOR_SIZE*19){
        sector *= 14;
    }
    while(start == 1 || (FAT_entry != 0x00 && 0xFF0 > FAT_entry)){
        for(int i = 0; i < sector; i += 32){
            int first_logical_cluster = ((p[i + directory + 27]&0xFF)<<8) + (p[i + directory + 26]&0xFF);
            uint8_t attributes = p[i + 11 + directory];
            int volume_label_bit = (attributes & ( 1 << 3 )) >> 3;
            int subdirectory_bit = (attributes & (1 << 4)) >> 4;
            if (p[i + directory] == 0x00){
                break;
            }
            if(p[i + directory] != 0x2E && attributes != 0x0F && first_logical_cluster != 0 && first_logical_cluster != 1 && volume_label_bit != 1 && p[i + directory] != 0xE5){
					int k;
					for (k = 0; k < 8; k++){
						name[k] = p[k + i + directory];
					}
					name[k] = '\0';
					trim_white_space(name);

				if (subdirectory_bit == 1){
					char* subdirectory_name = strcat(strcat(strdup(directory_name), "/"), strdup(name));
					int subdirectory = (first_logical_cluster + 31) * SECTOR_SIZE;

					if (is_subdir == 1 && (strcmp(subdirectory_name, searching_name)) == 0){
						found = subdirectory;
						break;
					}

                    found += search_directory(p , subdirectory, subdirectory_name, searching_name, is_subdir);
                }

				else if (subdirectory_bit == 0){
					int l;
					for (l = 8; l < 11; l++){
						extension[l-8] = p[l + i + directory];
					}
					extension[l] = '\0';

					trim_white_space(extension);
					strcat (name, ".");
					strcat(name, extension);
				    if (is_subdir == 0 && (strcmp(name, searching_name) == 0)){
						found = 1;
						break;
					}
				}
            }
        }
        start = 0;
        if (FAT_entry > 1){
            FAT_entry = read_FAT_entry(p, FAT_entry);
            directory = (FAT_entry+31) * SECTOR_SIZE;
        }
        else if (FAT_entry < 1){
            FAT_entry = 0x00;
        }
    }
	free(extension);
	free(name);
    return found;
}

int main(int argc, char *argv[]){
	// Check for input files
    if (argc != 3) {
        fprintf(stderr, "Please input a file system image AND file name\n");
        exit(1);
    }
	
	// Extract file name from full file path
	int is_root = 1;
	char * file_name = argv[2];
	if (strchr(file_name, '/') != NULL){
		file_name = strrchr(file_name, '/');
		file_name = file_name + 1;
		is_root = 0;
	}
	
	//Check if file is in linux directory
	int input_file = open(file_name, O_RDWR);
	if (input_file == -1){
		fprintf(stderr, "%s Not Found\n", file_name);
        exit(1);
	}

	// Make file path input uppercase
	for (unsigned int i = 0, n = strlen(argv[2]); i < n; i++) {
    	argv[2][i] = (char) toupper(argv[2][i]);
  	}

	//Make file name uppercase
	for (unsigned int i = 0, n = strlen(file_name); i < n; i++) {
    	file_name[i] = (char) toupper(file_name[i]);
  	}

	// Opening File System Image for reading and writing
    // Modified code from CSC 360 Fall 2022 Tutorial 8
	int fd;
	struct stat sb;

	fd = open(argv[1], O_RDWR);
	fstat(fd, &sb);

	char * p = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); 
	if (p == MAP_FAILED) {
		printf("Error: failed to map memory\n");
		exit(1);
	}

	// Check if directory is in file system image
	int dir_location;
	if (is_root == 0){
		char* slash_file = "/";
		slash_file = strcat(strdup(slash_file), strdup(file_name));
		char* file_path = remove_file_name(argv[2], slash_file);
		dir_location = search_directory(p, SECTOR_SIZE*19, "", file_path, 1);
		if (dir_location == -1){
			fprintf(stderr, "%s Directory Not Found\n", file_path);
			munmap(p, sb.st_size);
			close(fd);
			exit(1);
		}
	}

	// Check if file name is already in the disk
	if (search_directory(p, SECTOR_SIZE*19, "", file_name, 0) == 1){
		fprintf(stderr, "There is a File of the Same Name %s in the Disk\n", file_name);
		munmap(p, sb.st_size);
		close(fd);
		exit(1);
	} 

	// Map input file to memory
	// Modified code from CSC 360 Fall 2022 Tutorial 8
	struct stat file_sb;
	fstat(input_file, &file_sb);
	int file_size = (uint64_t)file_sb.st_size;

	char * file_p = mmap(NULL, file_sb.st_size, PROT_READ, MAP_SHARED, input_file, 0); 
	if (p == MAP_FAILED) {
		printf("Error: failed to map memory\n");
		munmap(p, sb.st_size);
		close(fd);
		exit(1);
	}

	//Check disk has enough space for file
	int free_disk_size = free_disk_space(p);
	if (file_size > free_disk_size) {
		fprintf(stderr, "Not enough free space in the disk image. Only %d bytes available in disk.\n", free_disk_size);
		munmap(file_p, file_sb.st_size);
		close(input_file);
		munmap(p, sb.st_size);
		close(fd);
		exit(1);
	}

	//Check if file is empty
	if (file_size == 0){
		fprintf(stderr, "File %s is Empty\n", file_name);
		munmap(file_p, file_sb.st_size);
		close(input_file);
		munmap(p, sb.st_size);
		close(fd);
		exit(1);
	}

	//Update directory
	int copy_cluster = find_free_cluster(p);
	int free_dir = -1;
	if (is_root == 1){
		for (int i = SECTOR_SIZE * 19; i < SECTOR_SIZE * 32 + SECTOR_SIZE; i += 32){
			if (p[i] == 0x00){
				free_dir = i;
				break;
			}
		}
	}
	else {
		for (int i = dir_location; i < dir_location + SECTOR_SIZE; i += 32){
			if (p[i] == 0x00){
				free_dir = i;
				break;
			}
		}
	}

	//Update directory - no free entry 
	if (free_dir == -1){
		fprintf(stderr, "No Free Entry in Directory\n");
		munmap(file_p, file_sb.st_size);
		close(input_file);
		munmap(p, sb.st_size);
		close(fd);
		exit(1);
	}

	//Update directory - update name and extension 
	int ext_index = -1;
	for (int k = 0; k < 8; k++){
		if (file_name[k] == '.'){
			ext_index = k + 1;
		}
		if (ext_index == -1){
			p[free_dir + k] = file_name[k];
		}
		else{
			p[free_dir + k] = ' ';
		}
	}

	for (int k = 8; k < 11; k++){
		p[free_dir + k] = file_name[k + ext_index - 8];
	}

	//Update directory - update file attributes
    p[free_dir + 11] = 0x00;  
	p[free_dir + 17] = 0;
    p[free_dir + 16] = 0;
    p[free_dir + 15] = 0;
    p[free_dir + 14] = 0;
	// Creation Time
	time_t t = time(NULL);
    struct tm *current = localtime(&t);
	int year = current -> tm_year + 1900;
	int month = current -> tm_mon + 1;
	int day = current -> tm_mday;
	int hour = current -> tm_hour;
	int minute = current -> tm_min;
    p[free_dir + 17] |= (year - 1980) << 1; 
    p[free_dir + 17] |= (month - ((p[free_dir + 16] & 0b11100000) >>5)) >> 3;
    p[free_dir+16] |= (month - ((p[free_dir + 17] & 0b00000001) << 3)) << 5;
    p[free_dir+16] |= (day & 0b00011111);
    p[free_dir+15] |= (hour << 3) & 0b11111000;
    p[free_dir+15] |= (minute - ((p[free_dir + 14] & 0b11100000) >> 5)) >> 3;
    p[free_dir+14] |= (minute - ((p[free_dir + 15] & 0b00000111) << 3)) << 5;
	//First Logical Cluster
	p[free_dir + 26] = (copy_cluster - (p[27] << 8)) & 0xFF;
	p[free_dir + 27] = (copy_cluster - p[26]) >> 8;
	//File Size
	p[free_dir + 28] = (file_size & 0x000000FF);
	p[free_dir + 29] = (file_size & 0x0000FF00) >> 8;
	p[free_dir + 30] = (file_size & 0x00FF0000) >> 16;
	p[free_dir + 31] = (file_size & 0xFF000000) >> 24;

	//Copy file
	update_FAT_entry(p, copy_cluster, 1);
	int copied_bytes = file_size;

	while (copied_bytes > 0){
		if (copy_cluster == -1){
			fprintf(stderr, "Could Not Find Free Cluster in Directory\n");
			munmap(file_p, file_sb.st_size);
			close(input_file);
			munmap(p, sb.st_size);
			close(fd);
			exit(1);
		}

		int cluster_address = (copy_cluster + 31) * SECTOR_SIZE;

		int byte_count = 0;
		while(byte_count < SECTOR_SIZE && copied_bytes > 0){
			p[byte_count + cluster_address] = file_p[file_size - copied_bytes];
			copied_bytes--;
			byte_count++;
		}

		if (copied_bytes == 0){
			printf("Sucessfully Copied File\n");
			update_FAT_entry(p, copy_cluster, 0xFFF);
		}

		int free_cluster = find_free_cluster(p);
		update_FAT_entry(p, copy_cluster, free_cluster);
		copy_cluster = free_cluster;
	}

	// Free memory
	munmap(file_p, file_sb.st_size);
	close(input_file);
	munmap(p, sb.st_size);
	close(fd);
	return 0;
}